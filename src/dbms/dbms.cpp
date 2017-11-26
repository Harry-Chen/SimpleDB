//
// Created by Harry Chen on 2017/11/26.
//

#include "dbms.h"

DBMS::DBMS() {
    current = new DB();
}

bool DBMS::requireDbOpen() {
    if (!current->isOpen()) {
        printf("%s\n", "Please USE database first!");
        return false;
    }
    return true;
}

void DBMS::printExpName(int err) {
    printf("Exception: ");
    printf("%s\n", Exception2String[err]);
}

void DBMS::printExprVal(const ExprVal &val) {
    switch (val.type) {
        case TERM_INT:
            printf("%d", val.value.value_i);
            break;
        case TERM_BOOL:
            printf("%s", val.value.value_b ? "TRUE" : "FALSE");
            break;
        case TERM_DOUBLE:
            printf("%lf", val.value.value_d);
            break;
        case TERM_STRING:
            printf("'%s'", val.value.value_s);
            break;
        case TERM_NULL:
            printf("NULL");
            break;
    }
}

bool DBMS::convert2Bool(const ExprVal &val) {
    bool t = false;
    switch (val.type) {
        case TERM_INT:
            t = val.value.value_i;
            break;
        case TERM_BOOL:
            t = val.value.value_b;
            break;
        case TERM_DOUBLE:
            t = val.value.value_d;
            break;
        case TERM_STRING:
            t = strlen(val.value.value_s);
            break;
        case TERM_NULL:
            t = false;
            break;
    }
    return t;
}

ExprVal DBMS::dbType2ExprType(char *data, ColumnType type) {
    ExprVal v;

    if (data == NULL) {
        return ExprVal(TERM_NULL);
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
            v.type = TERM_DOUBLE;
            v.value.value_d = *(float *) data;
            break;
        default:
            printf("Error: Unhandled type\n");
            assert(0);
    }
    return v;
}

bool DBMS::checkColumnType(ColumnType type, const ExprVal &val) {
    if (val.type == TERM_NULL)
        return true;
    switch (val.type) {
        case TERM_INT:
            return type == CT_INT;
            break;
        case TERM_DOUBLE:
            return type == CT_FLOAT;
            break;
        case TERM_STRING:
            return type == CT_VARCHAR;
            break;
        default:
            return false;
    }
}

char *DBMS::ExprType2dbType(const ExprVal &val) {
    char *ret;
    //TODO: data type convert here, e.g. double->int
    switch (val.type) {
        case TERM_INT:
            // printf("int value: %d\n", val.value.value_i);
            ret = (char *) &val.value.value_i;
            break;
        case TERM_BOOL:
            // printf("bool value: %d\n", val.value.value_b);
            ret = (char *) &val.value.value_b;
            break;
        case TERM_DOUBLE:
            // printf("double value: %lf\n", val.value.value_d);
            ret = (char *) &val.value.value_d;
            break;
        case TERM_STRING:
            // printf("string value: %s\n", val.value.value_s);
            ret = (char *) val.value.value_s;
            break;
        case TERM_NULL:
            // printf("NULL value\n");
            ret = NULL;
            break;
        default:
            printf("Error: Unhandled type\n");
            assert(0);
    }
    return ret;
}

void DBMS::cacheColumns(Table *tb, int rid) {
    std::string tb_name = tb->getTableName();
    tb_name = tb_name.substr(tb_name.find('.') + 1); //strip database name
    tb_name = tb_name.substr(0, tb_name.find('.'));
    clean_column_cache_by_table(tb_name.c_str());
    for (int i = 1; i <= tb->getColumnCount() - 1; ++i)//exclude RID
    {
        char *tmp = tb->select(rid, i);
        update_column_cache(tb->getColumnName(i),
                            tb_name.c_str(),
                            dbType2ExprType(tmp, tb->getColumnType(i))
        );
        pendingFree.push_back(tmp);
    }
}

void DBMS::freeCacheColumns() {
    for (auto i = pendingFree.begin(); i != pendingFree.end(); ++i) {
        delete (*i);
    }
    pendingFree.clear();
}

