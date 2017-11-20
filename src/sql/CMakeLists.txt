find_package(BISON REQUIRED)
find_package(FLEX REQUIRED)

BISON_TARGET(Parser ${CMAKE_CURRENT_SOURCE_DIR}/sql.y ${CMAKE_CURRENT_BINARY_DIR}/y.tab.c)
FLEX_TARGET(Lexer ${CMAKE_CURRENT_SOURCE_DIR}/sql.l ${CMAKE_CURRENT_BINARY_DIR}/lex.yy.c)

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
        ${CMAKE_CURRENT_SOURCE_DIR}/expr.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/execute.cpp
        PARENT_SCOPE
        )

set(HEADERS
        ${HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/type_def.h
        ${CMAKE_CURRENT_SOURCE_DIR}/expr.h
        ${CMAKE_CURRENT_SOURCE_DIR}/execute.h
        ${CMAKE_CURRENT_SOURCE_DIR}/parser.h
        PARENT_SCOPE
        )

MESSAGE(STATUS ${SOURCE})
MESSAGE(STATUS ${HEADERS})