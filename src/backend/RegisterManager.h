#ifndef __REGISTER_H__
#define __REGISTER_H__

#include <map>
#include <cassert>

class Table;

class RegisterManager {
private:
    std::map<int, Table *> list;

    RegisterManager() = default;

    ~RegisterManager() = default;

public:
    RegisterManager(RegisterManager const &) = delete;

    RegisterManager &operator=(RegisterManager const &) = delete;

    static RegisterManager &getInstance() {
        static RegisterManager instance;
        return instance;
    }

    void checkIn(int permID, Table *table) {
        assert(list.find(permID) == list.end());
        list[permID] = table;
    }

    Table *getPtr(int permID) {
        assert(list.find(permID) != list.end());
        return list[permID];
    }

    void checkOut(int permID) {
        list.erase(permID);
    }
};


#endif
