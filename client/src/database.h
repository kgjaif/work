#ifndef _DATABASE_
#define _DATABASE_
extern int sql_open_database(sqlite3 **db);
extern int sql_create_table(sqlite3 *db);
extern int sql_insert_data(sqlite3 *db,char *buf,char *devsn,char* buf_t,float temper,size_t rt_length);
extern int sql_select_data(sqlite3 *db);
extern int sql_delete_data(sqlite3 *db);
#endif
