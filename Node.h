//
// Created by pianzhikuang on 23-5-8.
//

#ifndef UNTITLED2_NODE_H
#define UNTITLED2_NODE_H
template<typename T>
struct Node
{
    Node(){}
    Node(const T &value) : data(value) { }
    T data;
    Node *next = nullptr;
};
#endif //UNTITLED2_NODE_H
