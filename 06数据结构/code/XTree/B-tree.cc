#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>

using namespace std;

// 一个B树的节点
class BTreeNode {
public:
    int *keys;      // 关键字数组
    int t;          // 最小度 (定义了节点关键字的数量限制)
    BTreeNode **C;  // 节点对应孩子节点的数组指针
    int n;          // 节点当前的关键字数量
    bool leaf;      // 当节点是叶子节点的时候为true 否则为false
public:
    BTreeNode(int _t, bool _leaf);   // 构造函数
 
    // 遍历所有以该节点为根的子树的关键字
    void traverse() {
        // 节点内共计n个关键字,n+1个孩子
        int i;
        for (i = 0; i < n; i++) {
            // 如果该节点不是叶子节点，在打印关键字之前，遍历子节点
            if (leaf == false)
                C[i]->traverse();
            cout << " " << keys[i];
        }
 
        // 打印最后一棵子树上的所有节点
        if (leaf == false)
            C[i]->traverse();
    }
 
    // 查询一个关键字在以该节点为根的子树上    
    BTreeNode *search(int k);   // 返回NULL如果这个关键字没有被找到
 
// 使BTree成为该节点的友元类,使我们能够访问BTree类的私有成员
friend class BTree;
};

// B树
class BTree {
    BTreeNode *root; // 根节点指针
    int t;           //最小度
public:
    // 构造函数 (初始化一个空树)
    BTree(int _t)
    {  root = NULL;  t = _t; }
 
    // 遍历该树的方法
    void traverse()
    {  if (root != NULL) root->traverse(); }
 
    // 在树中查询关键字
    BTreeNode* search(int k)
    {  return (root == NULL)? NULL : root->search(k); }
};

BTreeNode* search(BTreeNode* root, int k){
    int i = 1;
    while(i < root->n && k > root->keys[i])
        i += 1;

    if(i <= root->n && k == root->keys[i])
        return ;
    else if(root->leaf)
        return nullptr;
    else{
        return search(root->C[i], k);
    }
}