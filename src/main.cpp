#include <fstream>
#include <iostream>

#include "sql/execute.h"
#include "record/DB.h"
#include "sql/parser.h"
#include "dbms/dbms.h"

int main(int argc, char const *argv[])
{
  assert(sizeof(TableHead) <= PAGE_SIZE);
  if(argc < 2){
    return start_parse(NULL); //read SQL from STDIN
  }else{
    DBMS::getInstance()->switchToDB(argv[1]); //first parameter is DB name
    return start_parse(NULL);
  }
  return 0;
}