DBMS::IDX_TYPE DBMS::checkIndexAvai(Table *tb, int *rid_l, int *rid_u, int *col, expr_node *condition) {
    //TODO: complex conditions
    if (condition && condition->term_type == TERM_NONE && condition->op == OPER_AND)
        condition = condition->left;
    if (!(condition && condition->term_type == TERM_NONE && condition->left->term_type == TERM_COLUMN))
        return IDX_NONE;
    const char *cname = condition->left->column->column;
    int c = tb->getColumnID(cname);
    if (c == -1 || !tb->hasIndex(c))
        return IDX_NONE;
    ExprVal v;
    try {
        v = calcExpression(condition->right);
    } catch (...) {
        return IDX_NONE;
    }
    IDX_TYPE type;
    switch (condition->op) {
        case OPER_EQU:
            printf("Equal index on `%s`\n", cname);
            type = IDX_EQUAL;
            break;
        case OPER_LT:
        case OPER_LE:
            printf("Upper index on `%s`\n", cname);
            type = IDX_UPPER;
            break;
        case OPER_GT:
        case OPER_GE:
            printf("Lowwer index on `%s`\n", cname);
            type = IDX_LOWWER;
            break;
        default:
            type = IDX_NONE;
    }
    if (type != IDX_NONE) {
        *col = c;
        *rid_u = tb->selectIndexUpperBound(c, ExprType2dbType(v));
        *rid_l = tb->selectIndexLowerBound(c, ExprType2dbType(v));
    }
    return type;
}

int DBMS::nextWithIndex(Table *tb, IDX_TYPE type, int col, int rid, int rid_u) {
    if (type == IDX_NONE)
        return tb->getNext(rid);
    else if (type == IDX_EQUAL) {
        int nxt = tb->selectIndexNext(col);
        return rid == rid_u ? -1 : nxt; // current rid equals upper bound
    } else if (type == IDX_UPPER)
        return tb->selectReveredIndexNext(col);
    else if (type == IDX_LOWWER)
        return tb->selectIndexNext(col);
    assert(0);
}

void DBMS::iterateRecords(linked_list *tables, expr_node *condition, std::function<void(Table *, int)> callback) {
    int rid = -1;
    Table *tb = (Table *) tables->data;
    if (!tables->next)
        return iterateRecords(tb, condition, callback);
    while ((rid = tb->getNext(rid)) != -1) {
        cacheColumns(tb, rid);
        iterateRecords(tables->next, condition, callback);
    }
}

void DBMS::iterateRecords(Table *tb, expr_node *condition, std::function<void(Table *, int)> callback) {
    int rid = -1, rid_u;
    int col;
    // printf("table %s\n", tb->getTableName().c_str());
    IDX_TYPE idx = checkIndexAvai(tb, &rid, &rid_u, &col, condition);
    if (idx == IDX_NONE)
        rid = tb->getNext(-1);
    // printf("[%d,%d]\n", rid,rid_u);
    for (; rid != -1; rid = nextWithIndex(tb, idx, col, rid, rid_u)) {
        cacheColumns(tb, rid);
        // printf("rid=%d\n", rid);
        if (condition) {
            ExprVal val_cond;
            bool cond;
            try {
                val_cond = calcExpression(condition);
                cond = convert2Bool(val_cond);
            } catch (int err) {
                printExpName(err);
            } catch (...) {
                printf("Exception occur %d\n", __LINE__);
            }
            if (!cond)
                continue;
        }
        printf("RID=%d match\n", rid);
        callback(tb, rid);
    }

}

