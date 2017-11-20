set(SOURCE
${SOURCE}
${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
)

set(HEADERS
${HEADERS}
${CMAKE_CURRENT_SOURCE_DIR}/constants.h
)

add_subdirectory(record)
add_subdirectory(dbms)
add_subdirectory(sql)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCE} ${HEADER})
target_link_libraries(${CMAKE_PROJECT_NAME} SqlParser)
# add_dependencies(${CMAKE_PROJECT_NAME} SqlParser)