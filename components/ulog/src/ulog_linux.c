/**
 * @copyright Copyright (C) 2015-2019 Alibaba Group Holding Limited
 */
#ifdef __linux__

#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <stdarg.h>

#include <ulog/ulog.h>

static char       serverity_name[8] = {'V', 'A', 'F', 'E', 'W', 'T', 'I', 'D'};
static const char months[][4]       = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                                 "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

static unsigned char g_log_level = LOG_DEBUG;
int                  aos_set_log_level(aos_log_level_t log_level)
{
    g_log_level = log_level;
}

int ulog(const unsigned char lvl, const char *mod, const char *f, const unsigned long l,
         const char *fmt, ...)
{
    if (lvl > g_log_level) {
        return 0;
    }

    if (access("/etc/syslog.yoc", F_OK) != 0) {
        openlog(mod, LOG_CONS | LOG_PID, LOG_LOCAL0);
        va_list args;
        va_start(args, fmt);
        vsyslog(lvl, fmt, args);
        va_end(args);
        closelog();
    } else {
        time_t rawtime;
        time(&rawtime);
        char buffer[32];

        struct tm *tm = localtime(&rawtime);
        /* %b format of strftime() is platform-dependent, so we realized it by-self */
        snprintf(buffer, sizeof(buffer), "%s ", months[tm->tm_mon < 12 ? tm->tm_mon : 0]);
        strftime(&buffer[4], sizeof(buffer) - 4, "%d %H:%M:%S", tm);

        struct timeval  tv;
        struct timezone tz;
        gettimeofday(&tv, &tz);

        const int milli     = tv.tv_usec;
        char      ms_str[8] = "";

        memset(ms_str, 0, sizeof(ms_str));
        snprintf(ms_str, sizeof(ms_str), ".%06d", milli);
        strncat(buffer, ms_str, sizeof(buffer) - strlen(buffer) - 1);

        printf("[%s][%c]<%8s> ", buffer, serverity_name[lvl], mod);

        va_list args;
        va_start(args, fmt);
        vprintf(fmt, args);
        va_end(args);
        fflush(stdout);
        printf("\r\n");
    }

    return 0;
}

void ulog_init(void)
{
    return;
}

int ulog_man(const char *cmd_str)
{
    return 0;
}

void ulog_flush()
{
    return;
}

int aos_get_ulog_list(char *buf, const unsigned short len)
{
    return 0;
}

int aos_log_hexdump(const char* tag, char *buffer, int len)
{
    return 0;
}
#endif
