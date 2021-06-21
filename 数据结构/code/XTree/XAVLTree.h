#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>
#include "XBinTree.h"

using namespace std;


typedef int element;


typedef struct AVLNode * positon;
typedef positon AVLTree;
struct AVLNode {
    element   data;
    AVLTree * left;
    AVLTree * right;
    int height;
};

int MAX(int a,int b) {
    return a > b ? a : b;
}

int  GetHeight(AVLTree * bt) {
    
    //cout  << "bt: " << bt << endl;
    if(bt == NULL) //树空
        return 0;
    //cout  << "goto  L!" << endl;
    int left_hight = GetHeight((*bt)->left);
    //cout  << "goto  R!" << endl;
    int right_hight = GetHeight((*bt)->right);

    //cout  << "start return" << endl;
    return left_hight > right_hight ? left_hight : right_hight;
}

AVLTree SingleLeftRotation(AVLTree A){
    //左单旋算法
    //A必须有个左子节点B

    AVLTree B = *A->left;
    A->left = B->right;
    B->right = &A;
    A->height = MAX(GetHeight(A->left), GetHeight(A->right)) + 1;
    B->height = MAX(GetHeight(B->left), A->height) + 1;

    return B;
}

AVLTree SingleRightRotation(AVLTree A){
    //右单旋算法
    //A必须有个右子节点B

    AVLTree B = *A->right;
    A->right = B->left;
    B->left = &A;
    A->height = MAX(GetHeight(A->left), GetHeight(A->right)) + 1;
    B->height = MAX(GetHeight(B->right), A->height) + 1;

    return B;
}

AVLTree DoubleLeftRightRotation(AVLTree A){
    //左右双旋算法
    //A必须有个左子节点B ， 且B有个右子节点C

    *A->left = SingleRightRotation(*A->left);//B 与 C 做右单旋 返回 C

    return SingleLeftRotation(A);//A 与 C 做左单旋 返回 C
}

AVLTree DoubleRightLeftRotation(AVLTree A){
    //右左双旋算法

    *A->right = SingleLeftRotation(*A->right);

    return SingleRightRotation(A);
}

AVLTree Insert(AVLTree T, element X){
    if(!T){
        T = (AVLTree)malloc(sizeof(AVLTree));
        T->data = X;
        T->height = 1;
        T->left = T->right = NULL;
    } else if(X < T->data){
        *(T->left) = Insert(*T->left, X);

        if(GetHeight(T->left)-GetHeight(T->right) == 2 ){
            if(X < (*T->left)->data)
                T = SingleLeftRotation(T);//左单旋
            else
                T = DoubleLeftRightRotation(T);//左右双旋
        }
    } else if(X > T->data){
        *(T->right) = Insert(*T->right, X);

        if(GetHeight(T->right)-GetHeight(T->left) == -2 ){
            if(X > (*T->right)->data)
                T = SingleLeftRotation(T);//右单旋
            else
                T = DoubleRightLeftRotation(T);//右左双旋
        }
    }//else x == t.data 无需插入

    T->height = MAX(GetHeight(T->left), GetHeight(T->right)) + 1;
    return T;
}
