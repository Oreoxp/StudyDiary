#include "XList.h"

template <typename Object>
void XList<Object>::init(){
    theSize = 0;
    head = new Node;
    tail = new Node;
    head->next = tail;
    tail->prev = head;
}

template <typename Object>
XList<Object>::XList(){
    std::cout << "XList : " << std::endl;
    init();
}

template <typename Object>
XList<Object>::XList(const XList & rhs){
    std::cout << "XVector :  "<< std::endl;
    init();
    *this = rhs;
}

template <typename Object>
XList<Object>::~XList(){
    std::cout << "XList : ~XList : "<< std::endl;
    clear();
    delete head;
    delete tail;
}

template <typename Object>
const XList<Object> & XList<Object>::operator=(const XList & rhs){
    if(this == &rhs)
        return *this;
    clear();
    for(const_iterator itr = rhs.begin(); itr != rhs.end(); ++itr){
        push_back( *itr );
    }
    return *this;
}

template <typename Object>
typename Object::iterator begin(){
    return iterator(head->next);
}

