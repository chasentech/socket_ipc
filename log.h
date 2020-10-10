/* This is sample log system
 * histort:
 *      2020.07.15 base function
 */

#ifndef _LOG_H_
#define _LOG_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define NONE      "\e[0m"
#define BLACK     "\e[0;30m"
#define L_BLACK   "\e[1;30m"
#define RED       "\e[0;31m"
#define L_RED     "\e[1;31m"
#define GREEN     "\e[0;32m"
#define L_GREEN   "\e[1;32m"
#define BROWN     "\e[0;33m"
#define YELLOW    "\e[1;33m"
#define BLUE      "\e[0;34m"
#define L_BLUE    "\e[1;34m"
#define PURPLE    "\e[0;35m"
#define L_PURPLE  "\e[1;35m"
#define CYAN      "\e[0;36m"
#define L_CYAN    "\e[1;36m"
#define GRAY      "\e[0;37m"
#define WHITE     "\e[1;37m"

#define BOLD      "\e[1m"
#define UNDERLINE "\e[4m"
#define BLINK     "\e[5m"
#define REVERSE   "\e[7m"
#define HIDE      "\e[8m"
#define CLEAR     "\e[2J"
#define CLRLINE   "\r\e[K" //or "\e[1K\r"

enum LOG_LEVEL {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,
    LOG_LEVEL_MAX
};

extern enum LOG_LEVEL g_log_level;
extern char g_file_name[256];
extern FILE *g_fp;

extern time_t now;
extern struct tm *tm_now;

#define LOG_PRINT(fmt, args...) \
    if (g_log_level >= LOG_LEVEL_ERROR) { \
        time(&now); tm_now = localtime(&now);\
        printf("[%d-%2d-%2d %2d:%2d:%2d] ", tm_now->tm_year+1900, tm_now->tm_mon+1,  \
             tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec); \
        printf(WHITE fmt NONE, ##args);\
    } \
    if (strncmp(g_file_name, "nofile", strlen("nofile")) != 0) { \
        fprintf(g_fp, "[%d-%2d-%2d %2d:%2d:%2d] ", tm_now->tm_year+1900, tm_now->tm_mon+1,  \
             tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);  \
        fprintf(g_fp, fmt, ##args);\
    }

#define LOG_PERROR(fmt, args...) \
    if (g_log_level >= LOG_LEVEL_ERROR) { \
        printf("[%d-%2d-%2d %2d:%2d:%2d] ", tm_now->tm_year+1900, tm_now->tm_mon+1,  \
             tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec); \
        printf(L_RED "[PERROR] [%d] [%s] " NONE fmt ": %s\n", __LINE__, __FILE__, ##args, strerror(errno));\
    } \
    if (strncmp(g_file_name, "nofile", strlen("nofile")) != 0) { \
        fprintf(g_fp, "[%d-%2d-%2d %2d:%2d:%2d] ", tm_now->tm_year+1900, tm_now->tm_mon+1,  \
             tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);  \
        fprintf(g_fp, "[PERROR] [%d] [%s] " fmt ": %s\n", __LINE__, __FILE__, ##args, strerror(errno));\
    }

#define LOG_ERROR(fmt, args...) \
    if (g_log_level >= LOG_LEVEL_ERROR) { \
        printf("[%d-%2d-%2d %2d:%2d:%2d] ", tm_now->tm_year+1900, tm_now->tm_mon+1,  \
             tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec); \
        printf(L_RED "[ERROR]  [%d] [%s] " NONE fmt, __LINE__, __FILE__, ##args);\
    } \
    if (strncmp(g_file_name, "nofile", strlen("nofile")) != 0) { \
        fprintf(g_fp, "[%d-%2d-%2d %2d:%2d:%2d] ", tm_now->tm_year+1900, tm_now->tm_mon+1,  \
             tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);  \
        fprintf(g_fp, "[ERROR]  [%d] [%s] " fmt, __LINE__, __FILE__, ##args);\
    }

#define LOG_WARN(fmt, args...) \
    if (g_log_level >= LOG_LEVEL_WARN) { \
        printf("[%d-%2d-%2d %2d:%2d:%2d] ", tm_now->tm_year+1900, tm_now->tm_mon+1,  \
             tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec); \
        printf(YELLOW "[WARN]   [%d] [%s] " NONE fmt, __LINE__, __FILE__, ##args);\
    } \
    if (strncmp(g_file_name, "nofile", strlen("nofile")) != 0) { \
        fprintf(g_fp, "[%d-%2d-%2d %2d:%2d:%2d] ", tm_now->tm_year+1900, tm_now->tm_mon+1,  \
             tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);  \
        fprintf(g_fp, "[WARN]   [%d] [%s] " fmt, __LINE__, __FILE__, ##args);\
    }

#define LOG_INFO(fmt, args...) \
    if (g_log_level >= LOG_LEVEL_INFO) { \
        printf("[%d-%2d-%2d %2d:%2d:%2d] ", tm_now->tm_year+1900, tm_now->tm_mon+1,  \
             tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec); \
        printf(L_GREEN "[INFO]   [%d] [%s] " NONE fmt, __LINE__, __FILE__, ##args);\
    } \
    if (strncmp(g_file_name, "nofile", strlen("nofile")) != 0) { \
        fprintf(g_fp, "[%d-%2d-%2d %2d:%2d:%2d] ", tm_now->tm_year+1900, tm_now->tm_mon+1,  \
             tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);  \
        fprintf(g_fp, "[INFO]   [%d] [%s] " fmt, __LINE__, __FILE__, ##args);\
    }

#define LOG_DEBUG(fmt, args...) \
    if (g_log_level >= LOG_LEVEL_DEBUG) { \
        printf("[%d-%2d-%2d %2d:%2d:%2d] ", tm_now->tm_year+1900, tm_now->tm_mon+1,  \
             tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec); \
        printf(WHITE "[DEBUG]  [%d] [%s] " NONE fmt, __LINE__, __FILE__, ##args);\
    } \
    if (strncmp(g_file_name, "nofile", strlen("nofile")) != 0) { \
        fprintf(g_fp, "[%d-%2d-%2d %2d:%2d:%2d] ", tm_now->tm_year+1900, tm_now->tm_mon+1,  \
             tm_now->tm_mday, tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec);  \
        fprintf(g_fp, "[DEBUG]  [%d] [%s] " fmt, __LINE__, __FILE__, ##args);\
    }

void log_init();
void log_deinit();

void test_log();

#endif
