cmake_minimum_required(VERSION 2.8)

project (mySocket)

#Add related projects


#Add include directories
include_directories("inc")


#Add Library
add_library(mySocket_x86 SHARED src/mysocket_x86.c)
add_library(client       SHARED src/clientSocket.c)

target_link_libraries(client mySocket_x86)
#######################################

#if (TEST)
#  set(testsrcs test/test.c)
#  add_executable(fifo_test ${testsrcs})
#  target_link_libraries(fifo_test FIFO)
#endif()
