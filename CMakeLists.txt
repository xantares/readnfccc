project (ReadNFCcc C)

cmake_minimum_required(VERSION 2.8)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

find_package(LibNFC 1.6 REQUIRED)

include_directories(${LIBNFC_INCLUDE_DIRS})

add_executable(readnfccc readnfccc.c)
target_link_libraries(readnfccc ${LIBNFC_LIBRARIES})

install (TARGETS readnfccc DESTINATION bin)

