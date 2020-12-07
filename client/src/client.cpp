#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <signal.h>
#include <getopt.h>
#include <netinet/in.h>

#include <fcntl.h>
#include <pthread.h>
#include <sys/eventfd.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "client_socket.h"
#include "pkg_define.h"

#define MAX_BUF_LEN 1024

using namespace std;

char fifo_name_pre[50] = "/tmp/fifo_cli";
int fd_fifo = -1;

bool g_fifo_running = true;
bool g_running = true;

bool register_statu = false;

typedef enum PkgClientType
{
    PKG_CLI_MSG = PKG_TYPE_NUM,
    PKG_CLI_EXE_CMD,
    PKG_CLI_TYPE_NUM
}PkgClientType;

typedef struct cmdStr
{
    char short_str[10];
    char long_str[80];
}cmdStr;

cmdStr g_cmd_str[PKG_CLI_TYPE_NUM] = {
    ".1", "to register [TBD]",                  // PKG_REGISTER
    ".2", "set up chat with name. eg: .2 name.",       // PKG_SET_TO_SEND
    ".3", "get list online",                    // PKG_GET_LIST
    ".4", "exec cmd on server. eg: .2 ls.",     // PKG_EXE_CMD
    "NULL",  "",                                // PKG_RET_TO_CLI
    "NULL",  "",                                // PKG_SHOW
    "NULL",  "",                                // PKG_CLIENT_DEF  // end of ser cmd
    ".5", "send msg to aother client [TBD]",    // PKG_CLI_MSG
    ".6", "exec cmd on aother client [TBD]",    // PKG_CLI_EXE_CMD
};

int show_client_help()
{
    printf("help:\n");
    for (int i = 0; i < PKG_CLI_TYPE_NUM; i++) {
        if (strcmp(g_cmd_str[i].short_str, "NULL") == 0) {
            continue;
        }
        printf("%s-->%s\n", g_cmd_str[i].short_str, g_cmd_str[i].long_str);
    }

    return 0;
}

void signalstop(int sign_no)
{
    g_fifo_running = false;
    g_running = false;
}


void print_header(PkgHeader *header)
{
    printf("header info:\n");
    // printf("from:   %s\n", header->from);
    // printf("to:     %s\n", header->to);
    printf("type:   %d\n", header->type);
    printf("length: %d\n", header->length);
}

int recv_CB(char *data, int len)
{
    PkgHeader header;
    int header_size = (int)sizeof(PkgHeader);
    memset(&header, 0, header_size);
    memcpy((void *)&header, (void *)data, header_size);
    //print_header(&header);
    printf("head:[%d] + body[%d]\n", header_size, len - header_size);

    bool client_type = false;
    switch (header.type) {
    case PKG_RET_TO_CLI: {
        if (len - header_size < (int)strlen(STR_RET_LENGTH)) {
            printf("[cli] PKG_REGISTER_RET buffer size err\n");
        }
        else if (strncmp(&data[header_size], STR_REGISTER_SUCCEED, strlen(STR_REGISTER_SUCCEED)) == 0) {
            printf("[cli] REGISTER_SUCCEED\n");
            register_statu = true;
        }
        else if (strncmp(&data[header_size], STR_REGISTER_FAILED, strlen(STR_REGISTER_FAILED)) == 0) {
            printf("[cli] REGISTER_FAILED\n");
            register_statu = false;
        }
        else if (strncmp(&data[header_size], STR_SET_TO_SEND_SUCCEED, strlen(STR_SET_TO_SEND_SUCCEED)) == 0) {
            printf("[cli] SET_TO_SEND_SUCCEED\n");
        }
        else if (strncmp(&data[header_size], STR_SET_TO_SEND_FAILED, strlen(STR_SET_TO_SEND_FAILED)) == 0) {
            printf("[cli] SET_TO_SEND_FAILED\n");
        }
        else if (strncmp(&data[header_size], STR_EXE_CMD_SUCCEED, strlen(STR_EXE_CMD_SUCCEED)) == 0) {
            printf("[cli] EXE_CMD_SUCCEED\n");
        }
        else if (strncmp(&data[header_size], STR_EXE_CMD_FAILED, strlen(STR_EXE_CMD_FAILED)) == 0) {
            printf("[cli] EXE_CMD_FAILED\n");
        }
        } break;

    case PKG_SHOW: {
        printf("[cli] PKG_SHOW\n");
        printf("%.*s\n", len - header_size, &data[header_size]); 
// %.*s 其中的.*表示显示的精度 对字符串输出(s)类型来说就是宽度
// 这个*代表的值由后面的参数列表中的整数型(int)值给出
        } break;

    case PKG_CLIENT_DEF: {
        client_type = true;
        } break;

    default: break;
    }

    if (client_type == true) {
        switch (header.client_type) {
        case PKG_CLI_MSG: {
            printf("[cli] PKG_CLI_MSG\n");
            printf("%.*s\n", len - header_size, &data[header_size]); 
            } break;

        default: break;
        }
    }

    static unsigned int i_rece = 0;
    i_rece++;
    printf("***********************************rece count is %d\n", i_rece);
    return 0;
}

