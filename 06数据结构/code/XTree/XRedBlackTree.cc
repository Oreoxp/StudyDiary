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

void RB_Transplant(RBTree T, RBTree u, RBTree v, RBTree rbnil) {
    if(u->parant == rbnil){
        auto root = Find_Root(T, rbnil);
        root = v;
    }else if(u == u->parant->left) {
        u->parant->left = v;
    }else{
        u->parant->right = v;
    }
    v->parant = u->parant;
}

void RB_Delete_FixUp(RBTree T, RBNode* x, RBTree rbnil){
    RBTree w;//uncle
    auto root = Find_Root(T, rbnil);
    while (!x->isRed && x != root && x != rbnil) {
        if (x->parant->left == x) {
            w = x->parant->right;
            if (w->isRed) {
                // Case 1: x的兄弟w是红色的  
                w->isRed = false;
                x->parant->isRed = true;
                LeftRota(x->parant);
                w = x->parant->right;
            }

            if ( !w->left->isRed && !w->right->isRed) {
                // Case 2: x的兄弟w是黑色，且w的俩个孩子也都是黑色的  
                w->isRed = true;
                x = x->parant;
            } else if(!w->right->isRed) {
                w->left->isRed = false;
                w->isRed = true;
                RightRota(w);
                w = x->parant->right;
            }
            w->isRed = x->parant->isRed;
            x->parant->isRed = false;
            w->right->isRed = false;
            LeftRota(x->parant);
            x = Find_Root(T, rbnil);
        } else {//right
            w = x->parant->left;
            if(w->isRed){
                w->isRed = false;
                x->parant->isRed = true;
                RightRota(x->parant);
                w = x->parant->left;
            }
            if ( !w->left->isRed && !w->right->isRed) {
                // Case 2: x的兄弟w是黑色，且w的俩个孩子也都是黑色的  
                w->isRed = true;
                x = x->parant;
            } else if(!w->right->isRed) {
                w->right->isRed = false;
                w->isRed = true;
                LeftRota(w);
                w = x->parant->left;
            }
            w->isRed = x->parant->isRed;
            x->parant->isRed = false;
            w->left->isRed = false;
            RightRota(x);
            x = Find_Root(T, rbnil);
        }
    }
    x->isRed = false;
}

void RB_Delete(RBTree T, RBTree z, RBTree rbnil){
    RBTree y = z;
    RBTree x;
    bool oldyisRed = y->isRed;
    if(z->left == rbnil){
        x = z->right;
        RB_Transplant(T, z, z->right, rbnil);
    }else if(z->right == rbnil){
        x = z->left;
        RB_Transplant(T, z, z->left, rbnil);
    } else {
        y = y->right;
        while (y->left != rbnil)
            y = y->left;
        cout << "z的后继为：" << y->data << endl;
        oldyisRed = y->isRed;
        x = y->right;

        if(y->parant == z){
            x->parant = y;
        }else {
            RB_Transplant(T, y, y->right, rbnil);
            y->right = z->right;
            y->right->parant = y;
        }
        RB_Transplant(T, z, y, rbnil);
        y->left = z->left;
        y->left->parant = y;
        y->isRed = z->isRed;
    }

    if(!oldyisRed)
        RB_Delete_FixUp(T, x, rbnil);
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

    cout<<endl;
    cout<<endl;
    cout<<"insert end=============="<<endl;
    cout<<endl;
    cout<<endl;

    leaveOrder(Find_Root(rbtree, nil), nil);

    RB_Delete(rbtree, ab4, nil);
    RB_Delete(rbtree, ab5, nil);
    RB_Delete(rbtree, ab3, nil);
    RB_Delete(rbtree, ab2, nil);
    RB_Delete(rbtree, ab7, nil);
        
    /**/
    leaveOrder(Find_Root(rbtree, nil), nil);
    cout<<"start end==============="<<endl;

    return 0;
}
