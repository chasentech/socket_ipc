#include <stdio.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <stdlib.h>
#include <getopt.h>
#include <sys/sysinfo.h>

#include <iostream>
#include <vector>

#include "wrap.h"
#include "en_de_code.h"
#include "log.h"

static std::vector<std::string> g_sup_cmd;

static int is_running = 1;
static int is_running_thread = 1;

int efd_thread = 0;
void signalstop(int sign_no)
{
    is_running = 0;
    is_running_thread = 0;

    if(sign_no == SIGINT || sign_no == SIGQUIT || sign_no == SIGTERM) {
        uint64_t u = 10;
        ssize_t s = write(efd_thread, &u, sizeof(uint64_t));
        if (s != sizeof(uint64_t))
            perror("write");

        printf("in signalstop!\n");
    }
}

int send_self_info(int sockfd)
{
    DataDesc dataDesc;
    dataDesc.type = MSG_CONTENT;

    //获取内存信息
    struct sysinfo info;
    sysinfo(&info);
    char tmp[256] = {0};
    // printf("total: %ld, free: %ld\n", info.totalram,
    //             info.freeram);
    sprintf(tmp, "mem_total:%dMB, memoryUse:%dMB", (int)(info.totalram/1024/1024),
                (int)((info.totalram-info.freeram)/1024/1024));

    strcpy(dataDesc.buff, tmp);
    memset(tmp, 0, strlen(tmp));
    encode(&dataDesc, tmp);
    int send_buf_len = get_len(tmp);

    if (write(sockfd, tmp, send_buf_len) < 0) {
        perror("write failed!\n");
        return -1;
    }

    return 0;
}

int send_cmd_ret(int sockfd, char *buff)
{
    int len_tmp = strlen(buff);
    if (buff[len_tmp - 1] == '\n') {
        buff[len_tmp - 1] = '\0';
    }

    char encode_tmp[1024] = {0};
    DataDesc dataDesc;
    dataDesc.type = MSG_CONTENT;
    memset(&dataDesc.buff, 0, MAX_BUF_LEN);

    //remove space
    int i_space = 0;
    while (buff[i_space] == ' ') {
        i_space++;
    }

    do {
        int i_cmd_num = 0;
        for (i_cmd_num = 0; i_cmd_num < (int)g_sup_cmd.size(); i_cmd_num++) {
            if (strncmp(&buff[i_space], g_sup_cmd[i_cmd_num].c_str(),
                            strlen(g_sup_cmd[i_cmd_num].c_str())) == 0) {
                break; // find cmd
            }
        }

        if (i_cmd_num == (int)g_sup_cmd.size()) {
            strncpy(dataDesc.buff, "cmd is not support", strlen("cmd is not support"));
            break;
        }

        FILE *fp = popen(&buff[i_space], "r");
        if (fp == NULL) {
            perror("exe cmd failed");
            strncpy(dataDesc.buff, "exe cmd failed", strlen("exe cmd failed"));
            break;
        }

        char cmd_tmp[256] = {0};
        int cmd_tmp_len = 0;
        while(fgets(cmd_tmp, 256, fp) != NULL) {
            //每次读取一行
            int len = strlen(cmd_tmp);
            strncpy(&dataDesc.buff[cmd_tmp_len], cmd_tmp, len);
            cmd_tmp_len += len;
            memset(cmd_tmp, 0, 256);
        }

        pclose(fp);
        fp = NULL;

        if (strlen(dataDesc.buff) == 0) {
            strncpy(dataDesc.buff, "exe cmd no result", strlen("exe cmd no result"));
            break;
        }

        printf("cmd [%s]\n", &buff[i_space]);
        // printf("cmd ser [%s]\n", dataDesc.buff);

    } while(0);

    encode(&dataDesc, encode_tmp);
    int send_buf_len = get_len(encode_tmp);

    if (write(sockfd, encode_tmp, send_buf_len) < 0) {
        perror("write failed!\n");
        return -1;
    }

    return 0;
}


