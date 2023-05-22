#ifndef _DATABASE_
#define _DATABASE_

#include <sqlite3.h>
#include "syslog.h"
#include "packet.h"

extern int sql_open_database(sqlite3 **db);
extern int sql_create_table(sqlite3 *db);
extern int sql_insert_data(sqlite3 *db,char *buf,packet_t pack,size_t rt_length);
extern int sql_select_data(sqlite3 *db);
extern int sql_delete_data(sqlite3 *db);
#endif