static void usage(int argc, char *argv[])
{
    printf("%s usage:\n", argv[0]);
    printf("\t -I:  into input mode.\n");
    printf("\t -i:  input server IP addr, default 127.0.0.1\n");
    printf("\t -p:  input server portm default 10500.\n");
    printf("\t -n:  input client name.\n");
    printf("\t -h:  for help.\n");
}

void input_loop(const char *fifo_name)
{
	int fd = open(fifo_name, O_WRONLY);
	if (fd < 0) {
		perror("[cli] open failed");
		return;
	}

    fd_set allset;
    fd_set fdset;
    FD_ZERO(&allset);
    FD_ZERO(&fdset);
    FD_SET(STDIN_FILENO, &allset);
    int max_fd = STDIN_FILENO;

    printf("input '.help' to get help\n");
    while (g_fifo_running) {
        fdset = allset;
        int nready = select(max_fd+1, &fdset, NULL, NULL, NULL);
        if (nready <= 0) {
            perror("[cli] select error");
            break;
        }

        if (FD_ISSET(STDIN_FILENO, &fdset)) {
            char buf[256] = {0};
            if (fgets(buf, 256, stdin) == NULL) {
                perror("[cli] fgets error: ");
            }

            if (buf[0] == '\n') {
                continue;
            }

            int len_tmp = strlen(buf);
            if (buf[len_tmp - 1] == '\n') {
                buf[len_tmp - 1] = '\0';
            }

            if (write(fd, buf, sizeof(buf)) < 0) {
                perror("[cli] fifo write");
            }
        }
    }
    close(fd);
    printf("[cli] input mode exit\n");
}

static const char *short_options = "Ip:i:n:h";
static struct option long_options[] = {
    {"input mode",      required_argument, 0, 'I'},
    {"connect port",    required_argument, 0, 'p'},
    {"connect IP",      required_argument, 0, 'i'},
    {"name",            required_argument, 0, 'n'},
    {"help",            required_argument, 0, 'h'},
    {0, 0, 0, 0}
};

