//
// Created by pianzhikuang on 23-5-8.
//
#ifndef UNTITLED2_LOCKFREELIST_H
#define UNTITLED2_LOCKFREELIST_H
#include "Node.h"
#include <atomic>
/*头插和头pop的方法
*/
template<typename T>
class LockFreeList
{
    std::atomic<Node<T> *> head{nullptr};
    std::atomic<int> size{0};
public:
    void pushFront(const T &value)
    {
        auto *node = new Node<T>(value);
        node->next = head.load();
        while(!head.compare_exchange_weak(node->next, node)); //②
        size.fetch_add(1);
    }
    bool popFront(T &value)
    {
        Node<T>* temp = head.load();//留一个快照snapshot
        do {
            if (temp == nullptr) return false;//这句判断要在循环里面
        } while (!head.compare_exchange_weak(temp,temp->next));
        value = temp->data;//? before or after and tail behind this is a question
        delete temp;
        size.fetch_sub(1);
        return true;
    }
};
#endif //UNTITLED2_LOCKFREELIST_H