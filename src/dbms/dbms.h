#ifndef __DBMS_H__
#define __DBMS_H__

#include <vector>
#include <functional>
#include <string>
#include <iostream>
#include "record/DB.h"
#include "sql/type_def.h"
#include "sql/expr.h"

class DBMS {
    enum IDX_TYPE {
        IDX_NONE, IDX_LOWWER, IDX_UPPER, IDX_EQUAL
    };
    DB *current;
    std::vector<char *> pendingFree;

    DBMS();

    bool requireDbOpen();

    void printExpName(int err);

    void printExprVal(const ExprVal &val);

    bool convert2Bool(const ExprVal &val);

    ExprVal dbType2ExprType(char *data, ColumnType type);

    bool checkColumnType(ColumnType type, const ExprVal &val);

    char *ExprType2dbType(const ExprVal &val);

    void cacheColumns(Table *tb, int rid);

    void freeCacheColumns();

    IDX_TYPE checkIndexAvai(Table *tb, int *rid_l, int *rid_u, int *col, expr_node *condition);

    int nextWithIndex(Table *tb, IDX_TYPE type, int col, int rid, int rid_u);

    void iterateRecords(linked_list *tables, expr_node *condition, std::function<void(Table *, int)> callback);

    void iterateRecords(Table *tb, expr_node *condition, std::function<void(Table *, int)> callback);

    int isAggregate(const linked_list *column_expr);

    void freeLinkedList(linked_list *t);

public:
    static DBMS *getInstance();

    void exit();

    void switchToDB(const char *name);

    void createTable(const table_def *table);

    void dropDB(const char *db_name);

    void dropTable(const char *table);

    void listTables();

    void selectRow(const linked_list *tables, const linked_list *column_expr, expr_node *condition);

    void updateRow(const char *table, expr_node *condition, column_ref *column, expr_node *eval);

    void deleteRow(const char *table, expr_node *condition);

    void insertRow(const char *table, const linked_list *columns, const linked_list *values);

    void createIndex(column_ref *tb_col);

    void dropIndex(column_ref *tb_col);

    void descTable(const char *name);
};

#endif