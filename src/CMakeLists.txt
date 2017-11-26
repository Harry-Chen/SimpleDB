set(HEADERS
        ${HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/constants.h
        )

add_subdirectory(backend)
add_subdirectory(dbms)
add_subdirectory(sql_parser)

add_library(${CMAKE_PROJECT_NAME}_lib  ${SOURCE} ${HEADERS})
add_executable(${CMAKE_PROJECT_NAME} main.cpp)
target_link_libraries(${CMAKE_PROJECT_NAME} SqlParser ${CMAKE_PROJECT_NAME}_lib)