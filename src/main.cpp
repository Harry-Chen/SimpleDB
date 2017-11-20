#include "dbms/dbms.h"

#ifdef __cplusplus
extern "C" char start_parse(const char *expr_input);
#endif

int main(int argc, char const *argv[]) {
    //assert(sizeof(TableHead) <= PAGE_SIZE);
    if (argc < 2) {
        return start_parse(nullptr); //read SQL from STDIN
    } else {
        DBMS::getInstance()->switchToDB(argv[1]); //first parameter is DB name
        return start_parse(nullptr);
    }
}
