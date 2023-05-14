
#include <stdio.h>
#include <unistd.h> 
#include <stdlib.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <getopt.h>
#include <sqlite3.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <libgen.h>
#include <stdarg.h>
//#include "syslog.h"

int g_stop=0;

char *errmsg=NULL;

static inline void print_usage(char *program);

void set_stop(int signum);

int sql_open_database(sqlite3 **db);

int sql_create_table(sqlite3 *db);

int sql_insert_data(sqlite3 *db,char *buf,int port,char* buf_t,float temper,size_t rt_length);

int sql_select_data(sqlite3 *db);

int sql_delete_data(sqlite3 *db);

int socket_check(int *sockfd,char *servip,int *port);

float get_temper(float *temper);

int  get_date_time(char *buf_t);

void log_init(const char *config_file);

void log_write(int level,const char *fmt,...);

 
#define LOG_FILE "syslog.txt" 
#define MAX_LOG_FILE_SIZE (1024 * 1024) 
#define MAX_LOG_FILE_NUM 10 
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_ERROR 2
#define LOG_LEVEL_ALERT 3

#define MAX_RETRY_TIMES 5
#define RETRY_INTERVAL 3 

static FILE *log_file = NULL;
static int log_level = LOG_LEVEL_INFO;

int callback(void *Notused,int argc,char **argv, char **azColName)
{
	int         i;

	for(i=0; i<argc; i++)
	{
		printf("%s = %s\n",azColName[i],argv[i] ? argv[i] : "NULL");
	}
	printf("\n");

	return 0;
}


int main(int argc, char **argv)

