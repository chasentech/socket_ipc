#ifndef _PKG_DEFINE_H_
#define _PKG_DEFINE_H_

#include <stdio.h>

// typedef enum MsgType
// {
//     MSG_REGISTER,
//     MSG_SET_TO_SEND,
//     MSG_CONTENT,
//     MSG_GET_LIST,
//     MSG_GET_SER_STATUS,
//     MSG_GET_SITE_STATUS, //site is connected site
//     MSG_EXE_CMD_SER,
//     MSG_EXE_CMD_SITE,    //site is connected site
//     MSG_RELOAD_SUP_CMD,
//     MSG_TYPE_NUM
// }MsgType;

// typedef struct DataDesc
// {
//     enum MsgType type;
//     char buff[MAX_BUF_LEN];
// }DataDesc;

#define STR_RET_LENGTH "ABCD"
#define STR_REGISTER_SUCCEED "AA s"
#define STR_REGISTER_FAILED  "AA f"
#define STR_SET_TO_SEND_SUCCEED "BB s"
#define STR_SET_TO_SEND_FAILED  "BB f"
#define STR_EXE_CMD_SUCCEED "CC s"
#define STR_EXE_CMD_FAILED  "CC f"

typedef enum PkgType
{
    PKG_REGISTER,           //cli--->ser
    PKG_SET_TO_SEND,        //cli--->ser
    PKG_GET_LIST,           //cli--->ser
    PKG_EXE_CMD,            //cli--->ser
    PKG_RET_TO_CLI,         //cli<---ser
    PKG_SHOW,               //cli<---ser
    PKG_CLIENT_DEF,         //cli--->cli
    PKG_TYPE_NUM
}PkgType;

typedef struct PkgHeader
{
    // from name
    //char from[16];
    // // to name
    // char to[16];
    // packet tyte
    PkgType type;
    // if type is PKG_CLIENT_DEF, the server will parse client_type
    // param are customized by the client
    int client_type;
    // packet body length
    int length;
}PkgHeader;

#endif
