add_library(test_suite ${CMAKE_SOURCE_DIR}/src/constants.h)
target_link_libraries(test_suite gtest gtest_main ${CMAKE_PROJECT_NAME}_lib)

add_executable(const_test const_test.cc)
target_link_libraries(const_test test_suite)
add_test(NAME TestConstants COMMAND const_test)

#add_executable(io_test io_test.cc)
#target_link_libraries(io_test test_suite)
#add_test(NAME TestIO COMMAND io_test)

add_executable(strlike_test strlike_test.cc)
target_link_libraries(strlike_test test_suite)
add_test(NAME TestStrLIKE COMMAND strlike_test)

add_executable(table_test table_test.cc)
target_link_libraries(table_test test_suite)
add_test(NAME TestTable COMMAND table_test)