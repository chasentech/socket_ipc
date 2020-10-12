#include <iostream>
#include <vector>

#include <stdio.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <sys/select.h>
#include <getopt.h>
#include <sys/sysinfo.h>

#include "wrap.h"
#include "en_de_code.h"
#include "log.h"

using namespace std;

#define MAX_CLI_NUM 100

typedef struct CliInfo
{
    char IP[INET_ADDRSTRLEN]; //INET_ADDRSTRLEN = 16
    int port;
    int sockfd;
    int to_fd;
    char name[20];
}CliInfo;

vector<CliInfo> g_vec_cli_info;
static std::vector<std::string> g_sup_cmd;

void print_cli_info(vector<CliInfo> &vec)
{
    cout << "********<<cli_info<<********" << endl;
    cout << "client number: " << vec.size() << endl;
    for (unsigned int i = 0; i < vec.size(); i++) {
        cout << "-----------------------------" << endl;
        cout << "|    cli[" << i << "] IP:     " << vec[i].IP << endl
             << "|    cli[" << i << "] port:   " << vec[i].port << endl
             << "|    cli[" << i << "] name:   " << vec[i].name << endl
             << "|    cli[" << i << "] sockfd: " << vec[i].sockfd << endl
             << "|    cli[" << i << "] to_fd:  " << vec[i].to_fd << endl;
        cout << "-----------------------------" << endl;
    }
    cout << endl;
}

void vec_rm_value(vector<CliInfo> &vec, int value)
{
    vector<CliInfo>::iterator it;
    for (it = vec.begin(); it != vec.end(); ) {
        if (it->sockfd == value)
            it = vec.erase(it);
        else it++;
    }
}

int get_fd_by_name(vector<CliInfo> &vec, char *name)
{
    vector<CliInfo>::iterator it;
    for (it = vec.begin(); it != vec.end(); it++) {
        printf("[%s] ? [%s]\n", it->name, name);
        if (strcmp(it->name, name) == 0) {
            return it->sockfd;
        }
    }
    return -1;
}

int get_name_by_fd(vector<CliInfo> &vec, int sockfd, char *name)
{
    vector<CliInfo>::iterator it;
    for (it = vec.begin(); it != vec.end(); it++) {
        if (it->sockfd == sockfd) {
            if (strcpy(name, it->name) == NULL) {
                perror("strcpy failed: ");
            }
            else return 0;
        }
    }
    return -1;
}

int set_name_by_fd(vector<CliInfo> &vec, char *name, int sockfd)
{
    vector<CliInfo>::iterator it;
    for (it = vec.begin(); it != vec.end(); it++) {
        if (it->sockfd == sockfd) {
            strcpy(it->name, name);
            printf("file name [%s] in fd[%d]\n", it->name, sockfd);
            return 0;
        }
    }
    return -1;
}

int set_to_fd_by_fd(vector<CliInfo> &vec, int to_fd, int sockfd)
{
    vector<CliInfo>::iterator it;
    for (it = vec.begin(); it != vec.end(); it++) {
        if (it->sockfd == sockfd) {
            it->to_fd = to_fd;
            return 0;
        }
    }
    return -1;
}

int get_to_fd_by_fd(vector<CliInfo> &vec, int sockfd)
{
    vector<CliInfo>::iterator it;
    for (it = vec.begin(); it != vec.end(); it++) {
        if (it->sockfd == sockfd) {
            return it->to_fd;
        }
    }
    return -1;
}