void *thread_rece(void *arg)
{
    int *param = (int *)arg;
    int sockfd = *param;
    //printf("into thread, arg is %d\n", sockfd);

    char buf_recv[MAX_BUF_LEN] = {0};

    fd_set allset;
    fd_set fdset;
    FD_ZERO(&allset);
    FD_ZERO(&fdset);
    FD_SET(sockfd, &allset);
    FD_SET(efd_thread, &allset);

    int max_fd = sockfd > efd_thread ? sockfd : efd_thread;

    while (is_running_thread == 1) {
        fdset = allset;
        int nready = select(max_fd+1, &fdset, NULL, NULL, NULL);
        if (nready <= 0) {
            perror("select error");
            break;
        }

        if (FD_ISSET(sockfd, &fdset)) {
            int n = Read(sockfd, buf_recv, MAX_BUF_LEN);
            if (n == 0) {
                printf("the other side has been closed.\n");
                break;
            }
            else {
                DataDesc dataDesc;
                decode(&dataDesc, buf_recv);
                int len = get_len(buf_recv);

                switch (dataDesc.type) {
                    case MSG_REGISTER: printf("MSG_REGISTER\n"); break;         //usually won't arrive
                    case MSG_SET_TO_SEND: printf("MSG_SET_TO_SEND\n"); break;   //usually won't arrive
                    case MSG_CONTENT: {
                        printf("ser---------->\n");
                        printf("%s\n\n", dataDesc.buff);
                        break;
                    }
                    case MSG_GET_LIST: printf("MSG_GET_LIST\n"); break;         //usually won't arrive
                    case MSG_GET_SITE_STATUS: {
                        printf("MSG_GET_SITE_STATUS\n");
                        send_self_info(sockfd);
                        break;
                    }
                    case MSG_EXE_CMD_SITE: {
                        printf("MSG_EXE_CMD_SITE\n");
                        send_cmd_ret(sockfd, dataDesc.buff);
                        break;
                    }
                    default: break;
                }

                //clear buff
                memset(&dataDesc.buff, 0, strlen(dataDesc.buff));
                memset(buf_recv, 0, len);
            }
        }
        if (FD_ISSET(efd_thread, &fdset)) {
            printf("select efd, to exit thread\n");
            uint64_t u = 0;
            ssize_t s = read(efd_thread, &u, sizeof(uint64_t));  
            if (s != sizeof(uint64_t))
                perror("read");  
            printf("read %llu efd_thread, to break\n",(unsigned long long)u);
        }
    }
    printf("thread exit!\n");
    return NULL;
}

static const char *short_options = "p:i:n:";
static struct option long_options[] = {
    {"connect port",    required_argument, 0, 'p'},
    {"connect IP",      required_argument, 0, 'i'},
    {"cli name",        required_argument, 0, 'n'},
    {0, 0, 0, 0}
};

static void usage(int argc, char *argv[])
{
    printf("%s usage:\n", argv[0]);
    printf("\t -i:  input server IP addr, default 127.0.0.1\n");
    printf("\t -p:  input server port, default 10500\n");
    printf("\t -n:  input cli name\n");
    printf("\t -h:  for help\n");
}

typedef struct cmd_str
{
    char short_str[10];
    char long_str[80];
}cmd_str;
cmd_str g_cmd_str[MSG_TYPE_NUM];

static void help_cmd()
{
    printf("help list:\n");
    for (int i = 0; i < MSG_TYPE_NUM; i++) {
        if (i == MSG_REGISTER || i ==  MSG_CONTENT)
            continue;
        printf("[%s]: %s\n", g_cmd_str[i].short_str, g_cmd_str[i].long_str);
    }
    printf("\n");
}

int load_sup_cmd()
{
    g_sup_cmd.clear();
    FILE *fp = fopen("../sup_cmd.txt", "r");
    if (fp == NULL) {
        perror("fopen sup_cmd.txt failed.");
        return -1;
    }

    char cmd_tmp[40] = {0};
    while(fgets(cmd_tmp, 40, fp) != NULL) {
        //每次读取一行
        if (cmd_tmp[strlen(cmd_tmp) - 1] == '\n')
            cmd_tmp[strlen(cmd_tmp) - 1] = '\0';
        g_sup_cmd.push_back(std::string(cmd_tmp));
        // printf("g_sup_cmd %s\n", std::string(cmd_tmp).c_str());
        memset(cmd_tmp, 0, 40);
    }
    fclose(fp);
    printf("load support cmd num is %ld\n", g_sup_cmd.size());
    return 0;
}

