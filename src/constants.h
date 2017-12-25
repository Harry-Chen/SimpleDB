#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

//----------------------FILE---------------------------------------
#define PAGE_SIZE 8192
#define PAGE_IDX 13
#define MAX_FILE_NUM 128
#define BUF_CAPACITY 60000
#define PAGE_FOOTER_SIZE 64
#define MAX_REC_PER_PAGE 512


//----------------------TABLE--------------------------------------
#define MAX_COLUMN_SIZE 32
// both table name and column name
#define MAX_NAME_LEN 128
#define MAX_DATA_SIZE 3000
#define MAX_CHECK 16
#define MAX_FOREIGN_KEY 24

//----------------------Database-----------------------------------------
#define MAX_TABLE_SIZE 32

#define DATE_FORMAT "%Y-%m-%d"

#endif