int DBMS::isAggregate(const linked_list *column_expr) {
    int flags = 0;
    for (const linked_list *j = column_expr; j; j = j->next) {
        expr_node *node = (expr_node *) j->data;
        if (node->op == OPER_MAX ||
            node->op == OPER_MIN ||
            node->op == OPER_AVG ||
            node->op == OPER_SUM) {

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
        ColumnType type;
        column = *i;
        switch (column->type) {
            case COLUMN_TYPE_INT:
                type = CT_INT;
                break;
            case COLUMN_TYPE_VARCHAR:
                type = CT_VARCHAR;
                break;
            case COLUMN_TYPE_DOUBLE:
                type = CT_FLOAT;
                break;
        }
        int ret = tab->addColumn(column->name, type, column->size,
                                 column->flags & COLUMN_FLAG_NOTNULL,
                                 column->flags & COLUMN_FLAG_DEFAULT,
                                 0);
        if (ret == -1) {
            printf("column %s duplicated\n", column->name);
            succeed = false;
            break;
        }
    }

    linked_list *cons_list = table->constraints;
    for (; cons_list; cons_list = cons_list->next) {
        int t;
        table_constraint *cons = (table_constraint *) (cons_list->data);
        switch (cons->type) {
            case CONSTRAINT_PRIMARY_KEY:
                t = tab->getColumnID(cons->column_name);
                if (t == -1) {
                    printf("Column %s not exist\n", cons->column_name);
                    succeed = false;
                    break;
                }
                tab->createIndex(t);
                tab->setPrimary(t);
                break;
            case CONSTRAINT_CHECK:
                t = tab->getColumnID(cons->column_name);
                if (t == -1) {
                    printf("Column %s not exist\n", cons->column_name);
                    succeed = false;
                    break;
                }
                {
                    linked_list *exprs = cons->values;
                    for (; exprs; exprs = exprs->next) {
                        expr_node *node = (expr_node *) exprs->data;
                        ExprVal val;
                        try {
                            val = calcExpression(node);
                        } catch (int err) {
                            printExpName(err);
                            return;
                        } catch (...) {
                            printf("Exception occur %d\n", __LINE__);
                        }
                        tab->addCheck(t, OP_EQ, ExprType2dbType(val), RE_OR);
                        printf("CHECK: column %d val ", t);
                        printExprVal(val);
                        putchar('\n');
                    }
                }
                break;
        }
    }

    if (!succeed)
        current->dropTableByName(table->name);
    else {
        printf("Table %s created\n", table->name);
    }
}

void DBMS::dropDB(const char *db_name) {
    DB db;
    if (current->isOpen() && current->getDBName() == db_name)
        current->close();
    db.open(db_name);
    if (db.isOpen()) {
        db.drop();
        printf("database %s dropped!\n", db_name);
    } else {
        printf("failed to open database %s\n", db_name);
    }
}

void DBMS::dropTable(const char *table) {
    if (!requireDbOpen())
        return;
    current->dropTableByName(table);
    printf("%s dropped!\n", table);
}

void DBMS::listTables() {
    if (!requireDbOpen())
        return;
    const std::vector<std::string> &tables =
            current->getTableNames();
    printf("List of tables:\n");
    for (std::vector<std::string>::const_iterator i = tables.begin(); i != tables.end(); ++i) {
        printf("%s\n", i->c_str());
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
        const char *table = (const char *) tables->data;
        if (!(tb = current->getTableByName(table))) {
            printf("Table %s not found\n", table);
            allOpened = false;
        }
        linked_list *t = (linked_list *) malloc(sizeof(linked_list));
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
        printf("Error: Both aggregate function and non-aggregate column exist\n");
        return;
    }
    clean_column_cache();
    if (flags == 2) { //aggregate functions only
        std::map<int, ExprVal> aggregate_buf;
        std::map<int, int> rowCount;
        iterateRecords(openedTables, condition,
                       [&rowCount, &aggregate_buf, &column_expr, this](Table *tb, int rid) -> void {
                           int col = 0;
                           for (const linked_list *j = column_expr; j; j = j->next, col++) {
                               expr_node *node = (expr_node *) j->data;
                               ExprVal val;
                               try {
                                   val = calcExpression(node->left);
                                   if (val.type != TERM_NULL) {
                                       rowCount[col]++;
                                       if (!aggregate_buf.count(col)) {
                                           printf("rid=%d [%d] first\n", rid, col);
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
                                           }
                                       }
                                   }
                               } catch (int err) {
                                   printExpName(err);
                                   return;
                               } catch (...) {
                                   printf("Exception occur %d\n", __LINE__);
                                   return;
                               }
                           }
                       });
        int col = 0;
        printf("| ");
        for (const linked_list *j = column_expr; j; j = j->next, col++) {
            expr_node *node = (expr_node *) j->data;
            if (node->op == OPER_AVG && aggregate_buf.count(col)) {
                aggregate_buf[col] /= rowCount[col];
            }
        }
        for (int i = col - 1; i >= 0; i--) {
            if (aggregate_buf.count(i))
                printExprVal(aggregate_buf[i]);
            else
                printExprVal(ExprVal(TERM_NULL));
            printf(" | ");
        }
        printf("\n");
        freeCacheColumns();
        freeLinkedList(openedTables);
        return;
    }
    iterateRecords(openedTables, condition, [&column_expr, this](Table *tb, int rid) -> void {
        std::vector<ExprVal> output_buf;
        if (!column_expr) { //select *
            for (int i = tb->getColumnCount() - 1; i > 0; --i) {
                output_buf.push_back(
                        dbType2ExprType(
                                tb->select(rid, i),
                                tb->getColumnType(i)
                        )
                );
            }
        } else {
            for (const linked_list *j = column_expr; j; j = j->next) {
                expr_node *node = (expr_node *) j->data;
                ExprVal val;
                try {
                    val = calcExpression(node);
                    output_buf.push_back(val);
                } catch (int err) {
                    printExpName(err);
                    return;
                } catch (...) {
                    printf("Exception occur %d\n", __LINE__);
                    return;
                }
            }
        }
        printf("| ");
        for (auto i = output_buf.rbegin(); i != output_buf.rend(); ++i) {
            const ExprVal &val = *i;
            printExprVal(val);
            printf(" | ");
        }
        printf("\n");
    });
    freeCacheColumns();
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

    iterateRecords(tb, condition, [&col_to_update, &eval, this](Table *tb, int rid) -> void {
        ExprVal newval;
        try {
            newval = calcExpression(eval);
            printf("t=%d\n", tb->getColumnType(col_to_update));
            if (!checkColumnType(tb->getColumnType(col_to_update), newval)) {
                printf("Wrong data type\n");
                return;
            }
            std::string ret = tb->modifyRecord(rid, col_to_update, ExprType2dbType(newval));
            std::cout << ret << std::endl;
        } catch (int err) {
            printExpName(err);
        } catch (...) {
            printf("Exception occur %d\n", __LINE__);
        }
    });
    freeCacheColumns();
}

