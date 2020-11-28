#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "client_func.h"

// static cmdStr g_cmd_str[PKG_CLI_TYPE_NUM] = {
//     ".1", "to register",                        // PKG_REGISTER
//     ".2", "set up chat with name",              // PKG_SET_TO_SEND
//     ".3", "get list online",                    // PKG_GET_LIST 
//     ".4", "exec cmd on server",                 // PKG_EXE_CMD
//     "NULL",  "",                                // PKG_CLIENT_DEF
//     ".5", "send msg to aother client",          // PKG_CLI_MSG
//     ".6", "exec cmd on aother client",          // PKG_CLI_EXE_CMD
// };






// need to reply by return value
// 0 success
// 1 need reply
int process_recv(char *data, int len)
{
    // PkgHeader header;
    // memset(&header, 0, sizeof(PkgHeader));
    // memcpy((void *)&header, (void *)data, sizeof(PkgHeader));
    // //print_header(&header);
    // printf("head:[%d] + body[%d]\n", (int)sizeof(PkgHeader), len - (int)sizeof(PkgHeader));

    // //输出指定长度的字符串, 超长时截断, 不足时左对齐是:
    // // printf("%-N.Ms", str);          --N 为最终的字符串输出长度
    // //                                 --M 为从参数字符串中取出的子串长度

    // switch (header.type) {
    // case PKG_SHOW: {
    //     char tmp[header.length + 1] = {0}; //TODO
    //     memcpy(tmp, &data[sizeof(PkgHeader)], header.length);
    //     printf("len: %d, date: %s\n", header.length, tmp);
    //     } break;

    // case PKG_CONTENT: {
    //     printf("[ser] MSG_CONTENT\n");
    //     } break;

    // default: break;
    // }



    return 0;
}

int fill_send_data(char *buf, PkgHeader *header, char *body, int len)
{
    // if (strncmp(buf, ".help", strlen(".help")) == 0) {
    //     show_client_help();
    // }
    // else if (strncmp(buf, ".1", strlen(".1")) == 0) {
    //     printf("inpout .1\n");
    // }
    // else if (strncmp(buf, ".2", strlen(".2")) == 0) {
    //     printf("inpout .2\n");
    // }
    // else if (strncmp(buf, g_cmd_str[PKG_GET_LIST].short_str,
    //             strlen(g_cmd_str[PKG_GET_LIST].short_str)) == 0) {
    //     //strcpy(header.from, name);
    //     header->type = PKG_GET_LIST;
    //     header->length = 0;

    //     printf("inpout .3\n");
    // }
    // else {
    //     printf("input unknow\n");
    // }


    // memset(&header, 0, sizeof(PkgHeader));
    // strcpy(header.from, name.c_str());
    // strcpy(header.to, to_name.c_str());
    // header.type = PKG_CONTENT;
    // char temp[2048] = {0};
    // memset(temp, 5, 2048);
    // header.length = 2048;
    // ipc_client.send(&header, temp, header.length);

    // memset(&header, 0, sizeof(PkgHeader));
    // strcpy(header.from, name.c_str());
    // strcpy(header.to, name.c_str());
    // header.type = PKG_GET_LIST;
    // header.length = 0;
    // ipc_client.send(&header, NULL, 0);

    // static unsigned int i_send = 0;
    // i_send += 1;
    // printf("***********************************send count is %d\n", i_send);

    // /* When sending is too fast, the server may be abnormal due to the
    //  * limitation of the serial port baud rate, please run the server in telnet
    //  */
    // usleep(1000000);
    return 0;
}