{
	int                      ch;
	int                     rv=-1;
	int                     sockfd=-1;
	float                   temper;
	struct sockaddr_in      servaddr;
	char                    *servip=NULL;
	int                     port;
	char                    buf[512];
	char                    buf_tmp[512];
	char                   *domain=NULL;
	char                    ipstr[INET_ADDRSTRLEN];
	char                    buf_t[64];
	char                   *program=NULL;
	char                   *slc;
	struct addrinfo         hints,*res,*p;
	struct option           opts[]={
		{"ipaddr",required_argument,NULL,'i'},
		{"port",required_argument,NULL,'p'},
		{"doamin",required_argument,NULL,'i'},
		{"Help",no_argument,NULL,'h'},
		{NULL,0,NULL,0}
	};

	sqlite3 *db;
	program=basename(argv[0]);

	log_init("syslog.txt");

	//opensqlite
	/*
	   int rc=sqlite3_open("temperature.db",&db);
	   if(rc!=SQLITE_OK)
	   {
	   log_write(LOG_LEVEL_ALERT,"cannot open database:%s\n",sqlite3_errmsg(db));
	   sqlite3_close(db);
	   return -1;
	   }

	   log_write(LOG_LEVEL_INFO,"Open sqlite3 successfully\n");
	   */
	sql_open_database(&db);

	sql_create_table(db);
	//desvn="RPI00001";


	while((ch=getopt_long(argc,argv,"i:p:d:h",opts,NULL))!=-1)
	{
		switch(ch)
		{

			case 'i':
				servip=optarg;
				break;
			case 'p':
				port=atoi(optarg);
				break;
			case 'd':
				domain=optarg;
				break;
			case 'h':
				print_usage(argv[0]);
			default:
				log_write(LOG_LEVEL_ERROR,"Unknown return val: %d\n",ch);
		}
	}

	if(!port|!(!servip^!domain))
	{
		print_usage(argv[0]);
		return 0;
	}

	memset(&hints,0,sizeof(hints));
	hints.ai_flags-=AI_PASSIVE;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_family=AF_INET;
	hints.ai_protocol=0;

	if(!servip)
	{
		rv=getaddrinfo(argv[1],NULL,&hints,&res);
		if(rv!=0)
		{
			log_write(LOG_LEVEL_ERROR,"Analyze failure:%s\n",strerror(errno));
			return -1;
		}
		log_write(LOG_LEVEL_INFO,"Analyze successfully\n");

		for(p=res;p!=NULL;p=p->ai_next)
		{
			servaddr=*((struct sockaddr_in*)p->ai_addr);
			inet_ntop(AF_INET,&(servaddr.sin_addr),ipstr,sizeof(ipstr));
			servip=ipstr;
		}
		freeaddrinfo(res);
		return 0;
	}

	sockfd=socket(AF_INET,SOCK_STREAM,0);

	if(sockfd<0)
	{
		log_write(LOG_LEVEL_ERROR,"create socket failure:%s\n",strerror(errno));
		return -1;
	}

	log_write(LOG_LEVEL_INFO,"create socket[%d] successfully\n",sockfd);

	memset(&servaddr,0,sizeof(servaddr));

	servaddr.sin_family=AF_INET;

	servaddr.sin_port=htons(port);

	inet_aton(servip,&servaddr.sin_addr);

	rv= connect(sockfd,(struct sockaddr *)&servaddr,sizeof(struct sockaddr_in));
		if(rv<0)
		{
		log_write(LOG_LEVEL_ERROR,"connect to server[%s:%d] failure:%s\n",servip,port,strerror(errno));
		//socket_check(&sockfd,servip,&port);
		return -2;
		}

	get_temper(&temper);

	get_date_time(buf_t);

	signal(SIGINT,set_stop);

	memset(buf,0,sizeof(buf));

	snprintf(buf,sizeof(buf),"%d $ %s $ %f",port,buf_t,temper);

	write(sockfd,buf,strlen(buf));

	time_t last_time=time(NULL);

	while(!g_stop)
	{
		sleep(10);
		sql_insert_data(db,buf,port,buf_t,temper,sizeof(buf));
		memset(buf,0,sizeof(buf));
		snprintf(buf,sizeof(buf),"%d $ %s $ %f",port,buf_t,temper);
		rv=write(sockfd,buf,strlen(buf));
		if(rv<0)
		{
			log_write(LOG_LEVEL_ERROR,"write to server by sockfd[%d] failure:%s\n",sockfd,strerror(errno));
			return -3;

		}
		time_t cur_time=time(NULL);

		if(cur_time - last_time>=10)
		{

			write(sockfd,buf,strlen(buf));
		}
		printf("curtime:%d\n",cur_time-last_time);
		last_time=cur_time;

		memset(buf,0,sizeof(buf));

		rv=read(sockfd,buf,sizeof(buf));
		if(rv<0)
		{
			log_write(LOG_LEVEL_ERROR,"read data from server by sockfd[%d] failure:%s\n",sockfd,strerror(errno));
			return -4;
		}
		else if(rv==0)
		{
			log_write(LOG_LEVEL_INFO,"socket[%d] get disconnected\n",sockfd);
		}


		else if(rv>0)
		{
			//sql_delete_data(db);
			log_write(LOG_LEVEL_INFO,"read %d bytes data from server:%s\n",rv,buf);
			continue;
		}


		sql_select_data(db);

		char **azResult;
		int ncolumn,nrow;
		int index=ncolumn;
		if(sqlite3_get_table(db,slc,&azResult,&nrow,&ncolumn,&errmsg)!=SQLITE_OK)
		{
			printf("error select data: %s\n",errmsg);
			sqlite3_free(errmsg);
			sqlite3_close(db);
			return -1;
		}
		printf("===============\n");
		for(int i=0;i<nrow;i++)
		{
			for(int j=0;j<ncolumn;j++)
			{
				printf("%s---%s\n",azResult[j],azResult[index]);
				index ++;
			}
		}
		sqlite3_free_table(azResult);

	}
	sqlite3_close(db);
	close(sockfd);
	return 0;
}


static inline void print_usage(char *program)
{
	printf("%s usage:\n",program);
	printf("-i(--ipaddr):sepcify server IP address\n");
	printf("-p(--port):sepcify server port\n");
	printf("-d(--domain):sepcify server domain\n");
	printf("-h(--help):print the help information\n");
	return ;
}


