#ifndef __INDEX_H__
#define __INDEX_H__

#include "Compare.h"
#include "stx/btree_set.h"
#include "RegisterManager.h"
#include <string>
#include <sstream>
#include <fstream>

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

    std::string genFilename(int tab, int col) {
        std::ostringstream stm;
        stm << tab << '.' << col << ".id";
        return stm.str();
    }

public:
    void clear() {
        list.clear();
        iter = list.begin();
    }

    void load(int tab, int col) {
        std::ifstream stm(genFilename(tab, col).c_str());
        list.restore(stm);
    }

    void store(int tab, int col) {
        std::ofstream stm(genFilename(tab, col));
        list.dump(stm);
    }

    void drop(int tab, int col) {
        remove(genFilename(tab, col).c_str());
    }

    void erase(const IndexKey &key) {
        assert(list.erase(key) == 1);
    }

    void insert(const IndexKey &key) {
        assert(list.find(key) == list.end());
        list.insert(key);
    }

    int begin() {
        iter = list.begin();
        if (iter == list.end()) return -1;
        return iter->getRid();
    }

    int end() {
        iter = list.end();
        if (iter == list.begin()) return -1;
        iter--;
        return iter->getRid();
    }

    int lowerbound(const IndexKey &key) {
        iter = list.lower_bound(key);
        if (iter == list.end()) return -1;
        return iter->getRid();
    }

    int upperbound(const IndexKey &key) {
        iter = list.upper_bound(key);
        if (iter == list.begin()) return -1;
        if (iter != list.begin()) iter--;
        return iter->getRid();
    }

    int lowerboundEqual(const IndexKey &key) {
        iter = list.lower_bound(key);
        if (iter == list.end()) return -1;
        if (iter->getFastCmp() == key.getFastCmp()) return iter->getRid();
        return -1;
    }

    int next() {
        if (iter == list.end()) return -1;
        iter++;
        if (iter == list.end()) return -1;
        return iter->getRid();
    }

    int reversedNext() {
        if (iter == list.begin()) return -1;
        iter--;
        return iter->getRid();
    }
};

#endif
