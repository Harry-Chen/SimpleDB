//
// Created by Harry Chen on 2017/11/20.
//

#include "Table.h"

bool operator<(const IndexKey &a, const IndexKey &b) {
    assert(a.permID == b.permID);
    assert(a.col == b.col);
    Table *tab = RegisterManager::getInstance().getPtr(a.permID);
    ColumnType tp = tab->getColumnType(a.col);

    //compare null
    if (a.isNull) {
        if (b.isNull) return a.rid < b.rid;
        return false;
    } else {
        if (b.isNull) return false;
    }

    //fast compare
    int tmp;
    switch (tp) {
        case CT_INT:
            tmp = sgn(a.fastCmp - b.fastCmp);
            if (tmp == -1) return true;
            if (tmp == 1) return false;
            return a.rid < b.rid;
            break;
        case CT_VARCHAR:
        case CT_FLOAT:
            tmp = sgn(a.fastCmp - b.fastCmp);
            if (tmp == -1) return true;
            if (tmp == 1) return false;
            break;
        default:
            assert(0);
    }

    char *x = tab->select(a.rid, a.col);
    char *y = tab->select(b.rid, b.col);
    int res;
    switch (tp) {
        case CT_VARCHAR:
            res = compareVarcharSgn(x, y);
            break;
        case CT_FLOAT:
            res = compareFloatSgn(*(float *) x, *(float *) y);
        default:
            assert(0);
    }
    delete[] x;
    delete[] y;
    if (res < 0) return true;
    if (res > 0) return false;
    return a.rid < b.rid;
}