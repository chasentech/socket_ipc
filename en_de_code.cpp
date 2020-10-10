#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "en_de_code.h"

#define HEAD_OFFSET (0)
#define HEAD_LENGTH (1)
#define LEN_OFFSET (HEAD_OFFSET + HEAD_LENGTH)  // 1
#define LEN_LENGTH (4)
#define TYPE_OFFSET (LEN_OFFSET + LEN_LENGTH)   // 5
#define TYPE_LENGTH (4)
#define CONTENT_OFFSET (TYPE_OFFSET + TYPE_LENGTH) // 9

// #define LEN_OFFSET  (1)
// #define LEN_LENGTH  (4)
// #define TYPE_OFFSET (5)
// #define TYPE_LENGTH (4)
// #define CONTENT_OFFSET (TYPE_OFFSET + TYPE_LENGTH)

int encode(DataDesc *dateDesc, char *buf)
{
    int len = CONTENT_OFFSET + strlen(dateDesc->buff) + 1;
    buf[0] = '#';
    memcpy(&buf[TYPE_OFFSET], &(dateDesc->type), TYPE_LENGTH);
    strcpy(&buf[CONTENT_OFFSET], dateDesc->buff);
    buf[len - 1] = '$';
    memcpy(&buf[LEN_OFFSET], &len, LEN_LENGTH);

    return 0;
}

int get_len(char *buf)
{
    int len = 0;
    memcpy(&len, &buf[LEN_OFFSET], LEN_LENGTH);
    return len;
}

int decode(DataDesc *dateDesc, char *buf)
{
    int len = get_len(buf);
    if (buf[0] != '#' && buf[len - 1] != '$') {
        printf("decode failed: lenth error\n");
    }

    buf[len - 1] = '\0';
    memcpy(&(dateDesc->type), &buf[TYPE_OFFSET], TYPE_LENGTH);
    strcpy(dateDesc->buff, &buf[CONTENT_OFFSET]);

    return 0;
}
