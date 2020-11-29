#include <stdio.h>
#include <iostream>
#include "XVector/XVector.cpp"

using namespace std;

int main(){
    cout  << "hello main ~~~~~!" << endl;

    XVector<int> xv;

    xv.printfAll();


    for(int i = 0; i < 20; i++){
        xv.printfAll();
        xv.push_back(i); 
    }

    xv.printfAll();

    
    cout  << "bye main ~~~~~!" << endl;
    return 0;
}