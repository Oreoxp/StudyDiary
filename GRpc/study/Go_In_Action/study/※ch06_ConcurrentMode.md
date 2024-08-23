[TOC]

# 并发模式

​        学习 3 个可以在实际工程里使用的包，这 3 个包分别实现了不同的并发模式。每个包从一个实用的视角来讲解如何使用并发和通道。我们会学习如何用这个包简化并发程序的编写，以及为什么能简化的原因。



## runner

​        runner 包用于展示如何使用通道来监视程序的执行时间，如果程序运行时间太长，也可以用 runner 包来终止程序。当开发需要调度后台处理任务的程序的时候，这种模式会很有用。这个程序可能会作为 cron 作业执行，或者在基于定时任务的云环境（如 iron.io）里执行。

​        让我们来看一下 runner 包里的 runner.go 代码文件，如下代码所示：

```go
01 // Gabriel Aszalos 协助完成了这个示例
02 // runner 包管理处理任务的运行和生命周期
 03 package runner
04
05 import ( 
06 		"errors"
07 		"os"
08 		"os/signal"
09 		"time"
10 ) 
11
12 // Runner 在给定的超时时间内执行一组任务，
13 // 并且在操作系统发送中断信号时结束这些任务
14 type Runner struct { 
15 		// interrupt 通道报告从操作系统
16 		// 发送的信号
17 		interrupt chan os.Signal
18
19 		// complete 通道报告处理任务已经完成
20 		complete chan error
21
22 		// timeout 报告处理任务已经超时
23 		timeout <-chan time.Time
24
25 		// tasks 持有一组以索引顺序依次执行的
26 		// 函数
27 		tasks []func(int)
28 } 
29
30 // ErrTimeout 会在任务执行超时时返回
31 var ErrTimeout = errors.New("received timeout")
32
33 // ErrInterrupt 会在接收到操作系统的事件时返回
34 var ErrInterrupt = errors.New("received interrupt")
35
36 // New 返回一个新的准备使用的 Runner
37 func New(d time.Duration) *Runner { 
38 		return &Runner{
39 					interrupt: make(chan os.Signal, 1),
40 					complete: make(chan error),
41 					timeout: time.After(d),
42 				} 
43 } 
44
45 // Add 将一个任务附加到 Runner 上。这个任务是一个
46 // 接收一个 int 类型的 ID 作为参数的函数
47 func (r *Runner) Add(tasks ...func(int)) { 
48 		r.tasks = append(r.tasks, tasks...)
49 } 
50
51 // Start 执行所有任务，并监视通道事件
52 func (r *Runner) Start() error { 
53 		// 我们希望接收所有中断信号
54 		signal.Notify(r.interrupt, os.Interrupt)
55
56 		// 用不同的 goroutine 执行不同的任务
57 		go func() { 
58 			r.complete <- r.run()
59 		}()
60
61 		select { 
62 			// 当任务处理完成时发出的信号
63 			case err := <-r.complete:
64 				return err
65
66 			// 当任务处理程序运行超时时发出的信号
67 			case <-r.timeout:
68 				return ErrTimeout
69 			} 
70 } 
71
72 // run 执行每一个已注册的任务
73 func (r *Runner) run() error { 
74 		for id, task := range r.tasks { 
75 			// 检测操作系统的中断信号
76 			if r.gotInterrupt() { 
77 				return ErrInterrupt
78 			} 
79
80 			// 执行已注册的任务
81 			task(id)
82 		} 
83
84 		return nil
85 } 
86
87 // gotInterrupt 验证是否接收到了中断信号
88 func (r *Runner) gotInterrupt() bool { 
89 		select { 
90 			// 当中断事件被触发时发出的信号
91 			case <-r.interrupt:
92 				// 停止接收后续的任何信号
93 				signal.Stop(r.interrupt)
95 				return true
96
97 			// 继续正常运行
98 			default:
99 				return false
100 	} 
101 }
```

​        上述代码中的程序展示了依据调度运行的无人值守的面向任务的程序，及其所使用的并发模式。在设计上，可支持以下终止点：

- 程序可以在分配的时间内完成工作，正常终止；
- 程序没有及时完成工作，“自杀”；
- 接收到操作系统发送的中断事件，程序立刻试图清理状态并停止工作。



让我们走查一遍代码，看看每个终止点是如何实现的，如下代码所示。

```go
12 // Runner 在给定的超时时间内执行一组任务，
13 // 并且在操作系统发送中断信号时结束这些任务
14 type Runner struct { 
15 		// interrupt 通道报告从操作系统
16 		// 发送的信号
17 		interrupt chan os.Signal
18
19 		// complete 通道报告处理任务已经完成
20 		complete chan error
21
22 		// timeout 报告处理任务已经超时
23 		timeout <-chan time.Time
24
25 		// tasks 持有一组以索引顺序依次执行的
26 		// 函数
27 		tasks []func(int)
28 }
```

