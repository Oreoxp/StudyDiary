#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>

using namespace std;

template <typename Object>
struct Node{
    Object data;
    Node   *prev;
    Node   *next;

    Node(const Object & d = Object(), Node *p = NULL, Node *n = NULL);
};

template <typename Object>
class XList{

    class iterator;
    class const_iterator;
    
public:
    explicit XList();
    XList(const XList & rhs);
    virtual ~XList();

    const XList & operator=(const XList & rhs);

    iterator begin();
    const_iterator begin()const;
    iterator end();
    const_iterator end()const;

    int size() const;
    bool empty() const;
    void clear() const;

    Object & front();
    const Object & front()const;
    Object & back();
    const Object & back()const;

    void push_front(const Object & x);
    void push_back(const Object & x);
    void pop_front();
    void pop_back();

    iterator insert(iterator itr, const Object & x);

    iterator erase(iterator itr);

    iterator erase(iterator start, iterator end);

private:
    int  theSize;
    Node<Object> *head;
    Node<Object> *tail;

    void init();
};

template <typename Object>
class const_iterator{
public:
    const_iterator();
    const Object & operator*();
    const_iterator & operator++();
    const_iterator operator++(int );
    bool operator==(const const_iterator & rhs);
    bool operator!=(const const_iterator & rhs);

protected:
    Node<Object> *current;

    Object & retrieve();

    const_iterator(Node<Object> *p);

    friend class XList<Object>;
};

template <typename Object>
class iterator : public const_iterator<Object>
{
public:
    iterator();
    Object & operator*();
    iterator & operator++();
    iterator operator++(int );

protected:
    iterator(Node<Object> *p);

    friend class XList<Object>;
};