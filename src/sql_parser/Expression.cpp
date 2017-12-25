#include <map>
#include <cstring>
#include <cassert>
#include <regex>
#include <iomanip>

#include "constants.h"
#include "Expression.h"

using namespace std;

typedef std::pair<string, ExprVal> table_value_t;
static std::multimap<string, table_value_t> column_cache;

const char *Exception2String[] = {
        "No exception",
        "Different operand type in expression",
        "Illegal operator",
        "Unimplemented yet",
        "Column name not unique",
        "Unknown column",
        "Date not valid",
        "Wrong data type"
};

void clean_column_cache() {
    // printf("clean cache all\n");
    column_cache.clear();
}

void clean_column_cache_by_table(const char *table) {
    // printf("clean cache %s\n", table);
    for (auto it = column_cache.begin(); it != column_cache.end();) {
        if (it->second.first == table)
            it = column_cache.erase(it);
        else
            ++it;
    }
}

void update_column_cache(const char *col_name, const char *table, const ExprVal &v) {
    // printf("update cache %s\n", table);
    column_cache.insert(std::make_pair(string(col_name), table_value_t(string(table), v)));
}

void free_expr(expr_node *expr) {
    if(!expr) return;
    if (expr->op == OPER_NONE) {
        assert(expr->term_type != TERM_NONE);
        if (expr->term_type == TERM_STRING)
            free(expr->literal_s);
        else if (expr->term_type == TERM_COLUMN) {
            if (expr->column->table)
                free(expr->column->table);
            free(expr->column->column);
            free(expr->column);
        }

    } else {
        free_expr(expr->left);
        if (!(expr->op & OPER_UNARY))
            free_expr(expr->right);
    }
    free(expr);
}

bool strlike(const char *a, const char *b) {
    std::string regstr;
    char status = 'A';
    for (int i = 0; i < strlen(b); i++) {
        if (status == 'A') {
            // common status
            if (b[i] == '\\') {
                status = 'B';
            } else if (b[i] == '[') {
                regstr += "[";
                status = 'C';
            } else if (b[i] == '%') {
                regstr += ".*";
            } else if (b[i] == '_') {
                regstr += ".";
            } else {
                regstr += b[i];
            }
        } else if (status == 'B') {
            // after '\'
            if (b[i] == '%' || b[i] == '_' || b[i] == '!') {
                regstr += b[i];
            } else {
                regstr += "\\";
                regstr += b[i];
            }
            status = 'A';
        } else if (status == 'C') {
            // after '[' inside []
            if (b[i] == '!') {
                regstr += "^";
            } else {
                regstr += b[i];
            }
            status = 'A';
        } else
            assert(0);
    }

    std::regex reg(regstr);
    return std::regex_match(std::string(a), reg);
}

ExprVal termToValue(expr_node *expr) {
    ExprVal ret;
    ret.type = expr->term_type;
    switch (expr->term_type) {
        case TERM_INT:
            ret.value.value_i = expr->literal_i;
            break;
        case TERM_DATE: {
            auto date_literal = expr->literal_s;
            std::tm tm{};
            std::string date(date_literal);
            std::stringstream ss(date);
            ss >> std::get_time(&tm, DATE_FORMAT);
            auto tm_orig = tm;
            if (ss.fail()) {
                printf("Date not valid: %s\n", date_literal);
                throw (int) EXCEPTION_DATE_INVALID;
            }
            auto time = std::mktime(&tm); // drop the high 32 bits
            if (tm_orig.tm_mday != tm.tm_mday) {
                printf("Date not valid: %s\n", date_literal);
                throw (int) EXCEPTION_DATE_INVALID;
            }
            ret.value.value_i = (int) time;
            break;
        }
        case TERM_STRING:
            ret.value.value_s = expr->literal_s;
            break;
        case TERM_FLOAT:
            ret.value.value_f = expr->literal_f;
            break;
        case TERM_BOOL:
            ret.value.value_b = expr->literal_b;
            break;
        case TERM_COLUMN: {
            auto cnt = column_cache.count(string(expr->column->column));
            if (!cnt)
                goto unknown_col;
            else if (cnt > 1 && !expr->column->table)
                throw (int) EXCEPTION_COL_NOT_UNIQUE;
            {
                auto it = column_cache.find(string(expr->column->column));
                for (; it != column_cache.end(); ++it) {
                    if (!expr->column->table || it->second.first == string(expr->column->table)) {
                        ret = it->second.second;
                        goto found_col;
                    }
                }
            }
            unknown_col:
            printf("Column %s in table %s not cached.\n", expr->column->column, expr->column->table);
            throw (int) EXCEPTION_UNKNOWN_COLUMN;
            found_col:;
        }
            break;
        case TERM_NULL:
            break;
        default:
            assert(0);
    }
    return ret;
}

