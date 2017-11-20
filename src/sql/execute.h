#ifndef EXECUTE_H__
#define EXECUTE_H__


#ifdef __cplusplus
extern "C" {
#endif

#include "sql/type_def.h"

void report_sql_error(const char *error_name, const char *msg);
void execute_desc_tables(const char *table_name);
void execute_show_tables();
void execute_create_tb(const table_def *table);
void execute_create_db(const char *db_name);
void execute_drop_db(const char *db_name);
void execute_drop_table(const char *table_name);
void execute_use_db(const char *db_name);
void execute_insert_row(struct insert_argu *stmt);
void execute_sql_eof(void);
void execute_select(struct select_argu *stmt);
void execute_delete(struct delete_argu *stmt);
void execute_update(struct update_argu *stmt);
void execute_drop_idx(struct column_ref *tb_col);
void execute_create_idx(struct column_ref *tb_col);

#ifdef __cplusplus
}
#endif


#endif
