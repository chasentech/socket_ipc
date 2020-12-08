#include <iostream>
#include <vector>
#include <mutex>
#include <condition_variable>

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
#include <time.h>
#include <sys/time.h>

#include "pkg_define.h"
#include "mem_pool.h"
#include "log.h"

using namespace std;

//#define DEBUG

#define MAX_CLI_NUM 100
#define MAX_BUF_LEN 1024

#define BLOCK_SIZE 1024 * 256  //256K

int g_thread_running = 1;
MemPool g_mem_pool;
bool g_recv_flag = false;

std::mutex mutex_mem_pool;

std::mutex mutex_thread_recv;
std::condition_variable cond_thread_recv;

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

CliInfo *get_cli_by_name(vector<CliInfo> &vec, char *name)
{
    vector<CliInfo>::iterator it;
    for (it = vec.begin(); it != vec.end(); it++) {
        //printf("[ser] [%s] ? [%s]\n", it->name, name);
        if (strcmp(it->name, name) == 0) {
            return &(*it);
        }
    }
    return NULL;
}
CliInfo *get_cli_by_fd(vector<CliInfo> &vec, int fd)
{
    vector<CliInfo>::iterator it;
    for (it = vec.begin(); it != vec.end(); it++) {
        if (it->sockfd == fd) {
            return &(*it);
        }
    }
    return NULL;
}
CliInfo *get_cli_by_to_fd(vector<CliInfo> &vec, int to_fd)
{
    vector<CliInfo>::iterator it;
    for (it = vec.begin(); it != vec.end(); it++) {
        if (it->to_fd == to_fd) {
            return &(*it);
        }
    }
    return NULL;
}

int send(int sockfd, PkgHeader *header, const char *data, int len)
{
    if (write(sockfd, header, sizeof(PkgHeader)) < 0) {
        perror("[ser] write PkgHeader failed");
        return -1;
    }
    if (len <= 0) {
        return 0;
    }
    if (write(sockfd, data, len) < 0) {
        perror("[ser] write data failed");
        return -1;
    }
    return 0;
}

int send_server_info(int sockfd)
{/*
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
        perror("[ser] write failed!\n");
        return -1;
    }
*/
    return 0;
}

int send_list(vector<CliInfo> &vec, int sockfd)
{
    PkgHeader header;
    memset(&header, 0, sizeof(PkgHeader));
    header.type = PKG_SHOW;

    vector<CliInfo>::iterator it;
    //printf("[ser] client number is = %ld\n", vec.size());
    for (it = vec.begin(); it != vec.end(); it++) {
        char tmp[256] = {0};
        sprintf(tmp, "name:%-10s, IP:%-16s, port:%d, it_fd:%d, to_fd:%d",
                it->name, it->IP, it->port, it->sockfd, it->to_fd);

        header.length = strlen(tmp);
        send(sockfd, &header, tmp, header.length);
    }

    return 0;
}

//TODO remove space
//TODO check str on client or server
int send_cmd_ret(int sockfd, char *buff)
{
    PkgHeader header;
    memset(&header, 0, sizeof(PkgHeader));
    header.type = PKG_SHOW;

    // int len_tmp = strlen(buff);

    do {
        FILE *fp = popen(buff, "r");
        if (fp == NULL) {
            LOG_PERROR("[ser] exe cmd failed");
            break;
        }

        char cmd_tmp[256] = {0};
        // int cmd_tmp_len = 0;
        while(fgets(cmd_tmp, 256, fp) != NULL) {

            header.length = strlen(cmd_tmp);
            send(sockfd, &header, cmd_tmp, header.length);

            // //每次读取一行
            // int len = strlen(cmd_tmp);
            // strncpy(&dataDesc.buff[cmd_tmp_len], cmd_tmp, len);
            // cmd_tmp_len += len;
            memset(cmd_tmp, 0, 256);
        }
        pclose(fp);
        fp = NULL;

    } while (0);

/*
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
            printf("[ser] cmd is not support\n");
            strncpy(dataDesc.buff, "cmd is not support", strlen("cmd is not support"));
            break;
        }

        FILE *fp = popen(&buff[i_space], "r");
        if (fp == NULL) {
            perror("[ser] exe cmd failed");
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

        printf("[ser] cmd [%s]\n", &buff[i_space]);
        // printf("cmd ser [%s]\n", dataDesc.buff);

    } while(0);

    encode(&dataDesc, encode_tmp);
    int send_buf_len = get_len(encode_tmp);

    if (write(sockfd, encode_tmp, send_buf_len) < 0) {
        perror("[ser] write failed!\n");
        return -1;
    }
*/
    return 0;
}