​        代码从第 14 行声明 Runner 结构开始。这个类型声明了 3 个通道，用来辅助管理程序的生命周期，以及用来表示顺序执行的不同任务的函数切片。

​        第 17 行的 interrupt 通道收发 os.Signal 接口类型的值，用来从主机操作系统接收中断事件。os.Signal 接口的声明如下所示：

```go
// Signal 用来描述操作系统发送的信号。其底层实现通常会
// 依赖操作系统的具体实现：在 UNIX 系统上是
// syscall.Signal 
type Signal interface { 
		String() string
		Signal()//用来区分其他 Stringer
}
```



​        第二个通道被命名为 complete，因为它被执行任务的 goroutine 用来发送任务已经完成的信号。如果执行任务时发生了错误，会通过这个通道发回一个 error 接口类型的值。如果没有发生错误，会通过这个通道发回一个 nil 值作为 error 接口值。

​		第三个字段被命名为 timeout，接收 time.Time 值。

​		这个通道用来管理执行任务的时间。如果从这个通道接收到一个 time.Time 的值，这个程序就会试图清理状态并停止工作。

​		最后一个字段被命名为 tasks，是一个函数值的切片。

​		现在我们来看一下用户如何创建一个 Runner 类型的值，如下代码所示：

```go
36 // New 返回一个新的准备使用的 Runner
37 func New(d time.Duration) *Runner { 
38 		return &Runner{
39 				interrupt: make(chan os.Signal, 1),
40 				complete: make(chan error),
41 				timeout: time.After(d),
42 			} 
43 }
```

​		展示了名为 New 的工厂函数。这个函数接收一个 time.Duration 类型的值，并返回 Runner 类型的指针。这个函数会创建一个 Runner 类型的值，并初始化每个通道字段。因为 task 字段的零值是 nil，已经满足初始化的要求，所以没有被明确初始化。





现在看过了 runner 包的代码，并了解了代码是如何工作的，让我们看一下 main.go 代码文件中的测试程序：

```go
01 // 这个示例程序演示如何使用通道来监视
02 // 程序运行的时间，以在程序运行时间过长
03 // 时如何终止程序
03 package main
04
05 import ( 
06 		"log"
07 		"time"
08
09 		"github.com/goinaction/code/chapter7/patterns/runner"
10 ) 
11
12 // timeout 规定了必须在多少秒内处理完成
13 const timeout = 3 * time.Second
14
15 // main 是程序的入口
16 func main() { 
17 		log.Println("Starting work.")
18
19 		// 为本次执行分配超时时间
20 		r := runner.New(timeout)
21
22 		// 加入要执行的任务
23 		r.Add(createTask(), createTask(), createTask())
24
25 		// 执行任务并处理结果
26 		if err := r.Start(); err != nil { 
27 			switch err { 
28 				case runner.ErrTimeout:
29 					log.Println("Terminating due to timeout.")
30 					os.Exit(1)
31 				case runner.ErrInterrupt:
32 					log.Println("Terminating due to interrupt.")
33 					os.Exit(2)
34 			} 
35 		}	 
36
37 		log.Println("Process ended.")
38 } 
39
40 // createTask 返回一个根据 id 
41 // 休眠指定秒数的示例任务
42 func createTask() func(int) { 
43 		return func(id int) { 
44 					log.Printf("Processor - Task #%d.", id)
45 					time.Sleep(time.Duration(id) * time.Second)
46 				} 
47 }
```

​		代码的第 16 行是 main 函数。在第 20 行，使用 timeout 作为超时时间传给 New 函数，并返回了一个指向 Runner 类型的指针。之后在第 23 行，使用 createTask 函数创建了几个任务，并被加入 Runner 里。在第 42 行声明了 createTask 函数。这个函数创建的任务只是休眠了一段时间，用来模拟正在进行工作。增加完任务后，在第 26 行调用了 Start 方法，main 函数会等待 Start 方法的返回。

​		当 Start 返回时，会检查其返回的 error 接口值，并存入 err 变量。如果确实发生了错误，代码会根据 err 变量的值来判断方法是由于超时终止的，还是由于收到了中断信号终止。如果没有错误，任务就是按时执行完成的。如果执行超时，程序就会用错误码 1 终止。如果接收到中断信号，程序就会用错误码 2 终止。其他情况下，程序会使用错误码 0 正常终止。



## pool

​        本章会介绍 pool 包 。这个包用于展示如何**使用有缓冲的通道实现资源池，来管理可以在任意数量的 goroutine 之间共享及独立使用的资源。**这种模式在需要共享一组静态资源的情况（如共享数据库连接或者内存缓冲区）下非常有用。如果 goroutine 需要从池里得到这些资源中的一个，它可以从池里申请，使用完后归还到资源池里。

