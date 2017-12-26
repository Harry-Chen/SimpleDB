//
// Created by Harry Chen on 2017/11/26.
//

#include <vector>
#include <functional>
#include <algorithm>
#include <iostream>
#include <map>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <sql_parser/Expression.h>

#include "DBMS.h"

DBMS::DBMS() {
    current = new Database();
}

bool DBMS::requireDbOpen() {
    if (!current->isOpen()) {
        printf("%s\n", "Please USE database first!");
        return false;
    }
    return true;
}

void DBMS::printReadableException(int err) {
    printf("Exception: ");
    printf("%s\n", Exception2String[err]);
}

void DBMS::printExprVal(const Expression &val) {
    switch (val.type) {
        case TERM_INT:
            printf("%d", val.value.value_i);
            break;
        case TERM_BOOL:
            printf("%s", val.value.value_b ? "TRUE" : "FALSE");
            break;
        case TERM_FLOAT:
            printf("%.2f", val.value.value_f);
            break;
        case TERM_STRING:
            printf("'%s'", val.value.value_s);
            break;
        case TERM_DATE: {
            auto time = (time_t) val.value.value_i;
            auto tm = std::localtime(&time);
            std::cout << std::put_time(tm, DATE_FORMAT);
            break;
        }
        case TERM_NULL:
            printf("NULL");
            break;
        default:
            break;
    }
}

bool DBMS::convertToBool(const Expression &val) {
    bool t = false;
    switch (val.type) {
        case TERM_INT:
            t = val.value.value_i != 0;
            break;
        case TERM_BOOL:
            t = val.value.value_b;
            break;
        case TERM_FLOAT:
            t = val.value.value_f != 0;
            break;
        case TERM_STRING:
            t = strlen(val.value.value_s) != 0;
            break;
        case TERM_NULL:
            t = false;
            break;
        default:
            break;
    }
    return t;
}

Expression DBMS::dbTypeToExprType(char *data, ColumnType type) {
    Expression v;

    if (data == nullptr) {
        return Expression(TERM_NULL);
    }
    switch (type) {
        case CT_INT:
            v.type = TERM_INT;
            v.value.value_i = *(int *) data;
            break;
        case CT_VARCHAR:
            v.type = TERM_STRING;
            v.value.value_s = data;
            break;
        case CT_FLOAT:
            v.type = TERM_FLOAT;
            v.value.value_f = *(float *) data;
            break;
        case CT_DATE:
            v.type = TERM_DATE;
            v.value.value_i = *(int *) data;
            break;
        default:
            printf("Error: Unhandled type\n");
            assert(0);
    }
    return v;
}

term_type DBMS::ColumnTypeToExprType(const ColumnType &type) {
    switch (type) {
        case CT_INT:
            return TERM_INT;
        case CT_FLOAT:
            return TERM_FLOAT;
        case CT_VARCHAR:
            return TERM_STRING;
        case CT_DATE:
            return TERM_DATE;
        default:
            throw (int) EXCEPTION_WRONG_DATA_TYPE;
    }
}

bool DBMS::checkColumnType(ColumnType type, const Expression &val) {
    if (val.type == TERM_NULL)
        return true;
    switch (val.type) {
        case TERM_INT:
            return type == CT_INT || type == CT_FLOAT;
        case TERM_FLOAT:
            return type == CT_FLOAT;
        case TERM_STRING:
            return type == CT_VARCHAR;
        case TERM_DATE:
            return type == CT_DATE;
        default:
            return false;
    }
}

char *DBMS::ExprTypeToDbType(Expression &val, term_type desiredType) {
    char *ret = nullptr;
    //TODO: data type convert here, e.g. double->int
    switch (val.type) {
        case TERM_INT:
            if (desiredType == TERM_FLOAT) {
                val.value.value_f = val.value.value_i;
                ret = (char *) &val.value.value_f;
            } else {
                ret = (char *) &val.value.value_i;
            }
            break;
        case TERM_BOOL:
            ret = (char *) &val.value.value_b;
            break;
        case TERM_FLOAT:
            if (desiredType == TERM_INT) {
                val.value.value_i = (int) val.value.value_f;
                ret = (char *) &val.value.value_i;
            } else {
                ret = (char *) &val.value.value_f;
            }
            break;
        case TERM_STRING:
            ret = val.value.value_s;
            break;
        case TERM_DATE:
            ret = (char *) &val.value.value_i;
            break;
        case TERM_NULL:
            ret = nullptr;
            break;
        default:
            printf("Error: Unhandled type\n");
            assert(false);
    }
    return ret;
}

