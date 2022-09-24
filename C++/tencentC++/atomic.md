# atomic

C++11 中 atomic 标准库中新支持的几种 memory order，规定了一些指令重排方面的限制，仅说明下用到的三种：

1. `std::memory_order_relaxed`：不对重排做限制，只保证相关共享内存访问的原子性。
2. `std::memory_order_acquire`: 用在 load 时，保证同线程中该 load 之后的对相关内存读写语句不会被重排到 load 之前，并且其他线程中对同样内存用了 store release 都对其可见。
3. `std::memory_order_release`：用在 store 时，保证同线程中该 store 之后的对相关内存的读写语句不会被重排到 store 之前，并且该线程的所有修改对用了 load acquire 的其他线程都可见。



| Value                  | Explanation                                                  |
| ---------------------- | ------------------------------------------------------------ |
| `memory_order_relaxed` | 不对重排做限制，只保证相关共享内存访问的原子性。 (see [Relaxed ordering](https://en.cppreference.com/w/cpp/atomic/memory_order#Relaxed_ordering) below) |
| `memory_order_consume` | A load operation with this memory order performs a *consume operation* on the affected memory location: no reads or writes in the current thread dependent on the value currently loaded can be reordered before this load. Writes to data-dependent variables in other threads that release the same atomic variable are visible in the current thread. On most platforms, this affects compiler optimizations only (see [Release-Consume ordering](https://en.cppreference.com/w/cpp/atomic/memory_order#Release-Consume_ordering) below) |
| `memory_order_acquire` | A load operation with this memory order performs the *acquire operation* on the affected memory location: no reads or writes in the current thread can be reordered before this load. All writes in other threads that release the same atomic variable are visible in the current thread (see [Release-Acquire ordering](https://en.cppreference.com/w/cpp/atomic/memory_order#Release-Acquire_ordering) below) |
| `memory_order_release` | A store operation with this memory order performs the *release operation*: no reads or writes in the current thread can be reordered after this store. All writes in the current thread are visible in other threads that acquire the same atomic variable (see [Release-Acquire ordering](https://en.cppreference.com/w/cpp/atomic/memory_order#Release-Acquire_ordering) below) and writes that carry a dependency into the atomic variable become visible in other threads that consume the same atomic (see [Release-Consume ordering](https://en.cppreference.com/w/cpp/atomic/memory_order#Release-Consume_ordering) below). |
| `memory_order_acq_rel` | A read-modify-write operation with this memory order is both an *acquire operation* and a *release operation*. No memory reads or writes in the current thread can be reordered before the load, nor after the store. All writes in other threads that release the same atomic variable are visible before the modification and the modification is visible in other threads that acquire the same atomic variable. |
| `memory_order_seq_cst` | A load operation with this memory order performs an *acquire operation*, a store performs a *release operation*, and read-modify-write performs both an *acquire operation* and a *release operation*, plus a single total order exists in which all threads observe all modifications in the same order (see [Sequentially-consistent ordering](https://en.cppreference.com/w/cpp/atomic/memory_order#Sequentially-consistent_ordering) below) |











**Memory Order**

内存顺序描述了计算机 CPU 获取内存的顺序，内存的排序既可能发生在编译器编译期间，也可能发生在 CPU 指令执行期间。

为了尽可能地提高计算机资源利用率和性能，编译器会对代码进行重新排序， CPU 会对指令进行重新排序、延缓执行、各种缓存等等，以达到更好的执行效果。当然，任何排序都不能违背代码本身所表达的意义，并且在单线程情况下，通常不会有任何问题。

但是在多线程环境下，比如无锁（lock-free）数据结构的设计中，指令的乱序执行会造成无法预测的行为。所以我们通常引入内存栅栏（Memory Barrier）这一概念来解决可能存在的并发问题。



**Memory Barrier**

内存栅栏是一个令 CPU 或编译器在内存操作上限制内存操作顺序的指令，通常意味着在 barrier 之前的指令一定在 barrier 之后的指令之前执行。



在 C11/C++11 中，引入了六种不同的 memory order，可以让程序员在并发编程中根据自己需求尽可能降低同步的粒度，以获得更好的程序性能。这六种 order 分别是：

```text
relaxed, acquire, release, consume, acq_rel, seq_cst
```



*memory_order_relaxed:* 只保证当前操作的原子性，不考虑线程间的同步，其他线程可能读到新值，也可能读到旧值。比如 C++ shared_ptr 里的引用计数，我们只关心当前的应用数量，而不关心谁在引用谁在解引用。



*memory_order_release:*（可以理解为 mutex 的 unlock 操作）

1. 对**写入**施加 release 语义（store），在代码中这条语句前面的所有读写操作都无法被重排到这个操作之后，即 store-store 不能重排为 store-store, load-store 也无法重排为 store-load
2. 当前线程内的**所有**写操作，对于其他对这个原子变量进行 acquire 的线程可见
3. 当前线程内的**与这块内存有关**的**所有**写操作，对于其他对这个原子变量进行 consume 的线程可见



*memory_order_acquire:* （可以理解为 mutex 的 lock 操作）

1. 对**读取**施加 acquire 语义（load），在代码中这条语句后面所有读写操作都无法重排到这个操作之前，即 load-store 不能重排为 store-load, load-load 也无法重排为 load-load
2. 在这个原子变量上施加 release 语义的操作发生之后，acquire 可以保证读到所有在 release 前发生的写入，举个例子：

```text
c = 0;

thread 1:
{
  a = 1;
  b.store(2, memory_order_relaxed);
  c.store(3, memory_order_release);
}

thread 2:
{
  while (c.load(memory_order_acquire) != 3)
    ;
  // 以下 assert 永远不会失败
  assert(a == 1 && b == 2);
  assert(b.load(memory_order_relaxed) == 2);
}
```



*memory_order_consume:*

1. 对当前**要读取的内存**施加 release 语义（store），在代码中这条语句后面所有**与这块内存有关的**读写操作都无法被重排到这个操作之前
2. 在这个原子变量上施加 release 语义的操作发生之后，consume 可以保证读到所有在 release 前发生的**并且与这块内存有关的**写入，举个例子：

```text
a = 0;
c = 0;

thread 1:
{
  a = 1;
  c.store(3, memory_order_release);
}

thread 2:
{
  while (c.load(memory_order_consume) != 3)
    ;
  assert(a == 1); // assert 可能失败也可能不失败
}
```



*memory_order_acq_rel:*

1. 对读取和写入施加 acquire-release 语义，无法被重排
2. 可以看见其他线程施加 release 语义的所有写入，同时自己的 release 结束后所有写入对其他施加 acquire 语义的线程可见



*memory_order_seq_cst:*（顺序一致性）

1. 如果是读取就是 acquire 语义，如果是写入就是 release 语义，如果是读取+写入就是 acquire-release 语义
2. 同时会对所有使用此 memory order 的原子操作进行同步，所有线程看到的内存操作的顺序都是一样的，就像单个线程在执行所有线程的指令一样

通常情况下，默认使用 *memory_order_seq_cst*，所以你如果不确定怎么这些 memory order，就用这个。



以上就是这六种 memory_order 的简单介绍，除此之外还有些重要的概念，比如 sequence-before, happens-before 等等，具体可以参考 [std::memory_order - cppreference.com](https://link.zhihu.com/?target=https%3A//en.cppreference.com/w/cpp/atomic/memory_order)。



**RocksDB SkipList Memory Order**

下面我们结合代码具体看下 RocksDB SkipList 中的 memory order 使用。这部分内容需要你提前熟悉下相关代码。



RocksDB SkipList 支持一写多读。它涉及了三种 memory order，包括 relaxed, release 和 acquire。

一写多读有以下几点限制：

```text
1. 写入会在外部进行同步
2. 读取期间 SkipList 不会被销毁
3. SkipList 节点一旦被插入，不会被删除，除非 SkipList 被销毁
4. SkipList 节点一旦被插入，除了 next 域会变更外，其他域不会改变
```

我们把所有涉及 memory order 的操作分为三类：

1. SkipList 的 max_height_，代表跳跃表的高度。这个值始终使用 relaxed 语义去进行读写，并且只有在插入的时候才可能会改变。因为只有一个写线程，所以对写来说不会读到旧值；对于读，我们一一分析：

```text
1. 读到旧的 max_height_：不影响查找，我们可能读取到新插入的节点也可能读不到
2. 读到新的 max_height_：
  a. 读到 head_ 指向的旧节点，那么当我们查找 key 时，会发现 head_ 指向 nullptr，那么会立即下降到下一层
  b. 读到 head_ 指向的新插入节点，那么会使用这个新节点进行查找
```



\2. SkipList 的节点写操作。

```text
for (int i = 0; i < height; i++) {
  x->NoBarrier_SetNext(i, prev_[i]->NoBarrier_Next(i)); // relaxed
  prev_[i]->SetNext(i, x); // release
}
```

对于正在初始化的节点来说，我们使用 relaxed 语义，即 NoBarrier_SetNext() 和 NoBarrier_Next()，因为这时候节点还没有正式被加入到 SkipList，即对读线程不可见，所以可以使用较弱的 relaxed 语义，但是会在初始化完成后使用 release 语义将节点插入到 SkipList 中，即 SetNext()。根据 release 语义，之前所有 relaxed 操作在这个节点被插入到 SkipList 后对于其他线程的 acquire 操作都是可见的。

注意这里插入节点的**整个过程并不是原子**的，在**每一层**插入节点才是原子的。所以有个值得注意的点是在节点插入时我们采用从下到上的方式，因为对于 SkipList 来说，key 在 SkipList 内意味着 key 一定在 level 0，所以如果从上到下插入的话可能出现幻读，即在上层查找比较的时候存在这个 key，但是当下降到 level 0 时发现这个 key 并不存在。



\3. SkipList 的节点读操作。对于节点的所有读操作，都会使用 acquire 语义，也就是 Next() 函数，因为要保证我们读取的节点是最新的。

除了顺序插入这个优化，在这个优化里会用 relaxed 语义进行节点读取，也就是 NoBarrier_Next() 函数，因为对于写来说，会有外部同步，所以即使前后两次插入线程不同，使用 relaxed 语义也能读到最新的节点。







**PS:**

RocksDB SkipList 满足线性一致性，即 Linearizability，如果你了解了线性一致性可以去看下 SkipList 的单元测试。

RocksDB 里面还有一个 SkipList，叫做 InlineSkipList，它是支持多读多写的，节点插入的时候会使用 CAS 判断节点的 next域是否发生了改变，这个 CAS 操作使用默认的 *memory_order_seq_cst*。



**结语：**

Memory Order 是每个底层程序员都需要花时间去掌握的东西，至少会让你对于并发编程的理解会更深。这里有个对于 C++ 11 memory order 的知乎回答， 讲得很简洁明了，[知乎用户：如何理解 C++11 的六种 memory order？](https://www.zhihu.com/question/24301047/answer/85844428)。

然后 RocksDB 也提供了一个很好的学习 memory order 的地方—— SkipList，当初我看代码时直接跳过了原子操作相关的东西，因为感觉很复杂，现在看来花点时间还是能弄明白的。

另外 acquire-release 语义最近也被我放进了自己写的项目里，替代了之前的 full memory barrier，链接就不放出来了。





以下两个官方文档很适合做延伸阅读，尤其是第一个 linux kernal 文档，讲得非常详细，举了很多例子，而且还涉及了很多其他的东西，第二个文档是 C++11 memory order 的 reference。

[Linux-Kernal-Memory-Barrier](https://link.zhihu.com/?target=https%3A//github.com/torvalds/linux/blob/master/Documentation/memory-barriers.txt)

[std::memory_order - cppreference.com](https://link.zhihu.com/?target=https%3A//en.cppreference.com/w/cpp/atomic/memory_order)