void DBMS::deleteRow(const char *table, expr_node *condition) {
    std::vector<int> toBeDeleted;
    Table *tb;
    if (!requireDbOpen())
        return;
    if (!(tb = current->getTableByName(table))) {
        printf("Table %s not found\n", table);
        return;
    }
    iterateRecords(tb, condition, [&toBeDeleted, this](Table *tb, int rid) -> void {
        toBeDeleted.push_back(rid);
    });
    for (auto i = toBeDeleted.begin(); i != toBeDeleted.end(); ++i) {
        tb->dropRecord(*i);
    }
    freeCacheColumns();
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
            if (col->table && strcasecmp(col->table, table)) {
                printf("Illegal column reference: %s.%s\n", col->table, col->column);
                return;
            }
            int id = tb->getColumnID(col->column);
            printf("column %s id=%d\n", col->column, id);
            if (id < 0) {
                printf("Column %s not found\n", col->column);
                return;
            }
            colId.push_back(id);
        }
    printf("insert into %lu columns\n", colId.size());
    tb->clearTempRecord();
    for (const linked_list *i = values; i; i = i->next) {
        const linked_list *expr_list = (linked_list *) i->data;
        int cnt = 0;
        for (const linked_list *j = expr_list; j; j = j->next) {
            cnt++;
        }
        if (cnt != colId.size()) {
            printf("column size mismatch, skipped (value size=%d)\n", cnt);
            continue;
        }
        printf("insert one row...\n");
        auto it = colId.begin();
        std::string result;
        for (const linked_list *j = expr_list; j; j = j->next) {
            expr_node *node = (expr_node *) j->data;
            ExprVal val;
            try {
                val = calcExpression(node);
            } catch (int err) {
                printExpName(err);
                return;
            } catch (...) {
                printf("Exception occur %d\n", __LINE__);
            }
            printf("column [%d] value ", *it);
            printExprVal(val);
            putchar('\n');
            if (!checkColumnType(tb->getColumnType(*it), val)) {
                printf("Wrong data type\n");
                return;
            }
            result = tb->setTempRecord(*it, ExprType2dbType(val));
            if (!result.empty()) {
                std::cout << result << std::endl;
                goto next_rec;
            }
            ++it;
        }
        result = tb->insertTempRecord();
        if (!result.empty())
            std::cout << result << std::endl;
        next_rec:;
    }
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