int main(int argc, char *argv[])
{
    // test_log();
    // return 0;

    log_init();
    load_sup_cmd();

    //index is enum MsgType
    strcpy(g_cmd_str[MSG_REGISTER].short_str, "");
    strcpy(g_cmd_str[MSG_REGISTER].long_str, "to register.");
    strcpy(g_cmd_str[MSG_SET_TO_SEND].short_str, "c1");
    strcpy(g_cmd_str[MSG_SET_TO_SEND].long_str,
                "set up chat with name. eg.\n\tc1 white --> set to white");
    strcpy(g_cmd_str[MSG_CONTENT].short_str, "");
    strcpy(g_cmd_str[MSG_CONTENT].long_str,
                "send msg to another side(default is this).");
    strcpy(g_cmd_str[MSG_GET_LIST].short_str, "c2");
    strcpy(g_cmd_str[MSG_GET_LIST].long_str, "get list online.");
    strcpy(g_cmd_str[MSG_GET_SER_STATUS].short_str, "c3");
    strcpy(g_cmd_str[MSG_GET_SER_STATUS].long_str, "get server status.");
    strcpy(g_cmd_str[MSG_GET_SITE_STATUS].short_str, "c4");
    strcpy(g_cmd_str[MSG_GET_SITE_STATUS].long_str, "get another site status.");
    strcpy(g_cmd_str[MSG_EXE_CMD_SER].short_str, "c5");
    strcpy(g_cmd_str[MSG_EXE_CMD_SER].long_str, "run cmd in ser. eg.\n\tc5 ls");
    strcpy(g_cmd_str[MSG_EXE_CMD_SITE].short_str, "c6");
    strcpy(g_cmd_str[MSG_EXE_CMD_SITE].long_str,
                "run cmd in another site. eg.\n\tc6 date");
    strcpy(g_cmd_str[MSG_RELOAD_SUP_CMD].short_str, "c7");
    strcpy(g_cmd_str[MSG_RELOAD_SUP_CMD].long_str, "reload supported cmd.");

    signal(SIGINT, signalstop);
    signal(SIGQUIT, signalstop);
    signal(SIGTERM, signalstop);

    int port = 10500;
    char str_IP[INET_ADDRSTRLEN] = "127.0.0.1";
    char name[50] = {0};

    int ch = 0;
    int option_index = 0;
    while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (ch) {

        case 'p':
            port = atoi(optarg);
            break;

        case 'i':
            strcpy(str_IP, optarg);
            break;

        case 'n':
            strcpy(&name[0], optarg);
            break;

        default:
            usage(argc, argv);
            return 0;
        }
    }

    if (strlen(name) == 0) {
        printf("please input name.\n");
        usage(argc, argv);
        return 0;
    }

    printf("connect %s:%d\n", str_IP, port);
    int sockfd = Socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    inet_pton(AF_INET, str_IP, &servaddr.sin_addr);
    servaddr.sin_port = htons(port);

    Connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

    efd_thread = eventfd(0, 0);
    if (efd_thread < 0) {
        perror("efd_thread init failed: \n");
    }

    //create thread to rece msg from server
    pthread_t ntid_rece;
    pthread_create(&ntid_rece, NULL, thread_rece, (void *)&sockfd);

    fd_set allset;
    fd_set fdset;
    FD_ZERO(&allset);
    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO, &allset);
    int max_fd = STDIN_FILENO;

    // register name
    char send_buf[512] = {0};
    DataDesc dataDesc;
    dataDesc.type = MSG_REGISTER;
    strcpy(dataDesc.buff, name);
    encode(&dataDesc, send_buf);

    int send_buf_len = get_len(send_buf);
    if (write(sockfd, send_buf, send_buf_len) < 0) {
        perror("cli write name failed");
    }
    memset(send_buf, 0, send_buf_len);
    memset(&dataDesc.buff, 0, send_buf_len);

    //in main process, 'ctrl+c'(SIGNALINT) will interrupt select, so no deal with this statu
    printf("input 'c help' to get help\n");
    while (is_running) {
        fdset = allset;
        int nready = select(max_fd+1, &fdset, NULL, NULL, NULL);
        if (nready <= 0) {
            perror("select error");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &fdset)) {
            char buf[256] = {0};
            if (fgets(buf, 256, stdin) == NULL) {
                perror("fgets error: ");
            }

            if (buf[0] == '\n') {
                continue;
            }

            int len_tmp = strlen(buf);
            if (buf[len_tmp - 1] == '\n') {
                buf[len_tmp - 1] = '\0';
            }

            if (strncmp(buf, "c help", strlen("c help")) == 0) {
                help_cmd();
            }
            else if (strncmp(buf, g_cmd_str[MSG_SET_TO_SEND].short_str,
                        strlen(g_cmd_str[MSG_SET_TO_SEND].short_str)) == 0) {
                dataDesc.type = MSG_SET_TO_SEND;
                unsigned char name_start_index = strlen(g_cmd_str[MSG_SET_TO_SEND].short_str)+1;
                strcpy(dataDesc.buff, &buf[name_start_index]);
                encode(&dataDesc, send_buf);

                int send_buf_len = get_len(send_buf);
                if (write(sockfd, send_buf, send_buf_len) < 0) {
                    perror("cli write name failed");
                }
                memset(send_buf, 0, send_buf_len);
                memset(&dataDesc.buff, 0, send_buf_len);
            }
            else if (strncmp(buf, g_cmd_str[MSG_GET_LIST].short_str,
                        strlen(g_cmd_str[MSG_GET_LIST].short_str)) == 0) {
                dataDesc.type = MSG_GET_LIST;
                encode(&dataDesc, send_buf);

                int send_buf_len = get_len(send_buf);
                if (write(sockfd, send_buf, send_buf_len) < 0) {
                    perror("cli write name failed");
                }
                memset(send_buf, 0, send_buf_len);
                memset(&dataDesc.buff, 0, send_buf_len);
            }
            else if (strncmp(buf, g_cmd_str[MSG_GET_SER_STATUS].short_str,
                        strlen(g_cmd_str[MSG_GET_SER_STATUS].short_str)) == 0) {
                dataDesc.type = MSG_GET_SER_STATUS;
                encode(&dataDesc, send_buf);

                int send_buf_len = get_len(send_buf);
                if (write(sockfd, send_buf, send_buf_len) < 0) {
                    perror("cli write name failed");
                }
                memset(send_buf, 0, send_buf_len);
                memset(&dataDesc.buff, 0, send_buf_len);
            }
            else if (strncmp(buf, g_cmd_str[MSG_GET_SITE_STATUS].short_str,
                        strlen(g_cmd_str[MSG_GET_SITE_STATUS].short_str)) == 0) {
                dataDesc.type = MSG_GET_SITE_STATUS; //Forward MSG
                encode(&dataDesc, send_buf);

                int send_buf_len = get_len(send_buf);
                if (write(sockfd, send_buf, send_buf_len) < 0) {
                    perror("cli write name failed");
                }
                memset(send_buf, 0, send_buf_len);
                memset(&dataDesc.buff, 0, send_buf_len);
            }
            else if (strncmp(buf, g_cmd_str[MSG_EXE_CMD_SER].short_str,
                        strlen(g_cmd_str[MSG_EXE_CMD_SER].short_str)) == 0) {
                dataDesc.type = MSG_EXE_CMD_SER; //Forward MSG
                unsigned char name_start_index = strlen(g_cmd_str[MSG_SET_TO_SEND].short_str)+1;
                strcpy(dataDesc.buff, &buf[name_start_index]);
                encode(&dataDesc, send_buf);

                int send_buf_len = get_len(send_buf);
                if (write(sockfd, send_buf, send_buf_len) < 0) {
                    perror("cli write name failed");
                }
                memset(send_buf, 0, send_buf_len);
                memset(&dataDesc.buff, 0, send_buf_len);
            }
            else if (strncmp(buf, g_cmd_str[MSG_EXE_CMD_SITE].short_str,
                        strlen(g_cmd_str[MSG_EXE_CMD_SITE].short_str)) == 0) {
                dataDesc.type = MSG_EXE_CMD_SITE; //Forward MSG
                unsigned char name_start_index = strlen(g_cmd_str[MSG_SET_TO_SEND].short_str)+1;
                strcpy(dataDesc.buff, &buf[name_start_index]);
                encode(&dataDesc, send_buf);

                int send_buf_len = get_len(send_buf);
                if (write(sockfd, send_buf, send_buf_len) < 0) {
                    perror("cli write name failed");
                }
                memset(send_buf, 0, send_buf_len);
                memset(&dataDesc.buff, 0, send_buf_len);
            }
            else if (strncmp(buf, g_cmd_str[MSG_RELOAD_SUP_CMD].short_str,
                        strlen(g_cmd_str[MSG_RELOAD_SUP_CMD].short_str)) == 0) {
                dataDesc.type = MSG_RELOAD_SUP_CMD;
                encode(&dataDesc, send_buf);

                int send_buf_len = get_len(send_buf);
                if (write(sockfd, send_buf, send_buf_len) < 0) {
                    perror("cli write name failed");
                }
                memset(send_buf, 0, send_buf_len);
                memset(&dataDesc.buff, 0, send_buf_len);
                load_sup_cmd();
            }
            else {
                dataDesc.type = MSG_CONTENT;
                strcpy(dataDesc.buff, buf);
                encode(&dataDesc, send_buf);

                int send_buf_len = get_len(send_buf);
                if (write(sockfd, send_buf, send_buf_len) < 0) {
                    perror("cli write name failed");
                }
                memset(send_buf, 0, send_buf_len);
                memset(&dataDesc.buff, 0, send_buf_len);
            }
        }
    }

    pthread_join(ntid_rece, NULL);

    Close(sockfd);
    printf("main exit!\n");
    log_deinit();

    return 0;
}
