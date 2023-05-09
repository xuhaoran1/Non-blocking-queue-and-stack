#pragma once
#include "WithLockList.h"
#include "LockFreeList.h"
#include "LockFreeList2.h"
#include <chrono>
#include <iostream>
using namespace std;
int main()
{
    const int SIZE = 1000000;
    //有锁测试
    auto start = chrono::steady_clock::now();
    WithLockList<int> wlList;
    for(int i = 0; i < SIZE; ++i)
    {
        wlList.pushFront(i);
    }
    auto end = chrono::steady_clock::now();
    chrono::duration<double, std::micro> micro = end - start;
    cout << "with lock list costs micro:" << micro.count() << endl;

    //无锁测试
    start = chrono::steady_clock::now();
    LockFreeList<int> lfList;
    for(int i = 0; i < SIZE; ++i)
    {
        lfList.pushFront(i);
    }
    end = chrono::steady_clock::now();
    micro = end - start;
    cout << "free lock list costs micro:" << micro.count() << endl;

    //无锁测试2
    start = chrono::steady_clock::now();
    LockFreeList2<int> lfList2;
    for(int i = 0; i < SIZE; ++i)
    {
        lfList2.pushFront(i);
    }
    end = chrono::steady_clock::now();
    micro = end - start;
    cout << "free lock list2 costs micro:" << micro.count() << endl;
    return 0;
}