​		让我们看一下 pool 包里的 pool.go 代码文件，如下代码所示。

```go
01 // Fatih Arslan 和 Gabriel Aszalos 协助完成了这个示例
02 // 包 pool 管理用户定义的一组资源
03 package pool
04
05 import ( 
06 		"errors"
07 		"log"
08 		"io"
09 		"sync"
10 ) 
11
12 // Pool 管理一组可以安全地在多个 goroutine 间
13 // 共享的资源。被管理的资源必须
14 // 实现 io.Closer 接口
15 type Pool struct { 
16 		m sync.Mutex
17 		resources chan io.Closer
18 		factory func() (io.Closer, error)
19 		closed bool
20 } 
21
22 // ErrPoolClosed 表示请求（Acquire）了一个
23 // 已经关闭的池
24 var ErrPoolClosed = errors.New("Pool has been closed.")
25
26 // New 创建一个用来管理资源的池。
27 // 这个池需要一个可以分配新资源的函数， 
28 // 并规定池的大小
29 func New(fn func() (io.Closer, error), size uint) (*Pool, error) { 
30 		if size <= 0 { 
31 			return nil, errors.New("Size value too small.")
32 		} 
33
34 		return &Pool{
35 			factory: fn,
36 			resources: make(chan io.Closer, size),
37 			}, nil
38 	} 
39
40 // Acquire 从池中获取一个资源
41 func (p *Pool) Acquire() (io.Closer, error) { 
42 		select { 
43 			// 检查是否有空闲的资源
44 			case r, ok := <-p.resources:
45 				log.Println("Acquire:", "Shared Resource")
46 				if !ok { 
47 					return nil, ErrPoolClosed
48 				} 
49 				return r, nil
50
51 			// 因为没有空闲资源可用，所以提供一个新资源
52 			default:
53 				log.Println("Acquire:", "New Resource")
54 				return p.factory()
55 		} 
56 	} 
57
58 // Release 将一个使用后的资源放回池里
59 func (p *Pool) Release(r io.Closer) { 
60 		// 保证本操作和 Close 操作的安全
61 		p.m.Lock()
62 		defer p.m.Unlock()
63
64 		// 如果池已经被关闭，销毁这个资源
65 		if p.closed { 
66 			r.Close()
67 			return
68 		} 
69
70 		select { 
71 			// 试图将这个资源放入队列
72 			case p.resources <- r:
73 				log.Println("Release:", "In Queue")
74
75 				// 如果队列已满，则关闭这个资源
76 			default:
77 				log.Println("Release:", "Closing")
78 				r.Close()
79 		} 
80 	} 
81
82 // Close 会让资源池停止工作，并关闭所有现有的资源
83 func (p *Pool) Close() { 
84 		// 保证本操作与 Release 操作的安全
85 		p.m.Lock()
86 		defer p.m.Unlock()
87
88 		// 如果 pool 已经被关闭，什么也不做
89 		if p.closed { 
90 			return
91 		} 
92
93 		// 将池关闭
94 		p.closed = true
95
96 		// 在清空通道里的资源之前，将通道关闭
97 		// 如果不这样做，会发生死锁
98 		close(p.resources)
99
100 		// 关闭资源
101 	for r := range p.resources { 
102 		r.Close()
103 	} 
104 }
```

​		pool 包的代码声明了一个名为 Pool 的结构，该结构允许调用者根据所需数量创建不同的资源池。只要某类资源实现了 io.Closer 接口，就可以用这个资源池来管理。让我们看一下 Pool 结构的声明，如下代码所示：

```go
12 // Pool 管理一组可以安全地在多个 goroutine 间
13 // 共享的资源。被管理的资源必须
14 // 实现 io.Closer 接口
15 type Pool struct { 
16 		m sync.Mutex
17 		resources chan io.Closer
18 		factory func() (io.Closer, error)
19 		closed bool
20 }
```

​		Pool 结构声明了 4 个字段，每个字段都用来辅助以 goroutine 安全的方式来管理资源池。在第 16 行，结构以一个 sync.Mutex 类型的字段开始。这个互斥锁用来保证在多个 goroutine 访问资源池时，池内的值是安全的。第二个字段名为 resources，被声明为 io.Closer 接口类型的通道。这个通道是作为一个有缓冲的通道创建的，用来保存共享的资源。由于通道的类型是一个接口，所以池可以管理任意实现了 io.Closer 接口的资源类型。

​		factory 字段是一个函数类型。任何一个没有输入参数且返回一个 io.Closer 和一个 error 接口值的函数，都可以赋值给这个字段。这个函数的目的是，当池需要一个新资源时，可以用这个函数创建。这个函数的实现细节超出了 pool 包的范围，并且需要由包的使用者实现并提供。

