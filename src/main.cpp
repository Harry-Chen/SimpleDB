#include <cstring>
#include "dbms/DBMS.h"

#ifdef __cplusplus
extern "C" char start_parse(const char *expr_input);
#endif

bool initMode = false;

int main(int argc, char const *argv[]) {
    if (argc < 2) {
        return start_parse(nullptr); //read SQL from STDIN
    } else {
        if (argc == 3 && strcmp("init", argv[2]) == 0) {
            initMode = true;
        }
        DBMS::getInstance()->switchToDB(argv[1]); //first parameter is Database name
        return start_parse(nullptr);
    }
}
