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

#define STR_SUCCEED "succeed"
#define STR_FAILED  "failed "

typedef enum PkgType
{
    PKG_REGISTER,
    PKG_REGISTER_RET,
    PKG_SET_TO_SEND,
    PKG_SET_TO_SEND_RET,
    PKG_GET_LIST,
    PKG_SHOW,
    PKG_EXE_CMD,
    PKG_CLIENT_DEF,
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
