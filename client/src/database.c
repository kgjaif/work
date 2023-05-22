#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include "syslog.h"
#include "packet.h"

#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_ERROR 2

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

    sql = "create table if not exists temperature(id INTEGER PRIMARY KEY,port  VARCHAR(10),time VARCHAR(12),temper VARCHAR(20));"; 

    if(SQLITE_OK!=sqlite3_exec(db,sql,0,0,&errmsg))
    {   
        log_write(LOG_LEVEL_ERROR,"error creating table: %s\n",errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return -1; 
    }   
    log_write(LOG_LEVEL_INFO,"Create table temperature successfully\n");
}



int sql_insert_data(sqlite3 *db,char *buf,packet_t pack,size_t buf_length)

{
    char *inst;
    char *errmsg;
    inst=sqlite3_mprintf("INSERT INTO temperature VALUES(NULL,'%s','%s','%f');",pack.devsn,pack.buf_t,pack.temper);
    snprintf(buf,buf_length,"%s\n",inst);
    if(!inst)
    {
        log_write(LOG_LEVEL_ERROR,"insert error%s\n",errmsg);
        return 0;
    }

    if(SQLITE_OK!= sqlite3_exec(db,inst,0,0,&errmsg))
    {  
        log_write(LOG_LEVEL_ERROR,"error Insert data: %s\n",errmsg);
        sqlite3_free(errmsg);
        sqlite3_free(inst);
        sqlite3_close(db);
        return -1;
    }
}



int sql_select_data(sqlite3 *db)
{
    char *slc;
    char *errmsg;

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
    char *errmsg;
    char *rt="delete from temperature where id in(SELECT id FROM 'temperature' where id>=1);"; 
    if(sqlite3_exec(db,rt,0,0,&errmsg)!=SQLITE_OK)
    {
        log_write(LOG_LEVEL_ERROR,"error delete data: %s\n",errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return -1;
    }
}