int main(int argc, char *argv[])
{
    printf("in main\n");

    signal(SIGINT, signalstop);
    signal(SIGQUIT, signalstop);
    signal(SIGTERM, signalstop);

    int port = 10500;
    string ip("127.0.0.1");
    char name[20] = "null";

    //for debug
    bool input_mode = false;

    int ch = 0;
    int option_index = 0;
    while ((ch = getopt_long(argc, argv, short_options, long_options, &option_index)) != -1) {
        switch (ch) {

        case 'I':
            input_mode = true;
            break;

        case 'p':
            port = atoi(optarg);
            break;

        case 'i':
            ip = string(optarg);
            break;

        case 'n':
            strcpy(name, optarg);
            break;

        case 'h':
            usage(argc, argv);
            return 0;

        default:
            usage(argc, argv);
            return 0;
        }
    }

    if (strcmp(name, "null") == 0) {
        printf("please input name\n");
        usage(argc, argv);
        return 0;
    }

    char fifo_name[126] = {0};
    sprintf(fifo_name, "%s_%s", fifo_name_pre, name);

    if (input_mode == true) {
        input_loop(fifo_name);
        return 0;
    }

    if (access(fifo_name, F_OK) == 0) {
		if (unlink(fifo_name) < 0) {
			perror("[cli] unlink failed!");
		}
	}
	int ret = mkfifo(fifo_name, 0777);
	if (ret < 0) {
		perror("[cli] mkfifo failed!\n");
		return -1;
	}
	fd_fifo = open(fifo_name, O_RDWR);
	if (fd_fifo < 0) {
		perror("[cli] open failed");
		return -1;
	}

    //connect
    IpcClientBase ipc_client;
    ipc_client.setCB(&recv_CB);
    ipc_client.connectServer(ip.c_str(), port);

    fd_set allset;
    fd_set fdset;
    FD_ZERO(&allset);
    FD_ZERO(&fdset);
    FD_SET(fd_fifo, &allset);
    int max_fd = fd_fifo;

    //register
    PkgHeader header;
    memset(&header, 0, sizeof(PkgHeader));
    header.type = PKG_REGISTER;
    header.length = strlen(name);
    ipc_client.send(&header, name, header.length);

    //TODO
    //log in
    //username
    //password
    int i_register = 0;
    for (i_register = 0; i_register < 30; i_register++) {
        if (register_statu == true) {
            break;
        }
        usleep(100000); // 100ms * 30 = 3s
    }
    if (i_register >= 10) {
        printf("[cli] register faield\n");
        ipc_client.disConnect();

        if (access(fifo_name, F_OK) == 0) {
            if (unlink(fifo_name) < 0) {
                perror("unlink failed!");
            }
        }

        return -1;
    }
    printf("[cli] register success\n");

    //in main process, 'ctrl+c'(SIGNALINT) will interrupt select, so no deal with this statu
    while (g_running) {
        fdset = allset;
        int nready = select(max_fd+1, &fdset, NULL, NULL, NULL);
        if (nready <= 0) {
            perror("[cli] select error");
            break;
        }

        if (FD_ISSET(fd_fifo, &fdset)) {
            char buf[256] = {0};
            if (read(fd_fifo, buf, 256) < 0) {
                perror("[cli] fifo read:");
                continue;
            }

            printf("[cli] fifo read succeed\n");
            memset(&header, 0, sizeof(PkgHeader));
            //char body[MAX_BUF_LEN] = {0};
            if (strncmp(buf, ".help", strlen(".help")) == 0) {
                show_client_help();
            }
            else if (strncmp(buf, g_cmd_str[PKG_REGISTER].short_str,
                        strlen(g_cmd_str[PKG_REGISTER].short_str)) == 0) {
                printf("input .1\n");
                //header.type = PKG_REGISTER;
            }
            else if (strncmp(buf, g_cmd_str[PKG_SET_TO_SEND].short_str,
                        strlen(g_cmd_str[PKG_SET_TO_SEND].short_str)) == 0) {
                printf("input .2\n");
                //TODO remove space.
                //TODO check str on client or server
                int cmd_len = strlen(g_cmd_str[PKG_SET_TO_SEND].short_str) + 1; // 1 is space
                header.type = PKG_SET_TO_SEND;
                if (cmd_len < (int)strlen(buf)) {
                    header.length = strlen(buf) - cmd_len;
                    ipc_client.send(&header, &buf[cmd_len], header.length);
                }
                else {
                    printf("input to_name err\n\n");
                }
            }
            else if (strncmp(buf, g_cmd_str[PKG_GET_LIST].short_str,
                        strlen(g_cmd_str[PKG_GET_LIST].short_str)) == 0) {
                printf("input .3\n");
                header.type = PKG_GET_LIST;
                header.length = 0;
                ipc_client.send(&header, NULL, header.length);
            }
            else if (strncmp(buf, g_cmd_str[PKG_EXE_CMD].short_str,
                        strlen(g_cmd_str[PKG_EXE_CMD].short_str)) == 0) {
                printf("input .4\n");
                //TODO remove space
                //TODO check str on client or server
                int cmd_len = strlen(g_cmd_str[PKG_EXE_CMD].short_str) + 1; // 1 is space
                header.type = PKG_EXE_CMD;
                //header.length = 0;
                //ipc_client.send(&header, NULL, header.length);
                if (cmd_len < (int)strlen(buf)) {
                    header.length = strlen(buf) - cmd_len;
                    ipc_client.send(&header, &buf[cmd_len], header.length);
                }
                else {
                    printf("input to_name err\n\n");
                }
            }
            else {  //default is seng msg each other
                header.type = PKG_CLIENT_DEF;
                header.client_type = PKG_CLI_MSG;
                header.length = strlen(buf);
                ipc_client.send(&header, buf, header.length);
            }
        }
    }

    if (access(fifo_name, F_OK) == 0) {
        if (unlink(fifo_name) < 0) {
            perror("unlink failed!");
        }
    }

    ipc_client.disConnect();
    printf("[cli] main exit\n");

    return 0;
}
