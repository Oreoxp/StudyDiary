#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>
#include "../XVector/XVector.cpp"
//#include "../XList/XList.h"
//#include "../XList/XList.cc"

using namespace std;

template <typename Object>
struct BinNode {
    Object   element;
    BinNode* left;
    BinNode* right;

    BinNode(){
        cout  << "BinNode null~~~~~!" << endl;
        element = "";

        left = NULL;
        right = NULL;
    }
    BinNode(Object data){
    cout  << "BinNode ~~~~~!" << endl;
        element = data;
        left = NULL;
        right = NULL;
    }
};

template <typename Object>
bool empty(BinNode<Object>* bt) {
    std::cout  << "empty::element ="<< bt->element  <<",bool="<< (bt->element == "") << std::endl;
    return bt->element == "";
}

template <typename Object>
int  msize(BinNode<Object>* bt) {
    if(bt == NULL)
        return 0;
    return msize(bt->left) + msize(bt->right) + 1;
}

template <typename Object>
int  hight(BinNode<Object>* bt) {
    
    //cout  << "bt: " << bt << endl;
    if(bt == NULL) //树空
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
void midOrderEx(BinNode<Object>* bt) {
    BinNode<std::string> T;
    XVector< BinNode<std::string> > stack;
    {
        T.element = bt->element;
        T.left = bt->left;
        T.right = bt->right;
    }

    while(!empty(&T) || !stack.empty()){
        while (!empty(&T) ) {
            stack.push_back(T);
            {
                if(T.left == NULL)
                    break;
                T.element = T.left->element;
                T.right = T.left->right;
                T.left = T.left->left;
            }
        }
        {
            T.element = stack.back().element;
            T.left = stack.back().left;
            T.right = stack.back().right;
        }
        stack.pop_back();
        std::cout << "midOrderEx: " << T.element << std::endl;
        
        if (T.right != NULL) {
            T.element = T.right->element;
            T.left = T.right->left;
            T.right = T.right->right;
        } else {
            T = BinNode<std::string>();
        }
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


template <typename Object>
void leveOrder(BinNode<Object>* bt) {
    /*BinNode<std::string> T;
    XList< BinNode<std::string> > queue;
    if (empty(bt)) {
        return;
    }
    queue.push_back(*bt);

    while (!queue.empty())
    {
        {
            T.element = queue.front().element;
            T.left = queue.front().left;
            T.right = queue.front().right;
        }
        queue.pop_front();
        
        std::cout << "leveOrder: " << T.element << std::endl;

        if(T.left) 
            queue.push_back(*T.left);
        if(T.right) 
            queue.push_back(*T.right);
    }*/
    
}