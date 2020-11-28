#ifndef _CLIENT_FUNC_H_
#define _CLIENT_FUNC_H_

#include <stdio.h>

#include "pkg_define.h"

// typedef enum PkgClientType
// {
//     PKG_CLI_MSG = PKG_TYPE_NUM,
//     PKG_CLI_EXE_CMD,
//     PKG_CLI_TYPE_NUM
// }PkgClientType;


// typedef struct cmdStr
// {
//     char short_str[10];
//     char long_str[80];
// }cmdStr;

// need to reply by return value
// 0 success
// 1 need reply
int process_recv(char *data, int len);

int fill_send_data(char *buf, PkgHeader *header, char *body, int len);


#endif
