//
// Created by pianzhikuang on 23-5-8.
//

#ifndef UNTITLED2_WITHLOCKQUEUE_H
#define UNTITLED2_WITHLOCKQUEUE_H
#include "Node.h"
#include <mutex>
template<typename T>
class WithLockQueue
{
    std::mutex mtx;
    Node<T> head;
    Node<T> tail;
    int size{0};
public:
    WithLockQueue()
    {
        auto *node = new Node<T>();
        node->next = nullptr;
        head = tail = node;
    }
    void push(const T &value)
    {
        auto *node = new Node<T>(value);
        node->next = nullptr;
        {
            std::lock_guard<std::mutex> lock(mtx);
            tail->next = node;
            tail = node;
            size++;
        }
    }
    bool pop(T &value){
        std::lock_guard<std::mutex> lock(mtx);
        Node<T>* temp = head;
        Node<T>* new_head = temp->next;
        if(new_head== nullptr) {return false;}
        value = new_head->data;
        head = new_head;
        free(temp);//free可以不用锁
        size--;
        return true;
    }
};
#endif //UNTITLED2_WITHLOCKQUEUE_H
