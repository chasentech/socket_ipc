#ifndef _EN_DE_CODE_H_
#define _EN_DE_CODE_H_

#include <stdio.h>

#define MAX_BUF_LEN 1024

typedef enum MsgType
{
    MSG_REGISTER,
    MSG_SET_TO_SEND,
    MSG_CONTENT,
    MSG_GET_LIST,
    MSG_GET_SER_STATUS,
    MSG_GET_SITE_STATUS, //site is connected site
    MSG_EXE_CMD_SER,
    MSG_EXE_CMD_SITE,    //site is connected site
    MSG_RELOAD_SUP_CMD,
    MSG_TYPE_NUM
}MsgType;

typedef struct DataDesc
{
    enum MsgType type;
    char buff[MAX_BUF_LEN];
}DataDesc;

int encode(DataDesc *dateDesc, char *buf);
int get_len(char *buf);
int decode(DataDesc *dateDesc, char *buf);

#endif
