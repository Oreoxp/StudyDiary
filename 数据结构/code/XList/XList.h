#include <stdio.h>
#include <stdlib.h>
#include <stdexcept>
#include <iostream>

using namespace std;
template <typename Object>
class XList
{
private:
struct Node{
    Object data;
    Node   *prev;
    Node   *next;

    Node(const Object & d = Object(), Node *p = NULL, Node *n = NULL)
          : data( d ), prev( p ), next( n ) { }
};

public:
    class const_iterator
    {
      public:
  
        // Public constructor for const_iterator.
        const_iterator( ) : current( NULL )
          { }

        // Return the object stored at the current position.
        // For const_iterator, this is an accessor with a
        // const reference return type.
        const Object & operator* ( ) const
          { return retrieve( ); }
        
        const_iterator & operator++ ( )
        {
            current = current->next;
            return *this;
        }

        const_iterator operator++ ( int )
        {
            const_iterator old = *this;
            ++( *this );
            return old;
        }

        const_iterator & operator-- ( )
        {
            current = current->prev;
            return *this;
        }

        const_iterator operator-- ( int )
        {
            const_iterator old = *this;
            --( *this );
            return old;
        }
            
        bool operator== ( const const_iterator & rhs ) const
          { return current == rhs.current; }

        bool operator!= ( const const_iterator & rhs ) const
          { return !( *this == rhs ); }

      public:
        Node *current;

        // Protected helper in const_iterator that returns the object
        // stored at the current position. Can be called by all
        // three versions of operator* without any type conversions.
        Object & retrieve( ) const
          { return current->data; }

        // Protected constructor for const_iterator.
        // Expects a pointer that represents the current position.
        const_iterator( Node *p ) :  current( p )
          { }
        
        friend class XList<Object>;
    };

    class iterator : public const_iterator
    {
      public:

        // Public constructor for iterator.
        // Calls the base-class constructor.
        // Must be provided because the private constructor
        // is written; otherwise zero-parameter constructor
        // would be disabled.
        iterator( )
          { }

        Object & operator* ( )
          { return retrieve( ); }

        // Return the object stored at the current position.
        // For iterator, there is an accessor with a
        // const reference return type and a mutator with
        // a reference return type. The accessor is shown first.
        const Object & operator* ( ) const
          { return const_iterator::operator*( ); }
        
        iterator & operator++ ( )
        {
            current = current->next;
            return *this;
        }

        iterator operator++ ( int )
        {
            iterator old = *this;
            ++( *this );
            return old;
        }

        iterator & operator-- ( )
        {
            current = current->prev;
            return *this;
        }

        iterator operator-- ( int )
        {
            iterator old = *this;
            --( *this );
            return old;
        }

      protected:
        // Protected constructor for iterator.
        // Expects the current position.
        iterator( Node *p ) : const_iterator( p )
          { }

        friend class XList<Object>;
    };

public:
    explicit XList();
    XList(const XList & rhs);
    virtual ~XList();

    void printfAll();
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
    Node *head;
    Node *tail;

    void init();
};