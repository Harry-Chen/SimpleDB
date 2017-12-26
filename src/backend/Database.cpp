//
// Created by Harry Chen on 2017/11/20.
//
#include <fstream>
#include <vector>

#include "Database.h"

Database::Database() {
    ready = false;
    for (auto &tb : table) {
        tb = nullptr;
    }
}

Database::~Database() {
    if (ready) close();
}

bool Database::isOpen() {
    return ready;
}

std::string Database::getDBName() {
    return dbName;
}

void Database::close() {
    assert(ready);
    FILE *file = fopen((dbName + ".db").c_str(), "w");
    fprintf(file, "%zu\n", tableSize);
    for (size_t i = 0; i < tableSize; i++) {
        table[i]->close();
        delete table[i];
        table[i] = nullptr;
        fprintf(file, "%s\n", tableName[i].c_str());
    }
    tableSize = 0;
    fclose(file);
    ready = false;
}

void Database::drop() {
    assert(ready);
    remove((dbName + ".db").c_str());
    for (size_t i = 0; i < tableSize; i++) {
        table[i]->drop();
        delete table[i];
        table[i] = nullptr;
        remove((dbName + "." + tableName[i] + ".table").c_str());
    }
    ready = false;
    tableSize = 0;
}

void Database::open(const std::string &name) {
    assert(!ready);
    dbName = name;
    std::ifstream fin((name + ".db").c_str());
    fin >> tableSize;
    for (size_t i = 0; i < tableSize; i++) {
        fin >> tableName[i];
        assert(table[i] == nullptr);
        table[i] = new Table();
        table[i]->open((name + "." + tableName[i] + ".table").c_str());
    }
    ready = true;
}

void Database::create(const std::string &name) {
    auto file = fopen((name + ".db").c_str(), "w");
    assert(file);
    fclose(file);
    assert(ready == 0);
    ready = true;
    tableSize = 0;
    dbName = name;
}

// return 0 if not found
Table *Database::getTableByName(const std::string &name) {
    for (size_t i = 0; i < tableSize; i++)
        if (tableName[i] == name) {
            return table[i];
        }
    return nullptr;
}

size_t Database::getTableIdByName(const std::string &name) {
    for (size_t i = 0; i < tableSize; i++)
        if (tableName[i] == name) {
            return (size_t) i;
        }
    return (size_t) -1;
}

Table *Database::getTableById(const size_t id) {
    if (id < tableSize) return table[id];
    else return nullptr;
}

Table *Database::createTable(const std::string &name) {
    assert(ready);
    tableName[tableSize] = name;
    assert(table[tableSize] == nullptr);
    table[tableSize] = new Table();
    table[tableSize]->create((dbName + "." + name + ".table").c_str());
    tableSize++;
    return table[tableSize - 1];
}

void Database::dropTableByName(const std::string &name) {
    int p = -1;
    for (size_t i = 0; i < tableSize; i++)
        if (tableName[i] == name) {
            p = (int) i;
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

std::vector<std::string> Database::getTableNames() {
    std::vector<std::string> name;
    for (size_t i = 0; i < tableSize; i++) {
        name.push_back(tableName[i]);
    }
    return name;
}