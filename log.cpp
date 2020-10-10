#include <stdlib.h>
#include <unistd.h>
#include "log.h"

enum LOG_LEVEL g_log_level = LOG_LEVEL_ERROR;
char g_file_name[256] = "nofile";
FILE *g_fp = NULL;

time_t now;
struct tm *tm_now;

int printf_color(void)
{
    printf("This is a character control test!\n" );
    sleep(3);
    printf("[%2u]" CLEAR "CLEAR\n" NONE, __LINE__);

    printf("[%2u]" BLACK "BLACK " L_BLACK "L_BLACK\n" NONE, __LINE__);
    printf("[%2u]" RED "RED " L_RED "L_RED\n" NONE, __LINE__);
    printf("[%2u]" GREEN "GREEN " L_GREEN "L_GREEN\n" NONE, __LINE__);
    printf("[%2u]" BROWN "BROWN " YELLOW "YELLOW\n" NONE, __LINE__);
    printf("[%2u]" BLUE "BLUE " L_BLUE "L_BLUE\n" NONE, __LINE__);
    printf("[%2u]" PURPLE "PURPLE " L_PURPLE "L_PURPLE\n" NONE, __LINE__);
    printf("[%2u]" CYAN "CYAN " L_CYAN "L_CYAN\n" NONE, __LINE__);
    printf("[%2u]" GRAY "GRAY " WHITE "WHITE\n" NONE, __LINE__);

    printf("[%2u]\e[1;31;40m Red \e[0m\n",  __LINE__);

    printf("[%2u]" BOLD "BOLD\n" NONE, __LINE__);
    printf("[%2u]" UNDERLINE "UNDERLINE\n" NONE, __LINE__);
    printf("[%2u]" BLINK "BLINK\n" NONE, __LINE__);
    printf("[%2u]" REVERSE "REVERSE\n" NONE, __LINE__);
    printf("[%2u]" HIDE "HIDE\n" NONE, __LINE__);

    printf("Cursor test begins!\n" );
    printf("=======!\n" );
    sleep(10);
    printf("[%2u]" "\e[2ACursor up 2 lines\n" NONE, __LINE__);
    sleep(10);
    printf("[%2u]" "\e[2BCursor down 2 lines\n" NONE, __LINE__);
    sleep(5);
    printf("[%2u]" "\e[?25lCursor hide\n" NONE, __LINE__);
    sleep(5);
    printf("[%2u]" "\e[?25hCursor display\n" NONE, __LINE__);
    sleep(5);

    printf("Test ends!\n" );
    sleep(3);
    printf("[%2u]" "\e[2ACursor up 2 lines\n" NONE, __LINE__);
    sleep(5);
    printf("[%2u]" "\e[KClear from cursor downward\n" NONE, __LINE__);

    return 0;
}

void log_init()
{
	//print = PERROE = ERROE < WARN < INFO < DEBUG

    const char *env = getenv("LOG_LEVEL");
    // printf("env = %s\n", env);
    if (env != NULL && strlen(env) != 0) {
        if (strcmp(env, "error") == 0)
            g_log_level = LOG_LEVEL_ERROR;
        else if (strcmp(env, "warn") == 0)
            g_log_level = LOG_LEVEL_WARN;
        else if (strcmp(env, "info") == 0)
            g_log_level = LOG_LEVEL_INFO;
        else if (strcmp(env, "debug") == 0)
            g_log_level = LOG_LEVEL_DEBUG;
        else g_log_level = LOG_LEVEL_ERROR;
    }

    //strcpy(g_file_name, "./log.txt");
    env = getenv("LOG_FILE");
    // printf("env = %s\n", env);
    if (env != NULL && strlen(env) != 0) {
        if (strcmp(env, "nofile") != 0) {
            strcpy(g_file_name, env);
            g_fp = fopen(g_file_name, "a");
            if (g_fp == NULL) {
                perror("fopen failed");
                strcpy(g_file_name, "nofile");
            }
        }
    }
}

void log_deinit()
{
    if (g_fp != NULL) {
        fclose(g_fp);
    }
}

void test_log()
{
	log_init();
    int a = 20;

	LOG_PRINT("test LOG_PRINTF %d\n", a);
	LOG_PERROR("test LOG_PERROR");
	LOG_ERROR("test LOG_ERROR %d\n", a);
	LOG_WARN("test LOG_WARN %d\n", a);
	LOG_INFO("test LOG_INFO %d\n", a);
	LOG_DEBUG("test LOG_DEBUG %d\n", a);

    log_deinit();
}