int send_server_info(int sockfd)
{
    DataDesc dataDesc;
    dataDesc.type = MSG_CONTENT;
    memset(&dataDesc.buff, 0, MAX_BUF_LEN);

    //get mem information
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

int send_list(vector<CliInfo> &vec, int sockfd)
{
    DataDesc dataDesc;
    dataDesc.type = MSG_CONTENT;
    memset(&dataDesc.buff, 0, MAX_BUF_LEN);

    vector<CliInfo>::iterator it;
    printf("client number is = %ld\n", vec.size());
    for (it = vec.begin(); it != vec.end(); it++) {
        char tmp[256] = {0};
        sprintf(tmp, "name:%-10s, IP:%-16s, port:%d, it_fd:%d, to_fd:%d",
                it->name, it->IP, it->port, it->sockfd, it->to_fd);

        strcpy(dataDesc.buff, tmp);
        memset(tmp, 0, strlen(tmp));
        encode(&dataDesc, tmp);
        int send_buf_len = get_len(tmp);

        if (write(sockfd, tmp, send_buf_len) < 0) {
            perror("write failed!\n");
            return -1;
        }
        memset(tmp, 0, send_buf_len);
        memset(&dataDesc.buff, 0, send_buf_len);

        usleep(10000); //necessary
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
            printf("cmd is not support\n");
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

static int listenfd = Socket(AF_INET, SOCK_STREAM, 0);

void signalstop(int signum)
{
    printf("catch signal!\n");
    // Close(listenfd);
    // g_vec_cli_info.clear();
    // exit(0);
}

static const char *short_options = "p:";
static struct option long_options[] = {
    {"connect port",    required_argument, 0, 'p'},
    {0, 0, 0, 0}
};

static void usage(int argc, char *argv[])
{
    printf("%s usage:\n", argv[0]);
    printf("\t -p:  input server port, default 10500.\n");
    printf("\t -h:  for help.\n");
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
    signal(SIGINT, signalstop);
    signal(SIGQUIT, signalstop);
    signal(SIGTERM, signalstop);

    log_init();
    load_sup_cmd();

    int port = 10500;

    int ch = 0;
    int option_index = 0;
    while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (ch) {

        case 'p':
            port = atoi(optarg);
            printf("listen port is %d\n", port);
            break;

        default:
            usage(argc, argv);
            return 0;
        }
    }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    Bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr));
    Listen(listenfd, MAX_CLI_NUM);

    int maxi = -1;
    int maxfd = listenfd;
    fd_set rset, allset;
    FD_ZERO(&allset);
    FD_SET(listenfd, &allset);

    int i = 0;
    int client_fd[FD_SETSIZE]; //FD_SETSIZE为1024
    for (i = 0; i < FD_SETSIZE; i++)
        client_fd[i] = -1;

    char str_IP_tmp[INET_ADDRSTRLEN]; //INET_ADDRSTRLEN = 16
    char buf_recv[MAX_BUF_LEN] = {0};
    // char buf_send[MAX_BUF_LEN] = {0};
    LOG_PRINT("Accepting connections ...\n");

    //in main process, 'ctrl+c'(SIGNALINT) will interrupt select, so no deal with this statu
    while (1) {
        rset = allset;
        int nready = select(maxfd+1, &rset, NULL, NULL, NULL);
        if (nready <= 0) {
            perror("select error");
            break;
        }
        //LOG_PRINT("nready = %d\n", nready);
        if (FD_ISSET(listenfd, &rset)) { // new connect arived
            LOG_PRINT("new connect arived\n");
            CliInfo cli_info;
            struct sockaddr_in cliaddr;
            socklen_t cliaddr_len = sizeof(cliaddr);
            int connfd = Accept(listenfd,
                         (struct sockaddr *)&cliaddr, &cliaddr_len);

            inet_ntop(AF_INET, &cliaddr.sin_addr, str_IP_tmp, sizeof(str_IP_tmp));
            strcpy(cli_info.IP, str_IP_tmp);
            cli_info.port = ntohs(cliaddr.sin_port);
            cli_info.sockfd = connfd;
            cli_info.to_fd = connfd;
            //cli_info.id = g_vec_cli_info.size() + 1;
            //sprintf(cli_info.name, "n%d", connfd);

            LOG_PRINT("connect from %s at PORT %d\n",
                str_IP_tmp, ntohs(cliaddr.sin_port));

            for (i = 0; i < FD_SETSIZE; i++) {
                if (client_fd[i] < 0) {
                    client_fd[i] = connfd; //save descriptor
                    break;
                }
            }
            if (i == FD_SETSIZE) {
                printf("too many clients\n");
                exit(1);
            }

            FD_SET(connfd, &allset);
            if (connfd > maxfd) maxfd = connfd; //maxfd for select
            if (i > maxi) maxi = i;     //maxi in client_fd

            g_vec_cli_info.push_back(cli_info);
            print_cli_info(g_vec_cli_info);
            //printf("max fd: %d\n", maxi + 1);

            if (--nready == 0) continue;
        }

        for (i = 0; i <= maxi; i++) { //already connect sockfd
            int sockfd = client_fd[i];
            if (sockfd < 0) continue;

            if (FD_ISSET(sockfd, &rset)){
                int n = Read(sockfd, buf_recv, MAX_BUF_LEN);
                if (n == 0){
                    //print_cli_info(g_vec_cli_info);
                    vec_rm_value(g_vec_cli_info, sockfd);
                    print_cli_info(g_vec_cli_info);

                    //cout << "client size: " << g_vec_cli_info.size() << endl;
                    printf("the other side has been closed.\n");
                    Close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client_fd[i] = -1;
                }
                else {
                    DataDesc dataDesc;
                    memset(&dataDesc.buff, 0, MAX_BUF_LEN);
                    decode(&dataDesc, buf_recv);
                    int len = get_len(buf_recv);
                    switch (dataDesc.type) {
                        case MSG_REGISTER: {
                            printf("MSG_REGISTER\n");
                            set_name_by_fd(g_vec_cli_info, dataDesc.buff, sockfd);
                            break;
                        }

                        case MSG_SET_TO_SEND: {
                            printf("MSG_SET_TO_SEND\n");
                            int len_tmp = strlen(dataDesc.buff);
                            if (dataDesc.buff[len_tmp - 1] == '\n') {
                                dataDesc.buff[len_tmp - 1] = '\0';
                            }
                            int to_fd = get_fd_by_name(g_vec_cli_info, dataDesc.buff);
                            if (to_fd != -1) {
                                printf(" send name = %s\n", dataDesc.buff);
                                printf(" send to_fd = %d\n", to_fd);
                                set_to_fd_by_fd(g_vec_cli_info, to_fd, sockfd);
                                set_to_fd_by_fd(g_vec_cli_info, sockfd, to_fd); // bind to each other
                            }
                            else {
                                printf("set to_fd failed!\n");
                            }
                            break;
                        }

                        case MSG_CONTENT: {
                            printf("MSG_CONTENT\n");
                            int to_fd = get_to_fd_by_fd(g_vec_cli_info, sockfd);
                            if (to_fd >= 0) {
                                char name_tmp_f[20] = {0};
                                char name_tmp_t[20] = {0};
                                get_name_by_fd(g_vec_cli_info, sockfd, name_tmp_f);
                                get_name_by_fd(g_vec_cli_info, to_fd, name_tmp_t);
                                printf("[%s]-->[%s]\n", name_tmp_f, name_tmp_t);

                                if (write(to_fd, buf_recv, len) < 0) {
                                    perror("ser write failed");
                                    set_to_fd_by_fd(g_vec_cli_info, sockfd, sockfd); // set to_fd is self
                                }
                            }
                            else {
                                printf("no't to set to_fd!\n");
                            }
                            break;
                        }

                        case MSG_GET_SER_STATUS: {
                            printf("MSG_GET_SER_STATUS\n");
                            char name_tmp_t[20] = {0};
                            get_name_by_fd(g_vec_cli_info, sockfd, name_tmp_t);
                            printf("[ser]-->[%s]\n", name_tmp_t);
                            send_server_info(sockfd);
                            break;
                        }

                        case MSG_GET_SITE_STATUS: {
                            printf("MSG_GET_SITE_STATUS\n");
                            int to_fd = get_to_fd_by_fd(g_vec_cli_info, sockfd);
                            if (to_fd >= 0) {
                                char name_tmp_f[20] = {0};
                                char name_tmp_t[20] = {0};
                                get_name_by_fd(g_vec_cli_info, sockfd, name_tmp_f);
                                get_name_by_fd(g_vec_cli_info, to_fd, name_tmp_t);
                                printf("[%s]-->[%s]\n", name_tmp_f, name_tmp_t);
                                if (write(to_fd, buf_recv, len) < 0) {
                                    perror("ser write failed");
                                    set_to_fd_by_fd(g_vec_cli_info, sockfd, sockfd); // set to_fd is self
                                }
                            }
                            else {
                                printf("no't to set to_fd!\n");
                            }
                            break;
                        }

                        case MSG_GET_LIST: {
                            printf("MSG_GET_LIST\n");
                            char name_tmp_t[20] = {0};
                            get_name_by_fd(g_vec_cli_info, sockfd, name_tmp_t);
                            printf("[ser]-->[%s]\n", name_tmp_t);
                            send_list(g_vec_cli_info, sockfd);
                            break;
                        }

                        case MSG_EXE_CMD_SER: {
                            printf("MSG_EXE_CMD_SER\n");
                            char name_tmp_t[20] = {0};
                            get_name_by_fd(g_vec_cli_info, sockfd, name_tmp_t);
                            printf("[ser]-->[%s]\n", name_tmp_t);
                            send_cmd_ret(sockfd, dataDesc.buff);
                            break;
                        }

                        case MSG_EXE_CMD_SITE: {
                            printf("MSG_EXE_CMD_SITE\n");
                            int to_fd = get_to_fd_by_fd(g_vec_cli_info, sockfd);
                            if (to_fd >= 0) {
                                char name_tmp_f[20] = {0};
                                char name_tmp_t[20] = {0};
                                get_name_by_fd(g_vec_cli_info, sockfd, name_tmp_f);
                                get_name_by_fd(g_vec_cli_info, to_fd, name_tmp_t);
                                printf("[%s]-->[%s]\n", name_tmp_f, name_tmp_t);
                                if (write(to_fd, buf_recv, len) < 0) {
                                    perror("ser write failed");
                                    set_to_fd_by_fd(g_vec_cli_info, sockfd, sockfd); // set to_fd is self
                                }
                            }
                            else {
                                printf("no't to set to_fd!\n");
                            }
                            break;
                        }
                        case MSG_RELOAD_SUP_CMD: {
                            printf("MSG_RELOAD_SUP_CMD\n");
                            printf("reload supported smd\n");
                            load_sup_cmd();
                            break;
                        }

                        default: break;
                    }
                    //clear buff
                    memset(&dataDesc.buff, 0, strlen(dataDesc.buff));
                    memset(buf_recv, 0, len);
                }

                if (--nready == 0) break; //notice maxi and nready relationship, can early exit
            }
        }
    }

    printf("main exit!\n");
    g_vec_cli_info.clear();
    Close(listenfd);
    log_deinit();

    return 0;
}
