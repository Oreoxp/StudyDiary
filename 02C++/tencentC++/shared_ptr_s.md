# shared_ptr_s
这一篇是纯粹为了凑数， 之前已经说过了 shared_ptr不是线程安全的。参考 shared_ptr 那篇。 所以需要有个线程安全的版本，实现超级简单就是加了个锁
````c++

template<class T>
class shared_ptr_s final {
    //删掉了一些常规函数
public:
shared_ptr<T> lock() {
    shared_ptr<T> sptr;
    {
      lock_guard<spin_mutex> lk(sm_);
      sptr = sptr_;
    }
    return sptr;
  }
  
private:
  spin_mutex sm_;
  base::shared_ptr<T> sptr_;
};
````

内部持有一个std::shared_ptr， 然后有一个锁。调用lock 函数加个锁