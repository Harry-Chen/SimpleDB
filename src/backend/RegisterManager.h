#ifndef __REGISTER_H__
#define __REGISTER_H__

#include <map>
#include <cassert>

class Table;

class RegisterManager {
private:
    std::map<int, Table *> list;

    RegisterManager() {}

    ~RegisterManager() {}

    RegisterManager(RegisterManager const &);

    RegisterManager &operator=(RegisterManager const &);

public:
    static RegisterManager &getInstance() {
        static RegisterManager instance;
        return instance;
    }

    void checkin(int permID, Table *table) {
        assert(list.find(permID) == list.end());
        list[permID] = table;
    }

    Table *getPtr(int permID) {
        assert(list.find(permID) != list.end());
        return list[permID];
    }

    void checkout(int permID) {
        list.erase(permID);
    }
};


#endif
