

namespace JJ
{
template<typename T>
class allocator {
public:
    typedef T value_type;
    typedef T* pointer;
    typedef const T* const_pointer;
    typedef T& reference;
    typedef const T& const_reference;
    typedef std::size_t size_type;
    typedef std::ptrdiff_t difference_type;

    // rebind allocator of type U
    template<typename U>
    struct rebind {
        typedef allocator<U> other;
    };
public:
    allocator() {}
    ~allocator() {}
    static pointer allocate(size_type n, const void* hidt = 0) {
        T* tmp = new T[n];
        std::cout << "allocate, n=" << n << " p = "<< tmp << std::endl;
        return tmp;
    }
    static void deallocate(void* p, size_type n) {
        std::cout << "deallocate n = "<< n <<" p = "<< p <<  std::endl;
        delete[] static_cast<T*>(p);
    }
    static pointer address(T& x){
        return (pointer)&x;
    }
    size_type max_size() const{return 1000;}

    static void construct(T* p, const T& val){
        std::cout << "construct  in:"<< p << std::endl;
        new(p) T(val);
    }
    static void destroy(T* p){
        std::cout << "destroy  in:"<< p << std::endl;
    }
    
};

} // namespace JJ

