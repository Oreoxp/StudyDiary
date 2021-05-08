#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>
#include <queue>
#include "XBinTree.h"

using namespace std;


class RBNode
{
public:
    int   data;
    RBNode * left;
    RBNode * right;
    RBNode * parant;
    bool isRed;

public:
    RBNode(int d , RBNode * rbni, RBNode *l = nullptr, RBNode *r = nullptr,
        RBNode *p = nullptr, bool isR = false) 
        : data(d), left(rbni), right(rbni), parant(rbni), isRed(isR){
    }
};

typedef RBNode* RBTree;


RBTree InitRedBlackTree(int rootDate, RBTree rbnil){
    RBTree root = new RBNode(rootDate, rbnil);
    root->isRed = false;
    root->parant = rbnil;
    return root;
}

RBTree InitRedBlackTreeNil() {
    RBNode* rbnil = new RBNode(-1, nullptr);
    return rbnil;
}

/*          
            A                        B
           / \                      / \
          B   lA     ====>>       lB   A
         / \                          / \
        lB rB                        rB  lA
*/

RBTree RightRota(RBTree A) {
    cout << "RightRota:" <<A->data<< std::endl;
    RBTree B = A->left;
    A->left = B->right;
    B->right->parant = A;
    B->right = A;


    B->parant = A ->parant;

    if(A ->parant->left == A)
        A ->parant->left = B;
    else 
        A ->parant->right = B;

    A->parant = B;

    //color
    return B;
}

/*          
            A                        B
           / \                      / \
          lA   B     ====>>        A   rB
              / \                 / \
             lB rB               lA  lB
*/

RBTree LeftRota(RBTree A) {
    cout << "LeftRota:" <<A->data<< std::endl;
    RBTree B = A->right;
    A->right = B->left;
    B->left->parant = A;
    B->left = A;
    B->parant = A ->parant;
    if(A ->parant->left == A)
        A ->parant->left = B;
    else 
        A ->parant->right = B;
    A->parant = B;

    //color
    return B;
}

RBTree Find_Root(RBTree T, RBTree rbnil) {
    while(T->parant != rbnil){
        T = T->parant;
    }
    return T;
}


void leaveOrder(RBTree bt, RBTree rbnil) {
    queue<RBTree> rel; //定义一个队列，数据类型是二叉树指针，不要仅是int！！不然无法遍历
    rel.push(bt);
    int count = 0;
    while (!rel.empty())
    {
        RBTree front = rel.front();
        cout << "["<<front->data <<", p="<<front->parant->data<<",isred="<<front->isRed<<"]";
        rel.pop();                  //删除最前面的节点
        if (front->left != rbnil) //判断最前面的左节点是否为空，不是则放入队列
            rel.push(front->left);  
        if (front->right != rbnil)//判断最前面的右节点是否为空，不是则放入队列
            rel.push(front->right);

        count+=2;
        if(count == 2 || count == 6|| count == 14
        || count == 30)
            cout << endl;
    }
            cout << endl;
}

void RB_Insert_Fixup(RBTree T, RBNode* x, RBTree rbnil) {
    while(x->parant->isRed){
        if(x->parant == x->parant->parant->left){
            RBNode* y = x->parant->parant->right;
            if(y->isRed){
                x->parant->isRed = false;
                y->isRed = false;
                x->parant->parant->isRed = true;
                x = x->parant->parant;
                continue;
            }else if(x == x->parant->right){
                x = x->parant;
                LeftRota(x);
            }
            x->parant->isRed = false;
            x->parant->parant->isRed = true;
            RightRota(x->parant->parant);
            break;
        }else {
            //x->parant == x->p.p.right
            RBNode* y = x->parant->parant->left;
            if(y->isRed) {
                cout<<"x = "<< x->data<<"y = "<<y->data<<endl;
                x->parant->isRed = false;
                y->isRed = false;
                x->parant->parant->isRed = true;
                x = x->parant->parant;
                continue;
            }else if(x->parant == x->parant->left) {
                x = x->parant;
                RightRota(x);
            }
            x->parant->isRed = false;
            x->parant->parant->isRed = true;
            LeftRota(x->parant->parant);
            break;
        }
    }
    auto root = Find_Root(T, rbnil);
    root->isRed = false;
    cout<<"mid==========="<<endl;
    leaveOrder(root, rbnil);
    cout<<"midend========"<<endl;
}

void RB_Insert(RBTree T, RBNode* x, RBTree rbnil) {
    RBTree y = rbnil;
    RBTree root = Find_Root(T, rbnil);
    while(root != rbnil) {
        y = root;
        if(y->data > x->data) {
            root = root->left;
        }else{
            root = root->right;
        }
    }
    x->parant = y;
    if(y == rbnil) {
        cout << "error " << std::endl;
        T = x;
    }else if(x->data < y->data) {
        cout <<"把"<<x->data<<"放在"<<y->data<<"左边" << std::endl;
        y->left = x;
    }else {
        cout <<"把"<<x->data<<"放在"<<y->data<<"右边" << std::endl;
        y->right = x;
    }
    x->left = rbnil;
    x->right = rbnil;
    x->isRed = 1;
    RB_Insert_Fixup(T, x, rbnil);
}

int main(){
    //1 2 5 7 8 11 14
    cout<<"start main=============="<<endl;
    auto nil = InitRedBlackTreeNil();
    auto rbtree = InitRedBlackTree(1, nil);
    auto ab1 = new RBNode(2, nil);
    RB_Insert(rbtree, ab1, nil);
    auto ab2 = new RBNode(5, nil);
    RB_Insert(rbtree, ab2, nil);

    auto ab3 = new RBNode(7, nil);
    RB_Insert(rbtree, ab3, nil);
    
    auto ab4 = new RBNode(8, nil);
    RB_Insert(rbtree, ab4, nil);

    auto ab5 = new RBNode(11, nil);
    RB_Insert(rbtree, ab5, nil);
    auto ab6 = new RBNode(14, nil);
    RB_Insert(rbtree, ab6, nil);
    auto ab7 = new RBNode(4, nil);
    RB_Insert(rbtree, ab7, nil);
        
    /**/
    leaveOrder(Find_Root(rbtree, nil), nil);
    cout<<"start end==============="<<endl;

    return 0;
}
