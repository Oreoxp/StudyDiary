#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>
#include "XBinTree.h"

using namespace std;

typedef RBNode* RBTree;
struct RBNode {
    int   data;
    RBNode * left;
    RBNode * right;
    RBNode * parant;
    bool isRed;

    RBNode(int d = 0, RBNode *l = rbnil, RBNode *r = rbnil,
        RBNode *p = rbnil, bool isR = false)
        : data(d), left(l), right(r), parant(p), isRed(isR){
    }
};

RBNode* rbnil = new  RBNode();

RBTree InitRedBlackTree(int rootDate){
    RBTree root = new RBNode(rootDate);
    root->parant = rbnil;
    return root;
}

/*          
            A                        B
           / \                      / \
          B   lA     ====>>       lB   A
         / \                          / \
        lB rB                        rB  lA
*/

RBTree LeftRota(RBTree A) {
    RBTree B = A->left;
    A->left = B->right;
    B->right->parant = A;
    B->right = A;
    B->parant = A ->parant;
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

RBTree RightRota(RBTree A) {
    RBTree B = A->right;
    A->right = B->left;
    B->left->parant = A;
    B->left = A;
    B->parant = A ->parant;
    A->parant = B;

    //color
    return B;
}