ExprVal calcExpression(expr_node *expr) {
    assert(expr);
    if (expr->op == OPER_NONE)
        return termToValue(expr);
    assert(expr->term_type == TERM_NONE);
    ExprVal result;
    const ExprVal &lv = calcExpression(expr->left);
    const ExprVal &rv = (expr->op & OPER_UNARY) ? ExprVal() : calcExpression(expr->right);
    if (!(expr->op & OPER_UNARY) && rv.type == TERM_NULL) {  // (<anything> <any op> NULL) = NULL
        result.type = TERM_NULL;
        return result;
    }
    if (!(expr->op & OPER_UNARY) && lv.type != rv.type && lv.type != TERM_NULL)
        throw (int) EXCEPTION_DIFF_TYPE;

    if (lv.type == TERM_INT) {
        switch (expr->op) {
            case OPER_ADD:
                result.value.value_i = lv.value.value_i + rv.value.value_i;
                result.type = TERM_INT;
                break;
            case OPER_DEC:
                result.value.value_i = lv.value.value_i - rv.value.value_i;
                result.type = TERM_INT;
                break;
            case OPER_MUL:
                result.value.value_i = lv.value.value_i * rv.value.value_i;
                result.type = TERM_INT;
                break;
            case OPER_DIV:
                result.value.value_i = lv.value.value_i / rv.value.value_i;
                result.type = TERM_INT;
                break;
            case OPER_EQU:
                //printf("Left: %d, Right: %d\n", lv.value.value_i, rv.value.value_i);
                result.value.value_b = lv.value.value_i == rv.value.value_i;
                result.type = TERM_BOOL;
                break;
            case OPER_GT:
                result.value.value_b = lv.value.value_i > rv.value.value_i;
                result.type = TERM_BOOL;
                break;
            case OPER_GE:
                result.value.value_b = lv.value.value_i >= rv.value.value_i;
                result.type = TERM_BOOL;
                break;
            case OPER_LT:
                result.value.value_b = lv.value.value_i < rv.value.value_i;
                result.type = TERM_BOOL;
                break;
            case OPER_LE:
                result.value.value_b = lv.value.value_i <= rv.value.value_i;
                result.type = TERM_BOOL;
                break;
            case OPER_NEQ:
                result.value.value_b = lv.value.value_i != rv.value.value_i;
                result.type = TERM_BOOL;
                break;
            case OPER_NEG:
                result.value.value_i = -lv.value.value_i;
                result.type = TERM_INT;
                break;
            case OPER_ISNULL:
                result.value.value_b = false;
                result.type = TERM_BOOL;
                break;
            default:
                throw (int) EXCEPTION_ILLEGAL_OP;
        }
    } else if (lv.type == TERM_DATE) {
        switch (expr->op) {
            case OPER_EQU:
                result.value.value_b = lv.value.value_i == rv.value.value_i;
                result.type = TERM_BOOL;
                break;
            case OPER_GT:
                result.value.value_b = lv.value.value_i > rv.value.value_i;
                result.type = TERM_BOOL;
                break;
            case OPER_GE:
                result.value.value_b = lv.value.value_i >= rv.value.value_i;
                result.type = TERM_BOOL;
                break;
            case OPER_LT:
                result.value.value_b = lv.value.value_i < rv.value.value_i;
                result.type = TERM_BOOL;
                break;
            case OPER_LE:
                result.value.value_b = lv.value.value_i <= rv.value.value_i;
                result.type = TERM_BOOL;
                break;
            case OPER_NEQ:
                result.value.value_b = lv.value.value_i != rv.value.value_i;
                result.type = TERM_BOOL;
                break;
            default:
                throw (int) EXCEPTION_ILLEGAL_OP;
        }
    } else if (lv.type == TERM_FLOAT) {
        switch (expr->op) {
            case OPER_ADD:
                result.value.value_f = lv.value.value_f + rv.value.value_f;
                result.type = TERM_FLOAT;
                break;
            case OPER_DEC:
                result.value.value_f = lv.value.value_f - rv.value.value_f;
                result.type = TERM_FLOAT;
                break;
            case OPER_MUL:
                result.value.value_f = lv.value.value_f * rv.value.value_f;
                result.type = TERM_FLOAT;
                break;
            case OPER_DIV:
                result.value.value_f = lv.value.value_f / rv.value.value_f;
                result.type = TERM_FLOAT;
                break;
            case OPER_EQU:
                result.value.value_b = lv.value.value_f == rv.value.value_f;
                result.type = TERM_BOOL;
                break;
            case OPER_GT:
                result.value.value_b = lv.value.value_f > rv.value.value_f;
                result.type = TERM_BOOL;
                break;
            case OPER_GE:
                result.value.value_b = lv.value.value_f >= rv.value.value_f;
                result.type = TERM_BOOL;
                break;
            case OPER_LT:
                result.value.value_b = lv.value.value_f < rv.value.value_f;
                result.type = TERM_BOOL;
                break;
            case OPER_LE:
                result.value.value_b = lv.value.value_f <= rv.value.value_f;
                result.type = TERM_BOOL;
                break;
            case OPER_NEQ:
                result.value.value_b = lv.value.value_f != rv.value.value_f;
                result.type = TERM_BOOL;
                break;
            case OPER_NEG:
                result.value.value_f = -lv.value.value_f;
                result.type = TERM_FLOAT;
                break;
            case OPER_ISNULL:
                result.value.value_b = false;
                result.type = TERM_BOOL;
                break;
            default:
                throw (int) EXCEPTION_ILLEGAL_OP;
        }
    } else if (lv.type == TERM_BOOL) {
        switch (expr->op) {
            case OPER_AND:
                result.value.value_b = lv.value.value_b & rv.value.value_b;
                result.type = TERM_BOOL;
                break;
            case OPER_OR:
                result.value.value_b = lv.value.value_b | rv.value.value_b;
                result.type = TERM_BOOL;
                break;
            case OPER_EQU:
                result.value.value_b = lv.value.value_b == rv.value.value_b;
                result.type = TERM_BOOL;
                break;
            case OPER_NOT:
                result.value.value_b = !lv.value.value_b;
                result.type = TERM_BOOL;
                break;
            case OPER_ISNULL:
                result.value.value_b = false;
                result.type = TERM_BOOL;
                break;
            default:
                throw (int) EXCEPTION_ILLEGAL_OP;
        }
    } else if (lv.type == TERM_STRING) {
        switch (expr->op) {
            case OPER_EQU:
                result.value.value_b = (strcasecmp(lv.value.value_s, rv.value.value_s) == 0);
                result.type = TERM_BOOL;
                break;
            case OPER_NEQ:
                result.value.value_b = (strcasecmp(lv.value.value_s, rv.value.value_s) != 0);
                result.type = TERM_BOOL;
                break;
            case OPER_LIKE:
                result.value.value_b = strlike(lv.value.value_s, rv.value.value_s);
                result.type = TERM_BOOL;
                break;
            case OPER_ISNULL:
                result.value.value_b = false;
                result.type = TERM_BOOL;
                break;
            default:
                throw (int) EXCEPTION_ILLEGAL_OP;
        }
    } else if (lv.type == TERM_NULL) {
        if (expr->op == OPER_ISNULL) {
            result.value.value_b = true;
            result.type = TERM_BOOL;
        } else {
            result.type = TERM_NULL;
        }
    } else {
        assert(0);
    }
    return result;
}

