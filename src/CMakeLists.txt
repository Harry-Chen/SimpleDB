set(SOURCE
        ${SOURCE}
        ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
        )

set(HEADERS
        ${HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/constants.h
        )

add_subdirectory(backend)
add_subdirectory(dbms)
add_subdirectory(sql_parser)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE} ${HEADERS})
target_link_libraries(${CMAKE_PROJECT_NAME} SqlParser)