static int listenfd = socket(AF_INET, SOCK_STREAM, 0);

void signalstop(int signum)
{
    g_thread_running = 0;

    std::unique_lock <std::mutex> lck(mutex_thread_recv);
    g_recv_flag = true;
    cond_thread_recv.notify_all();

    printf("[ser] catch signal!\n");
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
        perror("[ser] fopen sup_cmd.txt failed.");
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
    printf("[ser] load support cmd num is %ld\n", g_sup_cmd.size());
    return 0;
}

void *thread_proc_msg(void *arg)
{
    while (g_thread_running) {
        std::unique_lock <std::mutex> lck(mutex_thread_recv);
        //std::cout << "wait......, flag" << g_recv_flag << std::endl;
        while (g_recv_flag == false) {
            cond_thread_recv.wait(lck);
        }
        //printf("[ser] thread_proc_msg wait success! flag = %d\n", g_recv_flag);
#ifdef DEBUG
        g_mem_pool.show_mem_pool_status();
#endif
        std::lock_guard <std::mutex> l_mp(mutex_mem_pool);

        int block_num = g_mem_pool.get_block_num();
#ifdef DEBUG
        printf("                                   block_num = %d\n", block_num);
#endif
        struct timeval tv5, tv6;
        gettimeofday(&tv5, NULL);
        for (int i = 0; i < block_num; i++) {

            while (1) {
                MemPool::BlockDesc block = g_mem_pool.get_block_desc(i);
                PkgHeader header;
                memset(&header, 0, sizeof(PkgHeader));

                // char sys_call_str[50] = {0};
                // sprintf(sys_call_str, "echo %d >> /root/reocrd.txt", block.use);
                // if (system(sys_call_str)) {};
#ifdef DEBUG
                printf("---------------block.use = %d\n", block.use);
#endif
                if (block.use >= (int)sizeof(PkgHeader)) {
                    //std::lock_guard <std::mutex> lck(mutex_mem_pool);
                    int n = g_mem_pool.read_data(block.id, (char *)&header, sizeof(PkgHeader));
                    if (n < 0) {
                        printf("[ser] g_mem_pool read_data error!\n");
                        break;
                    }
                }
                else break;

                int len_total = (int)sizeof(PkgHeader) + header.length;
#ifdef DEBUG
                printf("PkgHeader = %d, len_total = %d\n", (int)sizeof(PkgHeader), len_total);
#endif
                if (block.use >= len_total) {
                    char *data = new char[len_total + 1]; // TODO 1.优化 2.+1 is end '\0'
                    memset(data, 0, len_total + 1);
                    {
                        //std::lock_guard <std::mutex> lck(mutex_mem_pool);
                        int n = g_mem_pool.pop(block.id, data, len_total);
                        if (n < 0) {
                            printf("[ser] g_mem_pool pop error!\n");
                            break;
                        }
                    }
#ifdef DEBUG
                    printf("[ser] query header and body!\n");
#endif
                    CliInfo *cli_info = NULL;
                    cli_info = get_cli_by_fd(g_vec_cli_info, block.id);
                    if (cli_info == NULL) {
                        printf("block.id != sockfd, err!!!\n");
                        break;
                    }
                    switch (header.type) {
                    case PKG_REGISTER: {
                        LOG_PRINT("[%s]->[ser] PKG_REGISTER\n", cli_info->name);

                        PkgHeader header_ret;
                        memset(&header_ret, 0, sizeof(header_ret));
                        header_ret.type = PKG_RET_TO_CLI;

                        strcpy(cli_info->name, &data[sizeof(PkgHeader)]);
                        LOG_PRINT("fill name: IP: %s, port: %d, name: %s, sockfd: %d, to_fd: %d\n",
                                cli_info->IP, cli_info->port, cli_info->name, cli_info->sockfd, cli_info->to_fd);

                        // TODO check username and password
                        // printf("[ser] get cli by fd failed! error\n");
                        // header_ret.length = strlen(STR_REGISTER_FAILED);

                        header_ret.length = strlen(STR_REGISTER_SUCCEED);
                        send(cli_info->sockfd, &header_ret, STR_REGISTER_SUCCEED, header_ret.length);
                        LOG_PRINT("[ser]->[%s] STR_REGISTER_SUCCEED\n", cli_info->name);
                        } break;

                    case PKG_SET_TO_SEND: {
                        LOG_PRINT("[%s]->[ser] PKG_SET_TO_SEND\n", cli_info->name);

                        PkgHeader header_ret;
                        memset(&header_ret, 0, sizeof(header_ret));
                        header_ret.type = PKG_RET_TO_CLI;

                        CliInfo *cli_to = get_cli_by_name(g_vec_cli_info, &data[sizeof(PkgHeader)]);
                        if (cli_to == NULL) {
                            LOG_PRINT("[%s] to_name is unknow\n", &data[sizeof(PkgHeader)]);
                            header_ret.length = strlen(STR_SET_TO_SEND_FAILED);
                            send(cli_info->sockfd, &header_ret, STR_SET_TO_SEND_FAILED, header_ret.length);
                            LOG_PRINT("[ser]->[%s] STR_SET_TO_SEND_FAILED\n", cli_info->name);
                        }
                        else {
                            cli_info->to_fd = cli_to->sockfd; //bind each other
                            cli_to->to_fd = cli_info->sockfd; //bind each other
                            header_ret.length = strlen(STR_SET_TO_SEND_SUCCEED);
                            send(cli_info->sockfd, &header_ret, STR_SET_TO_SEND_SUCCEED, header_ret.length);
                            LOG_PRINT("[ser]->[%s] STR_SET_TO_SEND_SUCCEED\n", cli_info->name);
                        }
                        } break;

                    case PKG_GET_LIST: {
                        LOG_PRINT("[ser]->[%s] PKG_GET_LIST\n", cli_info->name);
                        send_list(g_vec_cli_info, block.id); //block.id = sockfd
                        } break;

                    case PKG_EXE_CMD: {
                        LOG_PRINT("[%s]->[ser] PKG_EXE_CMD\n", cli_info->name);
                        send_cmd_ret(block.id, &data[sizeof(PkgHeader)]);
                        } break;

                    case PKG_CLIENT_DEF: {
                        LOG_PRINT("[ser] PKG_CLIENT_DEF\n");
                        CliInfo *cli_to = get_cli_by_fd(g_vec_cli_info, cli_info->to_fd);
                        if (cli_to != NULL) {
                            int ret = send(cli_info->to_fd, &header, &data[(int)sizeof(PkgHeader)], header.length);
                            if (ret < 0) {
                                LOG_PRINT("[ser] PKG_CLIENT_DEF failed\n");
                                cli_info->to_fd = cli_info->sockfd;
                            }
                            else {
                                LOG_PRINT("[%s]->[%s] PKG_CLIENT_DEF succeed\n", cli_info->name, cli_to->name);
                            }
                        }
                        else {
                            LOG_PRINT("[ser] cli_to is NULL\n");
                            cli_info->to_fd = cli_info->sockfd;
                        }
                        } break;

                    default: break;
                    }

                    delete []data;
                }
                else break;

            }
        }
        gettimeofday(&tv6, NULL);
#ifdef DEBUG
        printf("                                   total parse     %ld us\n",
                (tv6.tv_sec - tv5.tv_sec) * 1000000 + (tv6.tv_usec - tv5.tv_usec));
#endif
        g_recv_flag = false;
    }

    printf("[ser] thread exit!\n");
    return NULL;
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
            printf("[ser] listen port is %d\n", port);
            break;

        default:
            usage(argc, argv);
            return 0;
        }
    }

    if (listenfd < 0) {
        LOG_PERROR("socket failed");
    }

    int opt = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(port);

    if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
        perror("[ser] bind err");
        return -1;
    }
    if (listen(listenfd, MAX_CLI_NUM) < 0) {
        perror("[ser] listen err");
        return -1;
    }

    pthread_t ntid_proc_msg;
    pthread_create(&ntid_proc_msg, NULL, thread_proc_msg, NULL);

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
    LOG_PRINT("[ser] Accepting connections ...\n");
    sleep(1);
    //LOG_ERROR("[ser] Accepting connections ...\n");


    //in main process, 'ctrl+c'(SIGNALINT) will interrupt select, so no deal with this statu
    while (1) {
        rset = allset;
        int nready = select(maxfd+1, &rset, NULL, NULL, NULL);
        if (nready <= 0) {
            perror("[ser] select error");
            break;
        }
        //LOG_PRINT("nready = %d\n", nready);
        if (FD_ISSET(listenfd, &rset)) { // new connect arived
            LOG_PRINT("[ser] new connect arived\n");
            CliInfo cli_info;
            struct sockaddr_in cliaddr;
            socklen_t cliaddr_len = sizeof(cliaddr);
            int connfd = accept(listenfd,
                         (struct sockaddr *)&cliaddr, &cliaddr_len);

            if (connfd < 0) {
                perror("[ser] accept err");
                continue;
            }

            inet_ntop(AF_INET, &cliaddr.sin_addr, str_IP_tmp, sizeof(str_IP_tmp));
            strcpy(cli_info.IP, str_IP_tmp);
            cli_info.port = ntohs(cliaddr.sin_port);
            cli_info.sockfd = connfd;
            cli_info.to_fd = connfd;
            //cli_info.id = g_vec_cli_info.size() + 1;
            //sprintf(cli_info.name, "n%d", connfd);

            LOG_PRINT("[ser] connect from %s at PORT %d\n",
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
                int n = read(sockfd, buf_recv, MAX_BUF_LEN);
                if (n < 0) {
                    perror("[ser] read err");
                    continue;
                }
                else if (n == 0) {
                    std::lock_guard <std::mutex> l_mp(mutex_mem_pool);

                    g_mem_pool.delete_block(sockfd);

                    vec_rm_value(g_vec_cli_info, sockfd);
                    print_cli_info(g_vec_cli_info);

                    //cout << "client size: " << g_vec_cli_info.size() << endl;
                    printf("[ser] the other side has been closed.\n");
                    close(sockfd);
                    FD_CLR(sockfd, &allset);
                    client_fd[i] = -1;
                }
                else {
                    struct timeval tv5, tv6;
                    gettimeofday(&tv5, NULL);
                    {
                        std::lock_guard <std::mutex> lck(mutex_mem_pool);
                        g_mem_pool.add_block(sockfd, BLOCK_SIZE); //notice [add_block] in mem_pool
                        g_mem_pool.push(sockfd, buf_recv, n);
                    }

                    //notice thread to process msg
                    std::unique_lock <std::mutex> lck(mutex_thread_recv);
                    g_recv_flag = true;
                    cond_thread_recv.notify_all();
                    //print_cli_info(g_vec_cli_info);

                    gettimeofday(&tv6, NULL);
#ifdef DEBUG
                    printf("cond_thread_recv.notify_all()\n");
                    printf("                                   total push      %ld us\n",
                            (tv6.tv_sec - tv5.tv_sec) * 1000000 + (tv6.tv_usec - tv5.tv_usec));
#endif
                }

                memset(buf_recv, 0, n);
                if (--nready == 0) break; //notice maxi and nready relationship, can early exit
            }
        }
    }

    printf("[ser] main exit!\n");
    g_vec_cli_info.clear();
    pthread_join(ntid_proc_msg, NULL);
    close(listenfd);
    log_deinit();

    return 0;
}
