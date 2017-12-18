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

class ExprVal {
public:
    uint8_t type;
    union {
        char *value_s;
        int value_i;
        float value_f;
        bool value_b;
    } value;

    bool operator<(const ExprVal &) const;

    void operator+=(const ExprVal &);

    void operator/=(int);

    ExprVal(uint8_t type_) : type(type_) {}

    ExprVal() = default;

    ExprVal(const ExprVal &) = default;
};

void clean_column_cache(void);

void clean_column_cache_by_table(const char *table);

void update_column_cache(const char *col_name, const char *table, const ExprVal &v);

ExprVal calcExpression(expr_node *expr);

void free_expr(expr_node *expr);

extern const char *Exception2String[];

#endif
