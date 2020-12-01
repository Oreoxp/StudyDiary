#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>

using namespace std;

template <typename Object>
class XList{
public:
    explicit XList();
    XList(const XList & rhs);
    virtual ~XList();
};
