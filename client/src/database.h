#ifndef _DATABASE_
#define _DATABASE_

#include <sqlite3.h>
#include "syslog.h"
#include "packet.h"

extern int open_database(sqlite3 **db);

extern int create_table(sqlite3 *db);

extern int insert_data(sqlite3 *db,packet_t pack);

extern int select_data(sqlite3 *db);

extern int sql_delete_data(sqlite3 *db);

#endif
