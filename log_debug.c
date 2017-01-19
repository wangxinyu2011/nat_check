
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <pthread.h>
#include <fcntl.h>

#include "log_debug.h"

static int log_counter = 0;
static int log_level = LOG_LEVEL_TRACE;
//static int log_level = LOG_LEVEL_DEBUG;

static int log_stdout = 1;
static int log_fd = -1;
static char path[128] = {0};


pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOG_LOCK pthread_mutex_lock(&log_mutex)
#define LOG_UNLOCK pthread_mutex_unlock(&log_mutex)


int log_init(char *subfix)
{
	sprintf(path, "%s%s", LOG_FILE, subfix);
	printf("log file path : %s \n", path);

	log_fd = open(path, O_RDWR | O_CREAT | O_TRUNC, 0666);

	if(log_fd < 0)
	{	
		printf("open log file %s error.\n", path);
		return -1;
	}

	return 0;
}


void log_msg(int level, char *fmt, ...) 
{

	LOG_LOCK;

	if(log_counter > 1000)
	{
		struct stat st;
		/*check log file size*/
		if(0 == stat(path, &st))
		{
			if(st.st_size > LOG_FILE_MAX_SIZE)
			{
				printf("Log file %s is more than %x, truncate it.\n", path, LOG_FILE_MAX_SIZE);
				close(log_fd);
				log_fd = open(path, O_RDWR | O_CREAT | O_TRUNC);
				log_counter = 0;
			}
		}
	}

	if (level <= log_level  ) 
	{
		va_list args; 
		char msg[LOG_MSG_MAX_LEN] = {0};
		
		va_start(args, fmt);
		vsnprintf(msg , LOG_MSG_MAX_LEN, fmt, args);

		if(log_fd > 0)
		{
			write(log_fd, msg, strlen(msg));
			log_counter++;
		}
		
		if( log_stdout)
			write(log_stdout, msg, strlen(msg));
		va_end(args);
		
	}
	LOG_UNLOCK;
	
}


void log_close()
{
	if(log_fd > 0)
		close(log_fd)	;
}

#if 0
void print_package(unsigned char *buf, int len)
{
	int i = 0;
	log_debug("Begin*************\n");
	for(i = 0; i < len; i++)
	{
		log_debug("%02X ", buf[i]);
		if(0 == (i+1)%16)
			log_debug("\n");
	}

	log_debug("\nEND****************\n");
}
#else
void print_package(unsigned char *buf, int len)
{
}
#endif

