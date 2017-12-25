#ifndef __DB_H__
#define __DB_H__

#include <string>
#include <vector>

#include "../constants.h"
#include "Table.h"

class Database {
    bool ready;
    std::string tableName[MAX_TABLE_SIZE];
    int tableSize;
    Table *table[MAX_TABLE_SIZE];
    std::string dbName;

public:
    Database();

    ~Database();

    bool isOpen();

    std::string getDBName();

    void close();

    void drop();

    void open(const std::string &name);

    void create(const std::string &name);

    // return 0 if not found
    Table *getTableByName(const std::string &name);

    size_t getTableIdByName(const std::string &name);

    Table *getTableById(const size_t id);

    Table *createTable(const std::string &name);

    void dropTableByName(const std::string &name);

    std::vector<std::string> getTableNames();
};

#endif