bool ExprVal::operator<(const ExprVal &b) const {
    if (b.type == TERM_NULL || type == TERM_NULL)
        return false;
    if (type != b.type)
        throw (int) EXCEPTION_DIFF_TYPE;
    switch (type) {
        case TERM_INT:
            return value.value_i < b.value.value_i;
            break;
        case TERM_FLOAT:
            return value.value_f < b.value.value_f;
            break;
        default:
            throw (int) EXCEPTION_ILLEGAL_OP;
    }
}

void ExprVal::operator+=(const ExprVal &b) {
    if (b.type == TERM_NULL || type == TERM_NULL)
        return;
    if (type != b.type)
        throw (int) EXCEPTION_DIFF_TYPE;
    switch (type) {
        case TERM_INT:
            value.value_i += b.value.value_i;
            break;
        case TERM_FLOAT:
            value.value_f += b.value.value_f;
            break;
        default:
            throw (int) EXCEPTION_ILLEGAL_OP;
    }

}

void ExprVal::operator/=(int div) {
    if (type == TERM_NULL)
        return;
    switch (type) {
        case TERM_INT:
            value.value_f = (double) value.value_i / div; //force convert here!
            type = TERM_FLOAT;
            break;
        case TERM_FLOAT:
            value.value_f /= div;
            break;
        default:
            throw (int) EXCEPTION_ILLEGAL_OP;
    }
}
