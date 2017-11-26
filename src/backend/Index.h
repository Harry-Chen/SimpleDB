#ifndef __INDEX_H__
#define __INDEX_H__

#include "stx/btree_set.h"

// set rid to -1 when using tempBuffer in the table to compare.
class IndexKey {
    int permID;
    int rid;
    int fastCmp;
    int8_t col;
    bool isNull;
public:
    IndexKey() = default;

    IndexKey(int _permID, int _rid, int _col, int _fastCmp, bool _isNull) {
        permID = _permID;
        rid = _rid;
        col = _col;
        fastCmp = _fastCmp;
        isNull = _isNull;
    }

    int getRid() const { return rid; }

    int getFastCmp() const { return fastCmp; }

    friend bool operator<(const IndexKey &a, const IndexKey &b);
};

class Index {
private:
    stx::btree_set<IndexKey> list;
    stx::btree_set<IndexKey>::iterator iter;

    std::string genFilename(int tab, int col);

public:
    void clear();

    void load(int tab, int col);

    void store(int tab, int col);

    void drop(int tab, int col);

    void erase(const IndexKey &key);

    void insert(const IndexKey &key);

    int begin();

    int end();

    int lowerbound(const IndexKey &key);

    int upperbound(const IndexKey &key);

    int lowerboundEqual(const IndexKey &key);

    int next();

    int reversedNext();
};

#endif
