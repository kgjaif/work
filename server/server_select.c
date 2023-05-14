/*********************************************************************************
 *      Copyright:  (C) 2022 iot<iot@email.com>
 *                  All rights reserved.
 *
 *       Filename:  server_select.c
 *    Description:  This file 
 *                 
 *        Version:  1.0.0(11/18/22)
 *         Author:  iot <iot@email.com>
 *      ChangeLog:  1, Release initial version on "11/18/22 10:32:55"
 *                 
 ********************************************************************************/
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>
#include <getopt.h>
#include <libgen.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <dirent.h>
#include <stdarg.h>


#define ARRAY_SIZE(x)		(sizeof(x)/sizeof(x[0]))
#define LOG_FILE "syslog.txt" 
#define MAX_LOG_FILE_SIZE (512 * 512) 
#define MAX_LOG_FILE_NUM 10 
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_ERROR 2
#define LOG_LEVEL_ALERT 3

static inline void msleep(unsigned long ms);
static inline void print_usage(char *progname);

static FILE *log_file = NULL;
static int log_level = LOG_LEVEL_INFO;

int socket_server_init(char *listen_ip,int listen_port);
int sql_open_database(sqlite3 **db);
int sql_create_table(sqlite3 *db);
int sql_insert_data(sqlite3 *db,char *buf);

int split_strtok(char *buf,size_t buf_size);

void log_init(const char *config_file);

void log_write(int level,const char *fmt,...);

