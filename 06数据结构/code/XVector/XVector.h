#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>

using namespace std;

template <typename Object>
class XVector{
public:
    explicit XVector(int initSize = 0);
    XVector(const XVector & rhs);
    virtual ~XVector();

    const XVector & operator= (const XVector & rhs);
    Object & operator[] (int index);
    const Object & operator[] (int index ) const;

    void reSize(int newSize);
    void reServe(int newCapacity);
    bool empty() const;
    int  size() const;
    int  capacity() const;

    void push_back(const Object & x);
    void pop_back();

    void printfAll();
    
    const Object & back() const;

public:
    typedef Object * iterator;
    typedef const Object * const_iterator;

    //iterator 
    iterator begin();
    const_iterator begin()const;

    iterator end();
    const_iterator end()const;

    enum { SPARE_CAPACITY = 16 };

protected:
    Object * object_          = NULL;
    int      object_capacity_ = 0;
    int      object_siez_     = 0;
};