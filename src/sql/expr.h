#ifndef EXPR_H__
#define EXPR_H__

#include <stdint.h>
#include "type_def.h"

// #ifdef __cplusplus
// extern "C"{
// #endif

enum{
    EXCEPTION_NONE,
    EXCEPTION_DIFF_TYPE,
    EXCEPTION_ILLEGAL_OP,
    EXCEPTION_UNIMPLEMENTED,
    EXCEPTION_COL_NOT_UNIQUE,
    EXCEPTION_UNKOWN_COLUMN
};

class ExprVal
{
public:
    uint8_t type;
    union{
        char* value_s;
        int value_i;
        double value_d;
        bool value_b;
    }value;
    bool operator<(const ExprVal&)const;
    void operator+=(const ExprVal&);
    void operator/=(int);
    ExprVal(uint8_t type_):type(type_){}
    ExprVal() = default;
    ExprVal(const ExprVal&) = default;
};

void clean_column_cache(void);
void clean_column_cache_by_table(const char* table);
void update_column_cache(const char* col_name, const char* table, const ExprVal &v);
ExprVal calcExpression(expr_node* expr);
void free_expr(expr_node* expr);

extern const char *Exception2String[];

// #ifdef __cplusplus
// }
// #endif

#endif