void DBMS::cacheColumns(Table *tb, int rid) {
    auto tb_name = tb->getTableName();
    tb_name = tb_name.substr(tb_name.find('.') + 1); //strip database name
    tb_name = tb_name.substr(0, tb_name.find('.'));
    cleanColumnCacheByTable(tb_name.c_str());
    for (int i = 1; i <= tb->getColumnCount() - 1; ++i)//exclude RID
    {
        auto *tmp = tb->select(rid, i);
        updateColumnCache(tb->getColumnName(i),
                          tb_name.c_str(),
                          dbTypeToExprType(tmp, tb->getColumnType(i))
        );
        pendingFree.push_back(tmp);
    }
}

void DBMS::freeCachedColumns() {
    for (const auto &ptr : pendingFree) {
        delete (ptr);
    }
    pendingFree.clear();
}

DBMS::IDX_TYPE DBMS::checkIndexAvailability(Table *tb, RID_t *rid_l, RID_t *rid_u, int *col, expr_node *condition) {
    //TODO: complex conditions
    if (condition && condition->node_type == TERM_NONE && condition->op == OPER_AND)
        condition = condition->left;
    if (!(condition && condition->node_type == TERM_NONE && condition->left->node_type == TERM_COLUMN))
        return IDX_NONE;
    auto col_name = condition->left->column->column;
    int c = tb->getColumnID(col_name);
    /*if(c == -1){
        c = tb->getColumnID(condition->right->column->column);
    }*/
    if (c == -1 || !tb->hasIndex(c))
        return IDX_NONE;
    Expression v;
    try {
        v = calcExpression(condition->right);
    } catch (int err) {
        printReadableException(err);
        return IDX_NONE;
    }
    IDX_TYPE type;
    switch (condition->op) {
        case OPER_EQU:
            type = IDX_EQUAL;
            break;
        case OPER_LT:
        case OPER_LE:
            type = IDX_UPPER;
            break;
        case OPER_GT:
        case OPER_GE:
            type = IDX_LOWWER;
            break;
        default:
            type = IDX_NONE;
    }
    if (type != IDX_NONE) {
        *col = c;
        auto colType = ColumnTypeToExprType(tb->getColumnType(c));
        *rid_u = tb->selectIndexUpperBound(c, ExprTypeToDbType(v, colType));
        *rid_l = tb->selectIndexLowerBound(c, ExprTypeToDbType(v, colType));
    }
    return type;
}

RID_t DBMS::nextWithIndex(Table *tb, IDX_TYPE type, int col, RID_t rid, RID_t rid_u) {
    if (type == IDX_EQUAL) {
        auto nxt = tb->selectIndexNext(col);
        return rid == rid_u ? (RID_t) -1 : nxt; // current rid equals upper bound
    } else if (type == IDX_UPPER)
        return tb->selectReveredIndexNext(col);
    else if (type == IDX_LOWWER)
        return tb->selectIndexNext(col);
    else {
        return tb->getNext(rid);
    }
}

void DBMS::iterateRecords(linked_list *tables, expr_node *condition, CallbackFunc callback) {
    auto rid = (unsigned int) -1;
    auto tb = (Table *) tables->data;
    if (!tables->next) { // fallback to one table
        return iterateRecords(tb, condition, callback);
    }
    if (!tables->next->next) {
        if (iterateTwoTableRecords(tb, (Table *) tables->next->data, condition, callback)) {
            cleanColumnCache();
            return;
        }
        printf("Iterating two tables with index failed, falling back to enumeration.\n");
    }
    while ((rid = tb->getNext(rid)) != (RID_t) -1) {
        cacheColumns(tb, rid);
        iterateRecords(tables->next, condition, callback);
    }
}

expr_node *DBMS::findJoinCondition(expr_node *condition) {
    expr_node *cond = nullptr;
    if (condition->left->node_type == TERM_COLUMN && condition->right->node_type == TERM_COLUMN) {
        cond = condition;
    } else if (condition->left->node_type == TERM_NONE) {
        cond = findJoinCondition(condition->left);
    } else if (!cond && condition->right->node_type == TERM_NONE) {
        cond = findJoinCondition(condition->right);
    }
    return cond;
}

