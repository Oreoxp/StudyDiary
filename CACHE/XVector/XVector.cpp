#include "XVector.h"

using namespace std;

template <typename Object>
XVector<Object>::XVector( int size ){
    std::cout << "XVector :  arge : size=" << size << std::endl;
    object_siez_ = size;
    object_capacity_ = SPARE_CAPACITY + size;
    object_ = new Object[object_capacity_];
    std::cout << "XVector :  init over : size = " << object_siez_ 
              << ",capacity = " << object_capacity_ 
              << ",object = " << object_ 
              << std::endl;
}

template <typename Object>
XVector<Object>::XVector(const XVector & rhs){
    std::cout << "XVector :  "<< std::endl;
    operator=(rhs);
}

template <typename Object>
XVector<Object>::~XVector(){
    std::cout << "XVector : ~XVector : "<< std::endl;
    delete [] object_;
}

template <typename Object>
const XVector<Object> & XVector<Object>::operator= (const XVector & rhs){
    std::cout << "XVector : operator= : "<< std::endl;

    if (this != &rhs ){
        delete [] object_;
        object_siez_ = rhs.size();
        object_capacity_ = rhs.capacity();

        object_ = new Object(capacity());
        for(int i = 0 ;i < size(); i++)
            object_[i] = rhs[i];
    }

    return *this;
}

template <typename Object>
Object & XVector<Object>::operator[] (int index){
    std::cout << "XVector : operator[] : "<< std::endl;
    return object_[index];
}

template <typename Object>
const Object & XVector<Object>::operator[] (int index ) const{
    std::cout << "XVector : c operator[] : "<< std::endl;
    return object_[index];
}

template <typename Object>
void XVector<Object>::reSize(int newSize){
    std::cout << "XVector : reSize start : oldSize = " << object_siez_ 
              <<",newSize = "<< newSize 
              << std::endl;

    if (newSize > object_capacity_)
        reServe(newSize * 2 + 1);

    object_siez_ = newSize;

   std::cout << "XVector :  reSize over : size = " << object_siez_ 
              << ",capacity = " << object_capacity_ 
              << ",object = " << object_ 
              << std::endl;
}

template <typename Object>
void XVector<Object>::reServe(int newCapacity){
    std::cout << "XVector : reServe : old capacity = " 
              << object_capacity_  
              << ",old size = " << object_siez_ 
              << ",newCapacity = " << newCapacity 
              << std::endl;

    if (newCapacity < object_siez_)
        return;
    Object *old_object = object_;

    object_ = new Object[newCapacity];
    for(int i = 0; i < size(); i++)
        object_[i] = old_object[i];
    
    object_capacity_ = newCapacity;
    delete [] old_object;

    std::cout << "XVector : reServe over : capacity = " 
            << object_capacity_  
            << ",old size = " << object_siez_ 
            << std::endl;
}

template <typename Object>
void XVector<Object>::printfAll(){
    std::cout << "XVector : printfAll : [";

    for(int i = 0; i < size(); i++)
        std::cout << object_[i] << "," ;

    std::cout << "]" << endl;
}

template <typename Object>
bool XVector<Object>::empty() const{
    std::cout << "XVector : empty : empty = " << size() << std::endl;
    return size() == 0;
}

template <typename Object>
int  XVector<Object>::size() const{
    //std::cout << "XVector : size : size = " << object_siez_ << std::endl;
    return object_siez_;
}

template <typename Object>
int  XVector<Object>::capacity() const{
    std::cout << "XVector : capacity : capacity =  " << object_capacity_ << std::endl;
    return object_capacity_;
}

template <typename Object>
void XVector<Object>::push_back(const Object & x){
    std::cout << "XVector : push_back : " << std::endl;
    if (object_siez_ == object_capacity_)
        reServe( 2 * object_capacity_ + 1 );
    object_[object_siez_++] = x;
}

template <typename Object>
void XVector<Object>::pop_back(){
    std::cout << "XVector : pop_back : " << std::endl;
    object_siez_--;
}

template <typename Object>
const Object & XVector<Object>::back() const{
    std::cout << "XVector : back : " << std::endl;
    return object_[object_siez_ - 1 ];
}

//iterator 
template <typename Object>
Object * XVector<Object>::begin(){
    std::cout << "XVector : begin : " << std::endl;
    return &object_[0];
}

template <typename Object>
const Object * XVector<Object>::begin()const{
    std::cout << "XVector : c begin : " << std::endl;
    return &object_[0];
}

template <typename Object>
Object * XVector<Object>::end(){
    std::cout << "XVector : end : " << std::endl;
    return object_[object_siez_ ];
}

template <typename Object>
const Object * XVector<Object>::end()const{
    std::cout << "XVector : c end : " << std::endl;
    return object_[object_siez_ ];
}