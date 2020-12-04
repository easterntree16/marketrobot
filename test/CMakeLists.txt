
cmake_minimum_required(VERSION 3.14)

# Project
project(test)

MESSAGE(STATUS "CMAKE_SOURCE_DIR = ${CMAKE_SOURCE_DIR}")
MESSAGE(STATUS "CMAKE_BINARY_DIR = ${CMAKE_BINARY_DIR}")


include_directories(
    ../src/mrlib/component
   )


set(test_json test_json.cpp)
add_executable(test_json ${test_json})

set(test_config test_config.cpp)
add_executable(test_config ${test_config})
TARGET_LINK_LIBRARIES(test_config marketrobot)


set(test_log test_log.cpp) 
add_executable(test_log ${test_log})
TARGET_LINK_LIBRARIES(test_log marketrobot)


#这是多行注释开始
#[[


ADD_EXECUTABLE(test_task_scheduler 
		test_task_scheduler.cpp
		../common/task_scheduler.h
		../common/task_scheduler.cpp
		../common/Timer.cpp
		../common/Timer.h
)

TARGET_LINK_LIBRARIES(test_task_scheduler 
D:/BackTraderCN/Class3/BackTraderCN/cpp/third_party/googletest/googletest/msvc/2010/gtest/x64-Debug/gtest_maind.lib)

ADD_TEST(NAME test-task-scheduler COMMAND test_task_scheduler --gtest_output=xml:testresult.xml)




ADD_EXECUTABLE(test_event_loop 
		test_event_loop.cpp
		../common/task_scheduler.h
		../common/task_scheduler.cpp
		../common/event_loop.h
		../common/event_loop.cpp
		../common/Timer.cpp
		../common/Timer.h
)
TARGET_LINK_LIBRARIES(test_event_loop mmap)


]]
#这是多行注释结束