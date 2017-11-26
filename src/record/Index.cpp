//
// Created by Harry Chen on 2017/11/20.
//
#include <string>
#include <sstream>
#include <fstream>

#include "RegisterManager.h"
#include "Index.h"

std::string Index::genFilename(int tab, int col) {
    std::ostringstream stm;
    stm << tab << '.' << col << ".id";
    return stm.str();
}

void Index::clear() {
    list.clear();
    iter = list.begin();
}

void Index::load(int tab, int col) {
    std::ifstream stm(genFilename(tab, col).c_str());
    list.restore(stm);
}

void Index::store(int tab, int col) {
    std::ofstream stm(genFilename(tab, col));
    list.dump(stm);
}

void Index::drop(int tab, int col) {
    remove(genFilename(tab, col).c_str());
}

void Index::erase(const IndexKey &key) {
    assert(list.erase(key) == 1);
}

void Index::insert(const IndexKey &key) {
    assert(list.find(key) == list.end());
    list.insert(key);
}

int Index::begin() {
    iter = list.begin();
    if (iter == list.end()) return -1;
    return iter->getRid();
}

int Index::end() {
    iter = list.end();
    if (iter == list.begin()) return -1;
    iter--;
    return iter->getRid();
}

int Index::lowerbound(const IndexKey &key) {
    iter = list.lower_bound(key);
    if (iter == list.end()) return -1;
    return iter->getRid();
}

int Index::upperbound(const IndexKey &key) {
    iter = list.upper_bound(key);
    if (iter == list.begin()) return -1;
    if (iter != list.begin()) iter--;
    return iter->getRid();
}

int Index::lowerboundEqual(const IndexKey &key) {
    iter = list.lower_bound(key);
    if (iter == list.end()) return -1;
    if (iter->getFastCmp() == key.getFastCmp()) return iter->getRid();
    return -1;
}

int Index::next() {
    if (iter == list.end()) return -1;
    iter++;
    if (iter == list.end()) return -1;
    return iter->getRid();
}

int Index::reversedNext() {
    if (iter == list.begin()) return -1;
    iter--;
    return iter->getRid();
}