int main(int argc,char **argv)
{
	int				listenfd,connfd;
	int				serv_port=0;
	int				daemon_run=0;
	char			*progname=NULL;
	int				opt;
	fd_set			rdset;
	int				rv;
	int				i,j;
	int				found;
	int				maxfd=0;
	char			buf[1024];
	int				fds_array[1024];
	struct option	long_options[]=
	{
		{"daemon",no_argument,NULL,'b'},
		{"port",required_argument,NULL,'p'},
		{"help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};

	sqlite3 *db;
	progname=basename(argv[0]);

	sql_open_database(&db);

	sql_create_table(db);


	while((opt=getopt_long(argc,argv,"bp:h",long_options,NULL))!=-1)
	{
		switch(opt)
		{
			case 'b':
				daemon_run=1;
				break;
			case 'p':
				serv_port=atoi(optarg);
				break;
			case 'h':
				print_usage(progname);
				return EXIT_SUCCESS;
			default:
				break;
		}
	}
	if( !serv_port )
	{ 
		print_usage(progname);
		return -1;
	}
	if( (listenfd=socket_server_init(NULL, serv_port)) < 0 )
	{
		log_write(LOG_LEVEL_ERROR,"ERROR: %s server listen on port %d failure\n", argv[0],serv_port);
		return -2;
	}
	log_write(LOG_LEVEL_INFO,"%s server start to listen on port %d\n", argv[0],serv_port);

	if( daemon_run )

	{
		daemon(0, 0);
	}
	for(i=0; i<ARRAY_SIZE(fds_array) ; i++)
	{
		fds_array[i]=-1;
	}
	fds_array[0] = listenfd;

	for ( ; ; )
	{
		FD_ZERO(&rdset);
		for(i=0; i<ARRAY_SIZE(fds_array) ; i++)
		{
			if( fds_array[i] < 0 )
				continue;
			maxfd = fds_array[i]>maxfd ? fds_array[i] : maxfd;
			FD_SET(fds_array[i], &rdset);
		}
		rv = select(maxfd+1, &rdset, NULL, NULL, NULL);
		if(rv < 0)
		{
			log_write(LOG_LEVEL_ERROR,"select failure: %s\n", strerror(errno));
			break;
		}
		else if(rv == 0)
		{
			log_write(LOG_LEVEL_ERROR,"select get timeout\n");
			continue;
		}
		if ( FD_ISSET(listenfd, &rdset) )
		{
			if( (connfd=accept(listenfd, (struct sockaddr *)NULL, NULL)) < 0)
			{
				log_write(LOG_LEVEL_ERROR,"accept new client failure: %s\n", strerror(errno));
				continue;
			}
			found = 0;
			for(i=0; i<ARRAY_SIZE(fds_array) ; i++)
			{
				if( fds_array[i] < 0 )
				{
					log_write(LOG_LEVEL_INFO,"accept new client[%d] and add it into array\n", connfd );
					fds_array[i] = connfd;
					found = 1;
					break;
				}
			}
			if( !found )
			{
				log_write(LOG_LEVEL_INFO,"accept new client[%d] but full, so refuse it\n", connfd);
				close(connfd);
			}
		}
		else
		{
			for(i=0; i<ARRAY_SIZE(fds_array); i++)
			{
				if( fds_array[i]<0 || !FD_ISSET(fds_array[i], &rdset) )
					continue;
				if( (rv=read(fds_array[i], buf, sizeof(buf))) <= 0)
				{
					log_write(LOG_LEVEL_ERROR,"socket[%d] read failure or get disconncet.\n", fds_array[i]);
					close(fds_array[i]);
					fds_array[i] = -1;
				}
				else
				{

					log_write(LOG_LEVEL_INFO,"socket[%d] read get %d bytes data\n", fds_array[i], rv);

					for(j=0; j<rv; j++)
					{
						buf[j]=toupper(buf[j]);
					}
					write(fds_array[i], buf, rv);
					split_strtok(buf,sizeof(buf));

					sql_insert_data(db,buf);
					if( write(fds_array[i], buf, rv) < 0 )
					{
						printf("socket[%d] write failure: %s\n", fds_array[i], strerror(errno));
						close(fds_array[i]);
						fds_array[i] = -1;
					}
				}
			}
		}
	}

CleanUp:
	close(listenfd);
	return 0;
}
static inline void msleep(unsigned long ms)
{
	struct timeval tv;
	tv.tv_sec = ms/1000;
	tv.tv_usec = (ms%1000)*1000;

	select(0, NULL, NULL, NULL, &tv);
}
static inline void print_usage(char *progname)
{
	printf("Usage: %s [OPTION]...\n", progname);

	printf(" %s is a socket server program, which used to verify client and echo back string from it\n",
			progname);
	printf("\nMandatory arguments to long options are mandatory for short options too:\n");

	printf(" -b[daemon ] set program running on background\n");
	printf(" -p[port ] Socket server port address\n");
	printf(" -h[help ] Display this help information\n");

	printf("\nExample: %s -b -p 8900\n", progname);
	return ;
}
int socket_server_init(char *listen_ip, int listen_port)
{
	struct sockaddr_in servaddr;
	int rv = 0;
	int on = 1;
	int listenfd;
	if ( (listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		printf("Use socket() to create a TCP socket failure: %s\n", strerror(errno));
		return -1;


	}

	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
	memset(&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family = AF_INET; 
	servaddr.sin_port = htons(listen_port);
	if( !listen_ip ) /*  Listen all the local IP address */
	{
		servaddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	}
	else
	{
		if (inet_pton(AF_INET, listen_ip, &servaddr.sin_addr) <= 0)
		{
			printf("inet_pton() set listen IP address failure.\n");
			rv = -2;
			goto CleanUp;
		}
	}
	if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0)
	{
		printf("Use bind() to bind the TCP socket failure: %s\n", strerror(errno));
		rv = -3;
		goto CleanUp;
	}
	if(listen(listenfd, 13) < 0)
	{
		printf("Use bind() to bind the TCP socket failure: %s\n", strerror(errno));
		rv = -4;
		goto CleanUp;
	}
CleanUp:
	if(rv<0)
		close(listenfd);
	else
		rv = listenfd;
	return rv;
}

int sql_open_database(sqlite3 **db)
{

	char *errmsg;
	if( sqlite3_open("temperature.db",db)!=SQLITE_OK)
	{   
		log_write(LOG_LEVEL_ERROR,"cannot open database:%s\n",    
				sqlite3_errmsg((sqlite3 *)db));
		sqlite3_close((sqlite3 *)db);
		return -1; 
	}   

	log_write(LOG_LEVEL_INFO,"Open sqlite3 successfully\n");
}


int sql_create_table(sqlite3 *db)
{
	char   *errmsg;
	char   *sql;

	sql = "create table if not exists backuptemp(data VARCHAR(80));";

	if(SQLITE_OK!=sqlite3_exec(db,sql,0,0,&errmsg))
	{   
		log_write(LOG_LEVEL_ERROR,"error creating table: %s\n",    
				errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return -1; 
	}   
	log_write(LOG_LEVEL_INFO,"Create table temperature successfully\n");
}

int sql_insert_data(sqlite3 *db,char *buf)

{
	char *tr;
	char *errmsg;
	tr=sqlite3_mprintf("INSERT INTO backuptemp VALUES('%s');",buf);
	//snprintf(buf,buf_length,"%s\n",tr);
	if(!tr)
	{
		log_write(LOG_LEVEL_ERROR,"insert error%s\n",errmsg);
		return 0;
	}

	if(SQLITE_OK!= sqlite3_exec(db,tr,0,0,&errmsg))
	{
		log_write(LOG_LEVEL_ERROR,"error Insert data: %s\n",errmsg);
		sqlite3_free(errmsg);
		sqlite3_free(tr);
		sqlite3_close(db);
		return -1;
	}
}

int split_strtok(char *buf,size_t buf_size)
{
	char *token;
	char delim='$';
	int  pos=0;
	int  len=0;
	token =strtok(buf,&delim);
	while(token !=NULL)
	{
		len=snprintf(buf+pos,buf_size-pos,"%s",token);
		if(len>=buf_size-pos)
		{
			break;
		}
		pos+=len;
		token=strtok(NULL,&delim);

	}
}






void log_write(int level, const char *fmt, ...) {
	time_t t;
	struct tm *tm;
	struct  tm result;
	va_list ap;
	char buf[1024];

	if (level < log_level) return;

	t = time(NULL);
	tm = localtime_r(&t,&result);

	switch (level) {
		case LOG_LEVEL_DEBUG:
			strcpy(buf, "[DEBUG] ");
			break;
		case LOG_LEVEL_INFO:
			strcpy(buf, "[INFO ] ");
			break;
		case LOG_LEVEL_ERROR:
			strcpy(buf, "[ERROR] ");
			break;
	}
	strftime(buf + strlen(buf), sizeof(buf) - strlen(buf), "%Y-%m-%d %H:%M:%S", tm);
	sprintf(buf + strlen(buf), " [PID:%d]", getpid());

	va_start(ap, fmt);
	vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), fmt, ap);
	va_end(ap);

	printf("%s\n", buf);

	if (log_file == NULL) {
		log_file = fopen(LOG_FILE, "a+");
	}
	if (log_file != NULL) {
		fseek(log_file, 0, SEEK_END);
		if (ftell(log_file) > MAX_LOG_FILE_SIZE) {
			fclose(log_file);
			for (int i = MAX_LOG_FILE_NUM - 1; i >= 1; i--) {
				char old_file_name[256], new_file_name[256];
				if (i == MAX_LOG_FILE_NUM - 1) {
					snprintf(old_file_name, sizeof(old_file_name), "%s", LOG_FILE);
				} else {
					snprintf(old_file_name, sizeof(old_file_name), "%s.%d", LOG_FILE, i);
				}
				snprintf(new_file_name, sizeof(new_file_name), "%s.%d", LOG_FILE, i + 1);
				rename(old_file_name, new_file_name);
			}
			rename(LOG_FILE, LOG_FILE ".1");
			log_file = fopen(LOG_FILE, "a+");
		}
		if (log_file != NULL) {
			fprintf(log_file, "%s\n", buf);
			fflush(log_file);
		}
	}
}


void log_init(const char *config_file) {

	log_level = LOG_LEVEL_DEBUG;
}

