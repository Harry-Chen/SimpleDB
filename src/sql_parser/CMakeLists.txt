find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

BISON_TARGET(Parser ${CMAKE_CURRENT_SOURCE_DIR}/sql.y ${CMAKE_CURRENT_BINARY_DIR}/sql.tab.c
        VERBOSE ${CMAKE_CURRENT_BINARY_DIR}/sql.output)
FLEX_TARGET(Lexer ${CMAKE_CURRENT_SOURCE_DIR}/sql.l ${CMAKE_CURRENT_BINARY_DIR}/sql.yy.c)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

ADD_FLEX_BISON_DEPENDENCY(Lexer Parser)

add_library(SqlParser
        ${BISON_Parser_OUTPUT_SOURCE}
        ${FLEX_Lexer_OUTPUTS}
        ${BISON_Parser_OUTPUT_HEADER}
        )

set(SOURCE
        ${SOURCE}
        ${CMAKE_CURRENT_SOURCE_DIR}/Expression.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Execute.cpp
        PARENT_SCOPE
        )

set(HEADERS
        ${HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/type_def.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Expression.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Execute.h
        PARENT_SCOPE
        )
