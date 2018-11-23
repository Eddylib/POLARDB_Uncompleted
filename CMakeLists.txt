cmake_minimum_required(VERSION 3.12)
project(engine)
set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_STANDARD 14)

include_directories(engine_example)
include_directories(engine_race)
include_directories(include)
include_directories(.)

add_library(engine
        engine_race/engine_race.cc
        engine_race/engine_race.h
        include/engine.h
        include/polar_string.h
        )


add_library(engine_example
        engine_example/data_store.cc
        engine_example/data_store.h
        engine_example/door_plate.cc
        engine_example/door_plate.h
        engine_example/engine_example.cc
        engine_example/engine_example.h
        engine_example/util.cc
        engine_example/util.h)

add_library(DBSystem
        engine_race/DBSystem/Index.cpp
        engine_race/DBSystem/Index.h
        engine_race/DBSystem/SkipList.cpp
        engine_race/DBSystem/SkipList.h
        engine_race/DBSystem/DBSystem.cpp
        engine_race/DBSystem/DBSystem.h
        engine_race/DBSystem/utils.cpp
        engine_race/DBSystem/utils.h
        engine_race/DBSystem/config.h
        engine_race/DBSystem/io.cpp
        engine_race/DBSystem/io.h
        engine_race/DBSystem/WaitQueue.h)

add_library(engine_race
        engine_race/engine_race.cc
        engine_race/engine_race.h
        )
#target_link_libraries(engine_race DBSystem)


#add_executable(test_real test/test.cc)
#target_link_libraries(test_real engine_race pthread)
#
#
#add_executable(test_example test/test.cc)
#target_link_libraries(test_example engine_example pthread)

add_executable(test_simple test_simple.cpp)
target_link_libraries(test_simple   DBSystem pthread)


add_executable(write test_db_write.cpp)
target_link_libraries(write   DBSystem pthread)

add_executable(read test_db_read.cpp)
target_link_libraries(read   DBSystem pthread)