​		第 19 行中的最后一个字段是 closed 字段。这个字段是一个标志，表示 Pool 是否已经被关闭。现在已经了解了 Pool 结构的声明，让我们看一下第 24 行声明的 error 接口变量，如下代码所示：

```go
22 // ErrPoolClosed 表示请求（Acquire）了一个
23 // 已经关闭的池
24 var ErrPoolClosed = errors.New("Pool has been closed.")
```

​		Go 语言里会经常创建 error 接口变量。这可以让调用者来判断某个包里的函数或者方法返回的具体的错误值。当调用者对一个已经关闭的池调用 Acquire 方法时，会返回代码里的 error 接口变量。因为 Acquire 方法可能返回多个不同类型的错误，所以 Pool 已经关闭时会关闭时返回这个错误变量可以让调用者从其他错误中识别出这个特定的错误。



接下来让我们来看一下 Acquire 方法，如下代码所示。这个方法可以让调用者从池里获得资源。

```go
40 // Acquire 从池中获取一个资源
41 func (p *Pool) Acquire() (io.Closer, error) { 
42 		select { 
43 			// 检查是否有空闲的资源
44 			case r, ok := <-p.resources:
45 				log.Println("Acquire:", "Shared Resource")
46 				if !ok { 
47 					return nil, ErrPoolClosed
48 				} 
49 				return r, nil
50
51 				// 因为没有空闲资源可用，所以提供一个新资源
52 			default:
53 				log.Println("Acquire:", "New Resource")
54 				return p.factory()
55 		} 
56 }
```

​		这个方法在还有可用资源时会从资源池里返回一个资源，否则会为该调用创建并返回一个新的资源。这个实现是通过 select/case 语句来检查有缓冲的通道里是否还有资源来完成的。如果通道里还有资源，如第 44 行到第 49 行所写，就取出这个资源，并返回给调用者。如果该通道里没有资源可取，就会执行 default 分支。在这个示例中，在第 54 行执行用户提供的工厂函数，并且创建并返回一个新资源。







## work

​		**work 包的目的是展示如何使用无缓冲的通道来创建一个 goroutine 池，这些 goroutine 执行并控制一组工作，让其并发执行。在这种情况下，使用无缓冲的通道要比随意指定一个缓冲区大小的有缓冲的通道好，因为这个情况下既不需要一个工作队列，也不需要一组 goroutine 配合执行。无缓冲的通道保证两个 goroutine 之间的数据交换。这种使用无缓冲的通道的方法允许使用者知道什么时候 goroutine 池正在执行工作，而且如果池里的所有 goroutine 都忙，无法接受新的工作的时候，也能及时通过通道来通知调用者。使用无缓冲的通道不会有工作在队列里丢失或者卡住，所有工作都会被处理。**

```go
01 // Jason Waldrip 协助完成了这个示例
02 // work 包管理一个 goroutine 池来完成工作
03 package work
04
05 import "sync"
06
07 // Worker 必须满足接口类型，
08 // 才能使用工作池
09 type Worker interface { 
10 		Task()
11 } 
12
13 // Pool 提供一个 goroutine 池，这个池可以完成
14 // 任何已提交的 Worker 任务
15 type Pool struct { 
16 		work chan Worker
17 		wg sync.WaitGroup
18 } 
19
20 // New 创建一个新工作池
21 func New(maxGoroutines int) *Pool { 
22 		p := Pool{
23 			work: make(chan Worker),
24 		} 
25
26 		p.wg.Add(maxGoroutines)
27 		for i := 0; i < maxGoroutines; i++ { 
28 			go func() { 
29 				for w := range p.work { 
30 					w.Task()
31 				} 
32 				p.wg.Done()
33 			}()
34 		} 
35
36 		return &p
37 } 
38
39 // Run 提交工作到工作池
40 func (p *Pool) Run(w Worker) { 
41 		p.work <- w
42 } 
43
44 // Shutdown 等待所有 goroutine 停止工作
45 func (p *Pool) Shutdown() { 
46 		close(p.work) 
47		p.wg.Wait()
48 }
```

​		New 函数使用固定数量的 goroutine 来创建一个工作池。goroutine 的数量作为参数传给 New 函数。在第 22 行，创建了一个 Pool 类型的值，并使用无缓冲的通道来初始化 work 字段。

​		之后，在第 26 行，初始化 WaitGroup 需要等待的数量，并在第 27 行到第 34 行，创建了同样数量的 goroutine。这些 goroutine 只接收 Worker 类型的接口值，并调用这个值的 Task 方法。

​		for range 循环会一直阻塞，直到从 work 通道收到一个 Worker 接口值。如果收到一个值，就会执行这个值的 Task 方法。一旦 work 通道被关闭，for range 循环就会结束，并调用 WaitGroup 的 Done 方法。然后 goroutine 终止。













