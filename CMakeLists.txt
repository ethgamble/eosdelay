cmake_minimum_required(VERSION 3.5)
project(eosdelay VERSION 1.0.0)

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
   set(TEST_BUILD_TYPE "Debug")
   set(CMAKE_BUILD_TYPE "Release")
else()
   set(TEST_BUILD_TYPE ${CMAKE_BUILD_TYPE})
endif()

if(EOSIO_CDT_ROOT STREQUAL "" OR NOT EOSIO_CDT_ROOT)
    set(EOSIO_CDT_ROOT "/usr/local/eosio.cdt")
endif()

list(APPEND CMAKE_MODULE_PATH ${EOSIO_CDT_ROOT}/lib/cmake)

include(EosioWasmToolchain)

include_directories(AFTER ${BOOST_ROOT}/include)
add_subdirectory("eosdelay")

if (APPLE)
   set(OPENSSL_ROOT "/usr/local/opt/openssl")
elseif (UNIX)
   set(OPENSSL_ROOT "/usr/include/openssl")
endif()
set(SECP256K1_ROOT "/usr/local")

