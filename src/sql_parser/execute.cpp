#include <stdio.h>
#include <stdlib.h>
#include "execute.h"

#include "backend/Database.h"
#include "dbms/dbms.h"

void free_column_ref(column_ref *c) {
    if (c->table)
        free(c->table);
    free(c->column);
    free(c);
}

void free_column_list(linked_list *cols) {
    while (cols) {
        column_ref *c = (column_ref *) cols->data;
        free_column_ref(c);
        linked_list *t = cols;
        cols = cols->next;
        free(t);
    }
}

void free_expr_list(linked_list *exprs) {
    while (exprs) {
        expr_node *e = (expr_node *) exprs->data;
        free_expr(e);
        linked_list *t = exprs;
        exprs = exprs->next;
        free(t);
    }

}

void free_values(linked_list *values) {
    while (values) {
        linked_list *exprs = (linked_list *) values->data;
        free_expr_list(exprs);
        linked_list *t = values;
        values = values->next;
        free(t);
    }
}

void free_tables(linked_list *tables) {
    //TODO: handle JOIN
    while (tables) {
        char *table_name = (char *) tables->data;
        free(table_name);
        linked_list *t = tables;
        tables = tables->next;
        free(t);
    }
}

void report_sql_error(const char *error_name, const char *msg) {
    printf("SQL Error[%s]: %s\n", error_name, msg);
}

void execute_desc_tables(const char *table_name) {
    DBMS::getInstance()->descTable(table_name);
    free((void *) table_name);
}

void execute_show_tables() {
    DBMS::getInstance()->listTables();
}

void execute_create_db(const char *db_name) {
    Database db;
    db.create(db_name);
    db.close();
    free((void *) db_name);
}

void execute_create_tb(const table_def *table) {
    DBMS::getInstance()->createTable(table);
    free((void *) table->name);
    column_defs *c = table->columns;
    while (c) {
        column_defs *next = c->next;
        free((void *) c->name);
        free((void *) c);
        c = next;
    }
    linked_list *cons = table->constraints;
    while (cons) {
        linked_list *next = cons->next;
        table_constraint *tc = (table_constraint *) (cons->data);
        free(tc->column_name);
        free_expr_list(tc->values);
        free(tc);
        free(cons);
        cons = next;
    }
}

void execute_drop_db(const char *db_name) {
    DBMS::getInstance()->dropDB(db_name);
    free((void *) db_name);
}

void execute_drop_table(const char *table_name) {
    DBMS::getInstance()->dropTable(table_name);
    free((void *) table_name);
}

void execute_use_db(const char *db_name) {
    DBMS::getInstance()->switchToDB(db_name);
    free((void *) db_name);
}

void execute_insert_row(struct insert_argu *stmt) {
    assert(stmt->table);
    DBMS::getInstance()->insertRow(stmt->table, stmt->columns, stmt->values);
    free_column_list(stmt->columns);
    free_values(stmt->values);
    free((void *) stmt->table);
}

void execute_select(struct select_argu *stmt) {
    DBMS::getInstance()->selectRow(stmt->tables, stmt->column_expr, stmt->where);
    free_tables(stmt->tables);
    free_expr_list(stmt->column_expr);
    if (stmt->where)
        free(stmt->where);
}

void execute_delete(struct delete_argu *stmt) {
    DBMS::getInstance()->deleteRow(stmt->table, stmt->where);
    free(stmt->table);
    if (stmt->where)
        free_expr(stmt->where);
}

void execute_update(struct update_argu *stmt) {
    DBMS::getInstance()->updateRow(stmt->table, stmt->where, stmt->column, stmt->val_expr);
    free(stmt->table);
    if (stmt->where)
        free_expr(stmt->where);
    free_column_ref(stmt->column);
    free_expr(stmt->val_expr);
}

void execute_drop_idx(struct column_ref *tb_col) {
    DBMS::getInstance()->dropIndex(tb_col);
    free_column_ref(tb_col);
}

void execute_create_idx(struct column_ref *tb_col) {
    DBMS::getInstance()->createIndex(tb_col);
    free_column_ref(tb_col);
}

void execute_sql_eof() {
    DBMS::getInstance()->exit();
}
