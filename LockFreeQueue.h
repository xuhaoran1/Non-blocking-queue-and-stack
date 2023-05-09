//
// Created by pianzhikuang on 23-5-8.
//
//C++->next要用，所以要搞一下atomic,如果不用C++,直接用C确实简单很多，或者用C直接写地址然后强转类型
#ifndef UNTITLED2_LOCKFREEQUEUE_H
#define UNTITLED2_LOCKFREEQUEUE_H
#include "Queue_Node.h"
#include <atomic>
template<typename T>
//size感觉意义不大，毕竟是和数据不一致的
class LockFreeQueue
{
    std::atomic<Node_New<T> *> head;
    std::atomic<Node_New<T> *> tail;
    std::atomic<int> size;
public:
    LockFreeQueue()
    {
        auto *node = new Node_New<T>();
        node->next = nullptr;
        head.store(node);
        tail.store(node);
    }
    void push(const T &value)
    {
        auto *node = new Node_New<T>(value);
        node->data = value;     //其实要保证对齐
        node->next.store(nullptr);
        Node_New<T> *temp_tail;
        for(;;){
            temp_tail = tail.load();
            std::atomic<Node_New<T> *>temp_next = temp_tail->next;
            Node_New<T> * next_ptr = temp_next.load();
            if(temp_tail==tail.load()){
                if(next_ptr== nullptr){
                    if(temp_next.compare_exchange_strong(next_ptr,node)){//boost是用的weak
                        break;
                    }
                }
                else{
                    tail.compare_exchange_strong(temp_tail,next_ptr);
                }
            }
        }
        tail.compare_exchange_strong(temp_tail,node);
    }
    bool pop(T &value) {
        Node_New<T> *temp_head;
        for (;;) {
            temp_head = head.load();
            Node_New<T> *temp_tail = tail.load();
            std::atomic<Node_New<T> *> temp_next = temp_head->next;
            if (temp_head == head.load()) {
                if (temp_head == temp_tail) {
                    if (temp_next.load() == nullptr) { return false; }
                    tail.compare_exchange_strong(temp_tail, temp_next.load());
                } else {
                    value = temp_next.load()->data;//这行是否是原子的，不重要
                    if (head.compare_exchange_strong(temp_head, temp_next.load())) {
                        break;
                    }
                }
            }
        }
        delete temp_head;
        return true;
    }
};
#endif //UNTITLED2_LOCKFREEQUEUE_H