bool
DBMS::iterateTwoTableRecords(Table *a, Table *b, expr_node *condition, CallbackFunc callback) {
    RID_t rid_a = (RID_t) -1, rid_l_a;
    int col_a;
    RID_t rid_b = (RID_t) -1, rid_l_b;
    int col_b;

    auto orig_cond = condition;

    condition = findJoinCondition(orig_cond);

    if (!condition) {
        return false;
    }

    std::vector<std::string> names;
    std::istringstream f(a->getTableName());
    std::string s;
    while (std::getline(f, s, '.')) {
        names.push_back(s);
    }
    if (names.at(1) != std::string(condition->left->column->table)) {
        // reverse a and b
        Table *temp = a;
        a = b;
        b = temp;
    }

    col_a = a->getColumnID(condition->left->column->column);
    col_b = b->getColumnID(condition->right->column->column);

    bool index_a = (col_a != -1 && a->hasIndex(col_a));
    bool index_b = (col_b != -1 && b->hasIndex(col_b));

    if (index_a && index_b) {
        goto index_both;
    } else if (index_a) {
        auto left = condition->left;
        condition->left = condition->right;
        condition->right = left;
        goto index_a;
    } else if (index_b) {
        goto index_b;
    } else {
        printf("No index on either %s or %s\n", a->getTableName().c_str(), b->getTableName().c_str());
        return false;
    }

#define iterateUseIndex(x, y) cacheColumns(x, rid_##x);\
    Expression v; \
    try{\
        v = calcExpression(condition->left);\
    } catch (int err) {\
                printReadableException(err);\
                return false;\
    }\
    auto data = ExprTypeToDbType(v, ColumnTypeToExprType(y->getColumnType(col_##y)));\
    rid_l_##y = y->selectIndexLowerBoundEqual(col_##y, data);\
    rid_##y = rid_l_##y;\
    for (; rid_##y != (RID_t) -1; rid_##y = y->selectIndexNextEqual(col_##y)) {\
        cacheColumns(y, rid_##y);\
        if (condition) {\
            Expression val_cond;\
            bool cond;\
            try {\
                val_cond = calcExpression(orig_cond);\
                cond = convertToBool(val_cond);\
            } catch (int err) {\
                printReadableException(err);\
                break;\
            } catch (...) {\
                printf("Exception occur %d\n", __LINE__);\
                break;\
            }\
            if (!cond)\
                continue;\
        }\
        callback(y, rid_##y);\
    }

    index_both:
    printf("Using index on both %s and %s\n", a->getTableName().c_str(), b->getTableName().c_str());
    rid_a = a->selectIndexLowerBoundNull(col_a);
    for (; rid_a != (RID_t) -1; rid_a = a->selectIndexNext(col_a)) {
        iterateUseIndex(a, b);
    }
    return true;

#define useIndex(x, y) printf("Using index on %s, iterating %s\n", x->getTableName().c_str(), y->getTableName().c_str());\
    while ((rid_##y = y->getNext(rid_##y)) != (RID_t) -1) { \
        iterateUseIndex(y, x); \
} \
return true;


    index_a:
useIndex(a, b);

    index_b:
useIndex(b, a);
}


void DBMS::iterateRecords(Table *tb, expr_node *condition, CallbackFunc callback) {
    RID_t rid = (RID_t) -1, rid_u;
    int col;
    IDX_TYPE idx = checkIndexAvailability(tb, &rid, &rid_u, &col, condition);
    if (idx == IDX_NONE)
        rid = tb->getNext((unsigned int) -1);
    for (; rid != (RID_t) -1; rid = nextWithIndex(tb, idx, col, rid, rid_u)) {
        cacheColumns(tb, rid);
        if (condition) {
            Expression val_cond;
            bool cond;
            try {
                val_cond = calcExpression(condition);
                cond = convertToBool(val_cond);
            } catch (int err) {
                printReadableException(err);
                return;
            } catch (...) {
                printf("Exception occur %d\n", __LINE__);
                return;
            }
            if (!cond)
                continue;
        }
        callback(tb, rid);
    }

}

int DBMS::isAggregate(const linked_list *column_expr) {
    int flags = 0;
    for (const linked_list *j = column_expr; j; j = j->next) {
        auto *node = (expr_node *) j->data;
        if (node->op == OPER_MAX ||
            node->op == OPER_MIN ||
            node->op == OPER_AVG ||
            node->op == OPER_SUM ||
            node->op == OPER_COUNT) {

            flags |= 2;
        } else {
            flags |= 1;
        }
    }
    return flags;
}

void DBMS::freeLinkedList(linked_list *t) {
    linked_list *next;
    for (; t; t = next) {
        next = t->next;
        free(t);
    }
}

DBMS *DBMS::getInstance() {
    static DBMS *instance;
    if (!instance)
        instance = new DBMS;
    return instance;
}

void DBMS::exit() {
    if (current->isOpen())
        current->close();
}

void DBMS::switchToDB(const char *name) {
    if (current->isOpen())
        current->close();

    current->open(name);
}

void DBMS::createTable(const table_def *table) {
    if (!requireDbOpen())
        return;
    assert(table->name != NULL);
    if (current->getTableByName(table->name)) {
        printf("Table `%s` already exists\n", table->name);
        return;
    }
    Table *tab = current->createTable(table->name);
    std::vector<column_defs *> column_rev;
    column_defs *column = table->columns;
    bool succeed = true;
    for (; column; column = column->next) {
        column_rev.push_back(column);
    }
    for (auto i = column_rev.rbegin(); i != column_rev.rend(); ++i) {
        auto type = (ColumnType) 0;
        column = *i;
        switch (column->type) {
            case COLUMN_TYPE_INT:
                type = CT_INT;
                break;
            case COLUMN_TYPE_VARCHAR:
                type = CT_VARCHAR;
                break;
            case COLUMN_TYPE_FLOAT:
                type = CT_FLOAT;
                break;
            case COLUMN_TYPE_DATE:
                type = CT_DATE;
                break;
            default:
                assert(false);
                break;
        }
        int ret = tab->addColumn(column->name, type, column->size,
                                 (bool) column->flags & COLUMN_FLAG_NOTNULL,
                                 (bool) column->flags & COLUMN_FLAG_DEFAULT,
                                 nullptr);
        if (ret == -1) {
            printf("Column %s duplicated\n", column->name);
            succeed = false;
            break;
        }
    }

    auto *cons_list = table->constraints;
    for (; cons_list; cons_list = cons_list->next) {
        int t;
        auto *cons = (table_constraint *) (cons_list->data);
        switch (cons->type) {
            case CONSTRAINT_PRIMARY_KEY: {
                auto *table_names = cons->values;
                for (; table_names; table_names = table_names->next) {
                    auto column_name = ((column_ref *) table_names->data)->column;
                    printf("Primary key constraint: Column in primary key: %s\n", column_name);
                    t = tab->getColumnID(column_name);
                    if (t == -1) {
                        printf("Primary key constraint: Column %s does not exist\n", column_name);
                        succeed = false;
                        break;
                    }
                    tab->createIndex(t);
                    tab->setPrimary(t);
                }


                break;
            }
            case CONSTRAINT_CHECK:
                t = tab->getColumnID(cons->column_name);
                if (t == -1) {
                    printf("Value constraint: Column %s does not exist\n", cons->column_name);
                    succeed = false;
                    break;
                }
                {
                    linked_list *exprs = cons->values;
                    for (; exprs; exprs = exprs->next) {
                        auto node = (expr_node *) exprs->data;
                        Expression val;
                        try {
                            val = calcExpression(node);
                        } catch (int err) {
                            printReadableException(err);
                            return;
                        } catch (...) {
                            printf("Exception occur %d\n", __LINE__);
                        }
                        tab->addCheck(t, OP_EQ, ExprTypeToDbType(val, ColumnTypeToExprType(tab->getColumnType(t))),
                                      RE_OR);
                        printf("Value constraint: Column %s must be ", cons->column_name);
                        printExprVal(val);
                        putchar('\n');
                    }
                }
                break;
            case CONSTRAINT_FOREIGN_KEY: {
                printf("Foreign key: COLUMN %s REFERENCES TABLE %s COLUMN %s\n",
                       cons->column_name, cons->foreign_table_name, cons->foreign_column_name);
                t = tab->getColumnID(cons->column_name);
                if (t == -1) {
                    printf("Foreign key constraint: Column %s does not exist\n", cons->column_name);
                    succeed = false;
                    break;
                }
                if (tab->getColumnType(t) != CT_INT) {
                    printf("Foreign key constraint: Column %s must be int.\n", cons->column_name);
                    succeed = false;
                    break;
                }
                auto foreign_table_id = current->getTableIdByName(cons->foreign_table_name);
                if (foreign_table_id == (size_t) -1) {
                    printf("Foreign key constraint: Foreign table %s does not exist\n", cons->foreign_table_name);
                    succeed = false;
                    break;
                }
                auto foreign_table = current->getTableById(foreign_table_id);
                auto foreign_col = foreign_table->getColumnID(cons->foreign_column_name);
                if (foreign_col == -1) {
                    printf("Foreign key constraint: Foreign column %s does not exist\n", cons->foreign_column_name);
                    succeed = false;
                    break;
                }
                if (tab->getColumnType(t) != foreign_table->getColumnType(foreign_col)) {
                    printf("Foreign key constraint: Type of foreign column %s does not match %s\n",
                           cons->foreign_column_name, cons->column_name);
                    succeed = false;
                    break;
                }
                if (!foreign_table->hasIndex(foreign_col)) {
                    printf("Foreign key constraint: Foreign column %s must be indexed.\n", cons->foreign_column_name);
                    succeed = false;
                    break;
                }
                tab->addForeignKeyConstraint(t, (int) foreign_table_id, foreign_col);
                break;
            }
            default:
                assert(0); // WTF?
        }
    }

    if (!succeed)
        current->dropTableByName(table->name);
    else {
        printf("Table %s created\n", table->name);
    }
}

void DBMS::dropDB(const char *db_name) {
    Database db;
    if (current->isOpen() && current->getDBName() == db_name)
        current->close();
    db.open(db_name);
    if (db.isOpen()) {
        db.drop();
        printf("Database %s dropped!\n", db_name);
    } else {
        printf("Failed to open database %s\n", db_name);
    }
}

void DBMS::dropTable(const char *table) {
    if (!requireDbOpen())
        return;
    current->dropTableByName(table);
    printf("Table %s dropped!\n", table);
}

void DBMS::listTables() {
    if (!requireDbOpen())
        return;
    const std::vector<std::string> &tables =
            current->getTableNames();
    printf("List of tables:\n");
    for (const auto &table : tables) {
        printf("%s\n", table.c_str());
    }
    printf("==========\n");
}

void DBMS::selectRow(const linked_list *tables, const linked_list *column_expr, expr_node *condition) {
    int flags;
    if (!requireDbOpen())
        return;
    linked_list *openedTables = nullptr;
    bool allOpened = true;
    for (; tables; tables = tables->next) {
        Table *tb;
        auto *table = (const char *) tables->data;
        if (!(tb = current->getTableByName(table))) {
            printf("Table %s not found\n", table);
            allOpened = false;
        }
        auto *t = (linked_list *) malloc(sizeof(linked_list));
        t->next = openedTables;
        t->data = tb;
        openedTables = t;
    }
    if (!allOpened) {
        freeLinkedList(openedTables);
        return;
    }
    flags = isAggregate(column_expr);
    if (flags == 3) {
        printf("Error: Cannot mix aggregate functions and non-aggregate columns in one query\n");
        return;
    }
    cleanColumnCache();
    if (flags == 2) { //aggregate functions only
        std::map<int, Expression> aggregate_buf;
        std::map<int, int> rowCount;
        try {
            iterateRecords(openedTables, condition,
                           [&rowCount, &aggregate_buf, &column_expr, this](Table *tb, int rid) -> void {
                               UNUSED(tb);
                               UNUSED(rid);
                               int col = 0;
                               for (const linked_list *j = column_expr; j; j = j->next, col++) {
                                   auto node = (expr_node *) j->data;
                                   Expression val;
                                   if (node->op != OPER_COUNT) {
                                       val = calcExpression(node->left);
                                   }
                                   if (node->left == nullptr || val.type != TERM_NULL) {
                                       rowCount[col]++;
                                       if (node->op != OPER_COUNT && !aggregate_buf.count(col)) {
                                           aggregate_buf[col] = (val);
                                       } else {
                                           switch (node->op) {
                                               case OPER_MIN:
                                                   if (val < aggregate_buf[col])
                                                       aggregate_buf[col] = val;
                                                   break;
                                               case OPER_MAX:
                                                   if (aggregate_buf[col] < val)
                                                       aggregate_buf[col] = val;
                                                   break;
                                               case OPER_SUM:
                                               case OPER_AVG:
                                                   aggregate_buf[col] += val;
                                                   break;
                                               default:
                                                   break;
                                           }
                                       }
                                   }

                               }
                           });
        } catch (int err) {
            printReadableException(err);
            return;
        } catch (...) {
            printf("Exception occur %d\n", __LINE__);
            return;
        }
        int col = 0;
        printf("| ");
        for (const linked_list *j = column_expr; j; j = j->next, col++) {
            auto node = (expr_node *) j->data;
            if (node->op == OPER_AVG && aggregate_buf.count(col)) {
                aggregate_buf[col] /= rowCount[col];
            } else if (node->op == OPER_COUNT) {
                aggregate_buf[col] = Expression();
                aggregate_buf[col].type = TERM_INT;
                aggregate_buf[col].value.value_i = rowCount[col];
            }
        }
        for (int i = col - 1; i >= 0; i--) {
            if (aggregate_buf.count(i) != 0)
                printExprVal(aggregate_buf[i]);
            else
                printExprVal(Expression(TERM_NULL));
            printf(" | ");
        }
        printf("\n");
        freeCachedColumns();
        freeLinkedList(openedTables);
        return;
    }
    int count = 0;

    iterateRecords(openedTables, condition, [&column_expr, &count, this](Table *tb, int rid) -> void {
        std::vector<Expression> output_buf;
        if (!column_expr) { // FIXME: will only select from one table when using *
            for (int i = tb->getColumnCount() - 1; i > 0; --i) {
                output_buf.push_back(
                        dbTypeToExprType(
                                tb->select(rid, i),
                                tb->getColumnType(i)
                        )
                );
            }
        } else {
            for (const linked_list *j = column_expr; j; j = j->next) {
                auto *node = (expr_node *) j->data;
                Expression val;
                try {
                    val = calcExpression(node);
                    output_buf.push_back(val);
                } catch (int err) {
                    printReadableException(err);
                    return;
                } catch (...) {
                    printf("Exception occur %d\n", __LINE__);
                    return;
                }
            }
        }
        printf("| ");
        for (auto i = output_buf.rbegin(); i != output_buf.rend(); ++i) {
            const Expression &val = *i;
            printExprVal(val);
            printf(" | ");
        }
        printf("\n");
        count++;
    });
    printf("%d rows in query.\n", count);
    freeCachedColumns();
    freeLinkedList(openedTables);
}

void DBMS::updateRow(const char *table, expr_node *condition, column_ref *column, expr_node *eval) {
    Table *tb;
    if (!requireDbOpen())
        return;
    if (!(tb = current->getTableByName(table))) {
        printf("Table %s not found\n", table);
        return;
    }

    int col_to_update;
    col_to_update = tb->getColumnID(column->column);
    if (col_to_update == -1) {
        printf("Column %s not found\n", column->column);
        return;
    }
    int count = 0;
    try {
        iterateRecords(tb, condition, [&col_to_update, &eval, &count, this](Table *tb, int rid) -> void {
            Expression new_val;
            new_val = calcExpression(eval);
            //printf("t=%d\n", tb->getColumnType(col_to_update));
            auto colType = tb->getColumnType(col_to_update);
            if (!checkColumnType(colType, new_val)) {
                printf("Wrong data type\n");
                throw (int) EXCEPTION_WRONG_DATA_TYPE;
            }
            std::string ret = tb->modifyRecord(rid, col_to_update,
                                               ExprTypeToDbType(new_val, ColumnTypeToExprType(colType)));
            if (!ret.empty()) {
                std::cout << ret << std::endl;
                throw (int) EXCEPTION_WRONG_DATA_TYPE;
            }
            ++count;
        });
    } catch (int err) {
        printReadableException(err);
    } catch (...) {
        printf("Exception occur %d\n", __LINE__);
    }
    printf("%d rows updated.\n", count);
    freeCachedColumns();
}

void DBMS::deleteRow(const char *table, expr_node *condition) {
    std::vector<RID_t> toBeDeleted;
    Table *tb;
    if (!requireDbOpen())
        return;
    if (!(tb = current->getTableByName(table))) {
        printf("Table %s not found\n", table);
        return;
    }
    iterateRecords(tb, condition, [&toBeDeleted, this](Table *tb, int rid) -> void {
        UNUSED(tb);
        toBeDeleted.push_back(rid);
    });
    for (const auto &i : toBeDeleted) {
        tb->dropRecord(i);
    }
    printf("%d rows deleted.\n", (int) toBeDeleted.size());
    freeCachedColumns();
}

void DBMS::insertRow(const char *table, const linked_list *columns, const linked_list *values) {
    Table *tb;
    if (!requireDbOpen())
        return;
    if (!(tb = current->getTableByName(table))) {
        printf("Table %s not found\n", table);
        return;
    }
    std::vector<int> colId;
    if (!columns) { //column list is not specified
        for (int i = tb->getColumnCount() - 1; i > 0; --i) //exclude RID
        {
            colId.push_back(i);
        }
    } else
        for (const linked_list *i = columns; i; i = i->next) {
            const column_ref *col = (column_ref *) i->data;
            if (col->table && strcasecmp(col->table, table) != 0) {
                printf("Illegal column reference: %s.%s\n", col->table, col->column);
                return;
            }
            int id = tb->getColumnID(col->column);
            printf("Column %s id=%d\n", col->column, id);
            if (id < 0) {
                printf("Column %s not found\n", col->column);
                return;
            }
            colId.push_back(id);
        }
    printf("Inserting into %lu columns\n", colId.size());
    tb->clearTempRecord();
    int count = 0;
    for (const linked_list *i = values; i; i = i->next) {
        const linked_list *expr_list = (linked_list *) i->data;
        unsigned int cnt = 0;
        for (const linked_list *j = expr_list; j; j = j->next) {
            cnt++;
        }
        if (cnt != colId.size()) {
            printf("Column size mismatch, will not execute (value size=%d)\n", cnt);
            continue;
        }
        //printf("Insert one row...\n");
        auto it = colId.begin();
        std::string result;
        for (const linked_list *j = expr_list; j; j = j->next) {
            auto node = (expr_node *) j->data;
            Expression val;
            try {
                val = calcExpression(node);
            } catch (int err) {
                printReadableException(err);
                return;
            } catch (...) {
                printf("Exception occur %d\n", __LINE__);
                return;
            }
            //printf("Column [%d] value ", *it);
            //printExprVal(val);
            //putchar('\n');
            auto colType = tb->getColumnType(*it);
            if (!checkColumnType(colType, val)) {
                printf("Wrong data type\n");
                return;
            }
            auto exprType = ColumnTypeToExprType(colType);
            result = tb->setTempRecord(*it, ExprTypeToDbType(val, exprType));
            if (!result.empty()) {
                std::cout << result << std::endl;
                goto next_rec;
            }
            ++it;
        }
        result = tb->insertTempRecord();
        if (!result.empty()) {
            std::cout << result << std::endl;
        } else {
            ++count;
        }
        next_rec:;
    }
    printf("%d rows inserted.\n", count);
}

void DBMS::createIndex(column_ref *tb_col) {
    Table *tb;
    if (!requireDbOpen())
        return;
    if (!(tb = current->getTableByName(tb_col->table))) {
        printf("Table %s not found\n", tb_col->table);
        return;
    }
    int t = tb->getColumnID(tb_col->column);
    if (t == -1) {
        printf("Column %s not exist\n", tb_col->column);
    } else
        tb->createIndex(t);
}

void DBMS::dropIndex(column_ref *tb_col) {
    Table *tb;
    if (!requireDbOpen())
        return;
    if (!(tb = current->getTableByName(tb_col->table))) {
        printf("Table %s not found\n", tb_col->table);
        return;
    }
    int t = tb->getColumnID(tb_col->column);
    if (t == -1) {
        printf("Column %s not exist\n", tb_col->column);
    } else if (!tb->hasIndex(t)) {
        printf("No index on %s(%s)\n", tb_col->table, tb_col->column);
    } else {
        tb->dropIndex(t);
    }
}

void DBMS::descTable(const char *name) {
    Table *tb;
    if (!requireDbOpen())
        return;
    if (!(tb = current->getTableByName(name))) {
        printf("Table %s not found\n", name);
        return;
    }
    tb->printSchema();
}

bool DBMS::valueExistInTable(const char *value, const ForeignKey &key) {
    auto table = current->getTableById(key.foreign_table_id);
    auto result = table->selectIndexLowerBoundEqual(key.foreign_col, value);
    return result != (RID_t) -1;
}