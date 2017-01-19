
#ifndef __LOG_DEBUG_HERDER__
#define __LOG_DEBUG_HERDER__


#define LOG_FILE "/var/log_file_"

#define LOG_LEVEL_ERROR   (0)
#define LOG_LEVEL_ALERT   (1)
#define LOG_LEVEL_DEBUG   (2)
#define LOG_LEVEL_TRACE   (3)

#define LOG_MSG_MAX_LEN (4*1024)
#define LOG_FILE_MAX_SIZE (128 *1024)

#define LOG_TRACE_STRING "%s:[%d]\n", __FUNCTION__, __LINE__

int  log_init(char *subfix);
void log_msg(int level, char *fmt, ...) ;
void log_close(void);
void print_package(unsigned char *buf, int len);


#define log_error(fmt, arg...) do{log_msg(LOG_LEVEL_ERROR, fmt, ##arg);}while(0)
#define log_alert(fmt, arg...) do{log_msg(LOG_LEVEL_ALERT, fmt, ##arg);}while(0)
#define log_debug(fmt, arg...) do{log_msg(LOG_LEVEL_DEBUG, fmt, ##arg);}while(0)
#define log_trace(fmt, arg...) do{log_msg(LOG_LEVEL_TRACE, fmt, ##arg);}while(0)
#define TRACE                  do{log_msg(LOG_LEVEL_TRACE, LOG_TRACE_STRING);}while(0)

#endif /* __LOG_DEBUG_HERDER__ */

