#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>

using namespace std;

template <typename Object>
struct BinNode {
    Object   element;
    BinNode* left;
    BinNode* right;

    BinNode(Object data){
    cout  << "BinNode ~~~~~!" << endl;
        element = data;
        left = nullptr;
        right = nullptr;
    }
};

template <typename Object>
bool empty(BinNode<Object>* bt) {
    return bt == nullptr;
}

template <typename Object>
int  msize(BinNode<Object>* bt) {
    if(bt == nullptr)
        return 0;
    return msize(bt->left) + msize(bt->right) + 1;
}

template <typename Object>
int  hight(BinNode<Object>* bt) {
    
    //cout  << "bt: " << bt << endl;
    if(bt == nullptr) //树空
        return 0;
    //cout  << "goto  L!" << endl;
    int left_hight = hight(bt->left);
    //cout  << "goto  R!" << endl;
    int right_hight = hight(bt->right);

    //cout  << "start return" << endl;
    return left_hight > right_hight ? left_hight : right_hight;
}

template <typename Object>
void preOrder(BinNode<Object>* bt) {
    if(bt){
        std::cout << "preOrder: " << bt->element << std::endl;
        preOrder(bt->left);
        preOrder(bt->right);
    }
}

template <typename Object>
void midOrder(BinNode<Object>* bt) {
    if(bt){
        midOrder(bt->left);
        std::cout << "midOrder: " << bt->element << std::endl;
        midOrder(bt->right);
    }
}

template <typename Object>
void postOrder(BinNode<Object>* bt) {
    if(bt){
        postOrder(bt->left);
        postOrder(bt->right);
        std::cout << "postOrder: " << bt->element << std::endl;
    }
}
