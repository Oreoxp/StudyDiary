##### 原子访问（atomic access）

一个线程在访问某个资源的同事能够保证其他线程会在同一时刻访问同一资源。

【例】

```c++
long g_x = 0;

DWORD WINAPI ThreadFunc1(PVOID pvParam){
    g_x++;
    return{0};
}
DWORD WINAPI ThreadFunc2(PVOID pvParam){
    g_x++;
    return{0};
}
```

当一个线程执行ThreadFunc1，一个线程执行ThreadFunc2，由于windows的抢占式多任务系统，得到的最后结果g_x<u>可能为1，可能为2</u>.

这种不确定的结果对程序来说非常恐怖，我们需要有一种方法能够保证对一个值得递增操作是原子操作——也就是说，不会被打断。

**Interlocked系列函数**提供了我们需要的解决方案。

##### API:

```c++
LONG InterlockedExchangeAdd(
    PLONG volatile plAddend,
    LONG lIncrement
);

LONG LONG InterlockedExchangeAdd64(
    PLONGLONG volatile pllAddend,
    LONGLONG llIncrement
)
```

非常简单的方法，只要调用这个函数，穿一个长整型变量的地址和另一个增量值，函数就会保证递增操作是以原子方式进行的。

因此我们可以将上述例子代码改写：

【例：改】

```c++
long g_x = 0;

DWORD WINAPI ThreadFunc1(PVOID pvParam){
    InterlockedExchangeAdd(&g_x,1);
    return{0};
}
DWORD WINAPI ThreadFunc2(PVOID pvParam){
    InterlockedExchangeAdd(&g_x,1);
    return{0};
}
```

经过这个微小的改动，对g_x的递增会以原子方式进行，我们也因此能够保证g_x最终的值等于2.