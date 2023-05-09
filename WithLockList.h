//
// Created by pianzhikuang on 23-5-8.
//
#ifndef UNTITLED2_WITHLOCKLIST_H
#define UNTITLED2_WITHLOCKLIST_H
#include "Node.h"
#include <mutex>
template<typename T>
class WithLockList
{
    std::mutex mtx;
    Node<T> *head{nullptr};
    int size{0};
public:
    void pushFront(const T &value)
    {
        auto *node = new Node<T>(value);
        {
            std::lock_guard<std::mutex> lock(mtx);
            node->next = head;
            head = node;
            size++;
        }
    }
    bool popFront(T &value){
        Node<T>* temp = head;
        {
            std::lock_guard<std::mutex> lock(mtx);
            if(head== nullptr) return false;
            head = head->next;
            value = temp->data;
            free(temp);
            size--;
        }
        return true;
    }
};
#endif //UNTITLED2_WITHLOCKLIST_H