void set_stop(int signum)
{
	if(signum==SIGINT)
	{
		printf("exit\n");
	}
	g_stop=1;
}


float get_temper(float *temper)
{
	DIR                             *dirp=NULL;
	struct  dirent  *direntp=NULL;
	char                    path_sn[32];
	char                    ds18b20_path[64];
	int             found=0;
	int             fd=-1;
	char            buf[128];
	char            *ptr=NULL;
	char            w1_path[128]="/sys/bus/w1/devices/";

	dirp=opendir(w1_path);
	if(!dirp)
	{
		printf("open the folser %s failure:%s\n",w1_path,strerror(errno));
		return -6;
	}

	while((direntp=readdir(dirp))!=NULL)
	{
		if(strcmp(direntp->d_name,".")==strcmp(direntp->d_name,"..")==0)
		{
			continue;
			printf("%s\n",direntp->d_name);
		}
		if(strstr(direntp->d_name,"28-"))
		{
			strncpy(path_sn,direntp->d_name,sizeof(path_sn));
		}
		found=1;
	}

	closedir(dirp);

	if(!found)
	{
		printf("Can Not found ds18b20 chipset\n");
		return -7;
	}

	strncat(w1_path,path_sn,sizeof(w1_path)-strlen(w1_path));
	strncat(w1_path,"/w1_slave",sizeof(w1_path)-strlen(w1_path));

	if((fd=open(w1_path,O_RDONLY))<0)
	{

		printf("Open the file failure:%s\n",strerror(errno));
		return -8;
	}

	memset(buf,0,sizeof(buf));
	if(read(fd,buf,sizeof(buf))<0)
	{
		printf("read data from fd =%d failure:%s\n",fd,strerror(errno));
		return -9;
	}

	ptr=strstr(buf,"t=");
	if(!ptr)
	{
		printf("Can Not find t=???\n");
		return -1;
	}
	ptr+=2;
	*temper=atof(ptr)/1000;

	write(fd,buf,sizeof(buf));

	close(fd);

	return 0;
}

int get_date_time(char *buf_t)
{
	struct tm *tm;
	struct tm result;
	time_t t;  
	t=time(NULL);
	tm = localtime_r(&t,&result);
	sprintf(buf_t,"%04d-%02d-%02d  %02d:%02d:%02d",
			tm->tm_year+1900, 
			tm->tm_mon+1,
			tm->tm_mday,
			tm->tm_hour,
			tm->tm_min,
			tm->tm_sec);

}


int sql_open_database(sqlite3 **db)
{

	char *errmsg;
	if( sqlite3_open("temperature.db",db)!=SQLITE_OK)
	{
		log_write(LOG_LEVEL_ERROR,"cannot open database:%s\n",sqlite3_errmsg((sqlite3 *)db));
		sqlite3_close((sqlite3 *)db);
		return -1;
	}

	log_write(LOG_LEVEL_INFO,"Open sqlite3 successfully\n");
}


