//
// Created by Harry Chen on 2017/11/20.
//
#include <fstream>
#include <vector>

#include "DB.h"

DB::DB() {
    ready = 0;
    for (int i = 0; i < MAX_TABLE_SIZE; i++) table[i] = 0;
}

DB::~DB() {
    if (ready) close();
}

bool DB::isOpen() {
    return ready;
}

std::string DB::getDBName() {
    return dbName;
}

void DB::close() {
    assert(ready == 1);
    FILE *file = fopen((dbName + ".db").c_str(), "w");
    fprintf(file, "%d\n", tableSize);
    for (int i = 0; i < tableSize; i++) {
        table[i]->close();
        delete table[i];
        table[i] = 0;
        fprintf(file, "%s\n", tableName[i].c_str());
    }
    tableSize = 0;
    fclose(file);
    ready = 0;
}

void DB::drop() {
    assert(ready == 1);
    remove((dbName + ".db").c_str());
    for (int i = 0; i < tableSize; i++) {
        table[i]->drop();
        delete table[i];
        table[i] = 0;
        remove((dbName + "." + tableName[i] + ".table").c_str());
    }
    ready = 0;
    tableSize = 0;
}

void DB::open(const std::string &name) {
    assert(ready == 0);
    dbName = name;
    std::ifstream fin((name + ".db").c_str());
    fin >> tableSize;
    for (int i = 0; i < tableSize; i++) {
        fin >> tableName[i];
        assert(table[i] == 0);
        table[i] = new Table();
        table[i]->open((name + "." + tableName[i] + ".table").c_str());
    }
    ready = 1;
}

void DB::create(const std::string &name) {
    FILE *file = fopen((name + ".db").c_str(), "w");
    assert(file);
    fclose(file);
    assert(ready == 0);
    ready = 1;
    tableSize = 0;
    dbName = name;
}

// return 0 if not found
Table *DB::getTableByName(const std::string &name) {
    for (int i = 0; i < tableSize; i++)
        if (tableName[i] == name) {
            return table[i];
        }
    return 0;
}

Table *DB::createTable(const std::string &name) {
    assert(ready == 1);
    tableName[tableSize] = name;
    assert(table[tableSize] == 0);
    table[tableSize] = new Table();
    table[tableSize]->create((dbName + "." + name + ".table").c_str());
    tableSize++;
    return table[tableSize - 1];
}

void DB::dropTableByName(const std::string &name) {
    int p = -1;
    for (int i = 0; i < tableSize; i++)
        if (tableName[i] == name) {
            p = i;
            break;
        }
    if (p == -1) {
        printf("drop Table: Table not found!\n");
        return;
    }
    table[p]->drop();
    delete table[p];
    table[p] = table[tableSize - 1];
    table[tableSize - 1] = 0;
    tableName[p] = tableName[tableSize - 1];
    tableName[tableSize - 1] = "";
    tableSize--;
    remove((dbName + "." + name + ".table").c_str());
}

std::vector<std::string> DB::getTableNames() {
    std::vector<std::string> name;
    for (int i = 0; i < tableSize; i++) {
        name.push_back(tableName[i]);
    }
    return name;
}