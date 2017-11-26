set(HEADERS
        ${HEADERS}
        ${CMAKE_CURRENT_SOURCE_DIR}/Compare.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Database.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Index.h
        ${CMAKE_CURRENT_SOURCE_DIR}/RegisterManager.h
        ${CMAKE_CURRENT_SOURCE_DIR}/Table.h
        PARENT_SCOPE
        )

set(SOURCE
        ${SOURCE}
        ${CMAKE_CURRENT_SOURCE_DIR}/Compare.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Database.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Index.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/Table.cpp
        PARENT_SCOPE
        )