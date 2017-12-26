#ifndef EXPR_H__
#define EXPR_H__

#include <stdint.h>
#include "type_def.h"

enum {
    EXCEPTION_NONE,
    EXCEPTION_DIFF_TYPE,
    EXCEPTION_ILLEGAL_OP,
    EXCEPTION_UNIMPLEMENTED,
    EXCEPTION_COL_NOT_UNIQUE,
    EXCEPTION_UNKNOWN_COLUMN,
    EXCEPTION_DATE_INVALID,
    EXCEPTION_WRONG_DATA_TYPE
};

class Expression {
public:
    term_type type;
    union {
        char *value_s;
        int value_i;
        float value_f;
        bool value_b;
    } value;

    bool operator<(const Expression &) const;

    void operator+=(const Expression &);

    void operator/=(int);

    explicit Expression(term_type type_) : type(type_) {}

    Expression() = default;

    Expression(const Expression &) = default;
};

void cleanColumnCache();

void cleanColumnCacheByTable(const char *table);

void updateColumnCache(const char *col_name, const char *table, const Expression &v);

Expression calcExpression(expr_node *expr);

void free_expr(expr_node *expr);

extern const char *Exception2String[];

#endif
