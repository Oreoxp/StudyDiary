#include <stdio.h>
#include <iostream>
//#include "XVector/XVector.cpp"
//#include "XList/XList.h"
//#include "XList/XList.cc"
//#include "Xtree/XBinTree.h"
//#include "Xtree/XAVLTree.h"
#include "Xtree/XHeap.h"

using namespace std;

int main(){
    cout  << "hello main ~~~~~!" << endl;

    /*XList<int> xv;

    xv.printfAll();


    for(int i = 0; i < 20; i++){
        xv.printfAll();
        xv.push_back(i); 
    }

    xv.printfAll();
        BinNode<string>* root = new BinNode<string>("A");

        BinNode<string>* nodeB = new BinNode<string>("B");
        BinNode<string>* nodeC = new BinNode<string>("C");
        BinNode<string>* nodeD = new BinNode<string>("D");
        BinNode<string>* nodeE = new BinNode<string>("E");
        BinNode<string>* nodeF = new BinNode<string>("F");
        root->left = nodeB;
        root->right = nodeC;
        nodeB->left = nodeD;
        nodeB->right = nodeE;
        nodeC->left = nodeF;
        */
        /*cout  << "--------root---------" << root<< endl;
        cout  << "--------nodeB--------" << nodeB<< endl;
        cout  << "--------nodeC--------" << nodeC<< endl;
        cout  << "--------nodeD--------" << nodeD<< endl;
        cout  << "--------nodeE--------" << nodeE<< endl;
        cout  << "--------nodeF--------" << nodeF<< endl;

        std::cout  << "start oper ~~~~~!" << std::endl;

        int height = hight(root);
        std::cout  << "height: " << height << std::endl;

        int size = msize(root);
        std::cout  << "size: " << size << std::endl;

        //std::cout  << "--------preOrder--------" << std::endl;
        //preOrder(root);

        //std::cout  << "--------midOrder--------" << std::endl;
        //midOrder(root);
        //std::cout  << "--------midOrderEx--------" << std::endl;
        //midOrderEx(root);

        //std::cout  << "--------postOrder-------" << std::endl;
        //postOrder(root);

        
        std::cout  << "--------leveOrder--------" << std::endl;
        leveOrder(root);
*/
    
    MaxHeap mh = Create(30);


    insert(mh, 58);
    insert(mh, 25);
    insert(mh, 44);
    insert(mh, 18);
    insert(mh, 10);
    insert(mh, 31);
    insert(mh, 110);
    insert(mh, 24);
    insert(mh, 48);
    insert(mh, 55);
    insert(mh, 53);
    insert(mh, 59);
    insert(mh, 66);
    insert(mh, 77);
    insert(mh, 88);
    showMaxHead(mh);
    deleteMax(mh);
    showMaxHead(mh);


    cout  << "bye main ~~~~~!" << endl;

    /*int x;
    while (getchar()!=48)
    {
        scanf("%d",&x);
        printf("%d",x);
    }*/
    
    return 0;
}