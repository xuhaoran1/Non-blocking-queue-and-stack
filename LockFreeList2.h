////
//// Created by pianzhikuang on 23-5-8.
////
//
//#ifndef UNTITLED2_LOCKFREELIST2_H
//#define UNTITLED2_LOCKFREELIST2_H
//#include "Node.h"
//#include <atomic>
//template<typename T>
//class LockFreeList2
//{
//    std::atomic<Node<T> *> head;
//public:
//    void pushFront(const T &value)
//    {
//        auto *node = new Node<T>(value);
//        node->next = nullptr;
//        Node<T>* head_temp = head.load();
//        do{
//            node->next = head_temp;
//        } while (head.compare_exchange_weak(head_temp,node));
//    }
//};
//#endif //UNTITLED2_LOCKFREELIST2_H
