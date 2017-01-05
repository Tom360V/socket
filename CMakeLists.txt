cmake_minimum_required(VERSION 2.8)

project (mySocket)

#Add related projects


#Add include directories
include_directories("inc")


#Add sources
set(srcs src/mysocket_x86.c)


#Add Library
add_library(mysocket_86 SHARED ${srcs})

#######################################

#if (TEST)
#  set(testsrcs test/test.c)
#  add_executable(fifo_test ${testsrcs})
#  target_link_libraries(fifo_test FIFO)
#endif()