int sql_create_table(sqlite3 *db)
{
	char   *errmsg;
	char   *sql;

	sql = "create table if not exists temperature(id INTEGER PRIMARY KEY,port VARCHAR(10),time VARCHAR(12),temper VARCHAR(20));";

	if(SQLITE_OK!=sqlite3_exec(db,sql,0,0,&errmsg))
	{
		log_write(LOG_LEVEL_ERROR,"error creating table: %s\n",errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return -1;
	}
	log_write(LOG_LEVEL_INFO,"Create table temperature successfully\n");
}


int sql_insert_data(sqlite3 *db,char *buf,int port,char* buf_t,float temper,size_t buf_length)

{
	char *tr;
	tr=sqlite3_mprintf("INSERT INTO temperature VALUES(NULL,'%d','%s','%f');",port,buf_t,temper);
	snprintf(buf,buf_length,"%s\n",tr);
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

int sql_select_data(sqlite3 *db)
{
	char *slc;

	slc="SELECT * FROM temperature";

	if(sqlite3_exec(db,slc,callback,0,&errmsg)!=SQLITE_OK)
	{
		log_write(LOG_LEVEL_ERROR,"error select data: %s\n",errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return -1;
	}
}


int sql_delete_data(sqlite3 *db)
{
	char *rt="delete from temperature where id in(SELECT id FROM 'temperature'where id>=1);";
	if(sqlite3_exec(db,rt,0,0,&errmsg)!=SQLITE_OK)
	{
		log_write(LOG_LEVEL_ERROR,"error delete data: %s\n",errmsg);
		sqlite3_free(errmsg);
		sqlite3_close(db);
		return -1;
	}
}


void log_write(int level, const char *fmt, ...)
{
	time_t t;
	struct tm *tm;
	struct tm result;
	va_list ap;
	char buf[1024];

	if (level < log_level) return;

	t = time(NULL);
	tm = localtime_r(&t,&result);

	switch (level) {
		case LOG_LEVEL_DEBUG:
			strcpy(buf, "[DEBUG]");
			break;
		case LOG_LEVEL_INFO:
			strcpy(buf, "[INFO]");
			break;
		case LOG_LEVEL_ERROR:
			strcpy(buf, "[ERROR]");
			break;
		case LOG_LEVEL_ALERT:
			strcpy(buf,"[ALERT]");
			break;
	}

	strftime(buf + strlen(buf), sizeof(buf) - strlen(buf), "%Y-%m-%d %H:%M:%S", tm);
	sprintf(buf + strlen(buf), " [PID:%d]", getpid());

	va_start(ap, fmt);
	vsnprintf(buf + strlen(buf), sizeof(buf) - strlen(buf), fmt, ap);
	va_end(ap);

	printf("%s\n", buf);

	if (log_file == NULL)
	{
		log_file = fopen(LOG_FILE, "a+");
	}
	if (log_file != NULL) 
	{
		fseek(log_file, 0, SEEK_END);

		if (ftell(log_file) > MAX_LOG_FILE_SIZE)
		{
			fclose(log_file);

			for (int i = MAX_LOG_FILE_NUM - 1; i >= 1; i--)
			{
				char old_file_name[256];
				char new_file_name[256];

				if (i == MAX_LOG_FILE_NUM - 1)
				{
					snprintf(old_file_name, sizeof(old_file_name), "%s", LOG_FILE);
				} 
				else
				{
					snprintf(old_file_name, sizeof(old_file_name), "%s.%d", LOG_FILE, i);
				}
				snprintf(new_file_name, sizeof(new_file_name), "%s.%d", LOG_FILE, i + 1);
				rename(old_file_name, new_file_name);
			}

			rename(LOG_FILE, LOG_FILE ".1");
			log_file = fopen(LOG_FILE, "a+");
		}

		if (log_file != NULL)
		{
			fprintf(log_file, "%s\n", buf);
			fflush(log_file);
		}
	}
}


void log_init(const char *config_file)
{

	log_level = LOG_LEVEL_DEBUG;
}


/* 
int socket_check(int *sockfd,char *servip,int *port)
{

	close(sockfd);
	sockfd=socket(AF_INET,SOCK_STREAM,0);

	struct sockaddr_in servaddr;

	memset(&servaddr,0,sizeof(servaddr));

	servaddr.sin_family=AF_INET;

	servaddr.sin_port=htons(port);

	inet_aton(servip,&servaddr.sin_addr);

	connect(sockfd,(struct sockaddr *)&servaddr,sizeof(struct
				sockaddr_in));

	int error=0;
	socklen_t length=sizeof(error);
	int ret=getsockopt(sockfd,SOL_SOCKET,SO_ERROR,&error,&length);
	if(ret<0||error!=0)
	{
		log_write(LOG_LEVEL_ERROR,"connection failed\n");
	}
	else
	{
		log_write(LOG_LEVEL_INFO,"connection succeeded\n");
	}
}
*/
