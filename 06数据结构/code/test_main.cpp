#include <stdio.h>
#include <iostream>
#include "XVector/XVector.cpp"
#include "XList/XList.h"
#include "XList/XList.cc"

using namespace std;

int main(){
    cout  << "hello main ~~~~~!" << endl;

    XList<int> xv;

    xv.printfAll();


    for(int i = 0; i < 20; i++){
        xv.printfAll();
        xv.push_back(i); 
    }

    xv.printfAll();

    
    cout  << "bye main ~~~~~!" << endl;
    return 0;
}