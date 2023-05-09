cmake_minimum_required(VERSION 3.16)
project(untitled2)

set(CMAKE_CXX_STANDARD 11)

set(CMAKE_CXX_FLAGS "-pthread")
add_executable(main.cpp Node.h WithLockList.h LockFreeList.h LockFreeList2.h LockFreeQueue.h WithLockQueue.h WithTwoLockQueue.h Queue_Node.h)
