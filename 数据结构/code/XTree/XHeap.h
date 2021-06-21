#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>
//#include "XBinTree.h"

using namespace std;


typedef int element;

typedef struct HNode * Heap;
struct HNode {
    element * data;
    int size;
    int capacity;
};

typedef Heap MaxHeap;//最大堆
typedef Heap MinHeap;//最小堆

#define MAX_DATA INT_MAX

MaxHeap Create(int maxsize) {
    MaxHeap head = (MaxHeap)malloc(sizeof(MaxHeap));
    head->data = (element *)malloc((maxsize + 1 ) * sizeof(element));
    head->size = 0;
    head->capacity = maxsize;
    head->data[0] = MAX_DATA;

    return head;
}

//最大堆插入
//从新增的最后一个节点的父节点开始，用要插入的元素向上过滤上层节点
bool insert(MaxHeap h, element x){

    if(h->size == h->capacity){
        std::cout << "head is full" ;
        return false;
    }

    int i = ++h->size ;
    for( ;h->data[i/2] < x; i/=2) {
        h->data[i] = h->data[i/2];
    }     
    h->data[i] = x;
    return true;

}

//最大堆的删除
//从根节点开始，用最大堆中的最后一个元素向上过滤下层节点

element deleteMax(MaxHeap h) {
    int i = h->size;
    element preroot = h->data[1];
    h->data[1] = h->data[i];
    element root = h->data[1];

    std::cout << endl;
    std::cout << "deleteMax show: "<< endl;
    int j = 1;
    int size = --h->size;
    int operbit = 1;
    std::cout << "size: "<< size << endl;
    for(; (j*2 < size || j*2+1 < size)&&(h->data[j*2] > root || h->data[j*2+1] > root);) {
        if(h->data[j*2] > root) {//左子节点
            cout<<"left"<< j << ","
            <<"h->data[j] = " << h->data[j] << ","
            <<"h->data[j*2] = " << h->data[j*2]<<endl;
            h->data[j] = h->data[j*2];
            operbit = j;
            j*=2;
        }else if(h->data[j*2+1] > root){//右子节点
            cout<<"right : j="<< j  << ","
            <<"h->data[j] = " << h->data[j] << ","
            <<"h->data[j*2+1] = " << h->data[j*2+1]<<endl;
            h->data[j] = h->data[j*2+1];
            operbit = j;
            j=j*2+1;
        }
    }     
    cout<<"jump for operbit="<< operbit <<endl;
    h->data[j] = root;

    return preroot;
}

void showMaxHead(MaxHeap h) {
    
    /*for(int i = 0; i < h->size + 1; i++) {
        std::cout << h->data[i] << ",";
    }*/
    std::cout << endl;

    std::cout << "Max head show: "<< endl;
    for(int i = 1; i < h->size+ 1; i++) {
        std::cout << h->data[i] << ",";
        if ( i == 1 || i == 3 || i == 7 || i == 15){
            std::cout << endl;
        }
    }
    std::cout << endl;
}