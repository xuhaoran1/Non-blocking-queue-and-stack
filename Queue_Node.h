//
// Created by pianzhikuang on 23-5-8.
//

#ifndef UNTITLED2_QUEUE_NODE_H
#define UNTITLED2_QUEUE_NODE_H
#include <atomic>
template<typename T>
struct Node_New
{
    Node_New(){}
    Node_New(const T &value) : data(value) { }
    T data;
    std::atomic<Node_New *>next = nullptr;
};
#endif //UNTITLED2_QUEUE_NODE_H
