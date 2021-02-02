#include "XList.h"

template <typename Object>
void XList<Object>::init(){
    theSize = 0;
    head = new Node();
    tail = new Node();
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
void XList<Object>::printfAll(){
    std::cout << "XList : printfAll : [";
    for(const_iterator itr = head; itr != tail; ++itr){
        std::cout << itr.data << "," ;
    }
    std::cout << "]" << endl;
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

template <typename Object>
typename Object::const_iterator begin(){
    return const_iterator(head->next);
}

template <typename Object>
typename Object::iterator end(){
    return iterator(tail->prev);
}

template <typename Object>
typename Object::const_iterator end(){
    return const_iterator(tail->prev);
}

template <typename Object>
int XList<Object>::size()const{
    return theSize;
}

template <typename Object>
bool XList<Object>::empty()const{
    return theSize <= 0;
}

template <typename Object>
void XList<Object>::clear()const{
    while( !empty( ) )
        pop_front( );
}

template <typename Object>
Object & XList<Object>::front() {
    return head->next->data;
}

template <typename Object>
const Object & XList<Object>::front() const{
    return head->next->data;
}

template <typename Object>
Object & XList<Object>::back() {
    return tail->prev->data;
}

template <typename Object>
const Object & XList<Object>::back() const{
    return tail->prev->data;
}

template <typename Object>
void XList<Object>::push_front(const Object & x){
    Node<Object> new_node = new Node(x);
    head->next->prev = new_node;
    new_node->next = head->next;
    new_node->prev = head;
    theSize ++;
}

template <typename Object>
void XList<Object>::push_back(const Object & x){
    Node<Object> new_node = new Node(x);
    tail->prev->next = new_node;
    new_node->prev = tail->prev;
    new_node->next = tail;
    tail->prev = new_node;
    theSize ++;
}

template <typename Object>
void XList<Object>::pop_front(){
    Node<Object> front_node = head->next;
    head->next = front_node->next;
    head->next->prev = head;
    delete front_node;
    theSize --;
}

template <typename Object>
void XList<Object>::pop_back(){
    Node<Object> back_node = tail->prev;
    tail->prev = back_node->prev;
    tail->prev->next = tail;
    delete back_node;
    theSize --;
}

template <typename Object>
typename::XList<Object>::iterator XList<Object>::insert(XList<Object>::iterator itr, const Object & x){
    if(!itr){
        return nullptr;
    }
    Node<Object> new_node = new Node(x);
    if (itr == head) {
        push_front(new_node);
    } else if(itr == tail) {
        push_back(new_node);
    } else {
        Node<Object> front_node = itr->prev;
        front_node->next = new_node;
        new_node->prev = front_node;
        new_node->next = *itr;
        itr->prev = new_node;
    }
    theSize ++;
    return itr;
}

template <typename Object>
typename::XList<Object>::iterator XList<Object>::erase(XList<Object>::iterator itr){
    if(!itr){
        return nullptr;
    }

    if (itr == head) {
        return nullptr;
    } else if(itr == tail) {
        return nullptr;
    } else {
        Node<Object> front_node = itr->prev;
        Node<Object> back_node = itr->next;
        front_node->next = back_node;
        back_node->prev = front_node;
        delete *itr;
        theSize --;
        return back_node;
    }
}

template <typename Object>
typename::XList<Object>::iterator XList<Object>::erase(XList<Object>::iterator start,XList<Object>::iterator end){
    if(!itr){
        return nullptr;
    }
    if (itr == head) {
        return nullptr;
    } else if(itr == tail) {
        return nullptr;
    } else {
        Node<Object> front_node = start->prev;
        Node<Object> back_node = end->next;
        front_node->next = back_node;
        back_node->prev = front_node;
        for(const_iterator itr = start; itr != end; ++itr){
            delete *itr;
            theSize --;
        }
        return back_node;
    }
}