package main

import "fmt"

func main() {
    var haha string = "你好"
    fmt.Println("Hello, World!" + haha + "hehe")

    var socketprot = 1
    var ip = "123.456"
    var port_ip = "%s:%d"
    var port_ip_str = fmt.Sprintf(port_ip, ip, socketprot)
    fmt.Println("myip:",port_ip_str)

    ss := "hello"
    fmt.Println(ss)

    
    //const iota  从0开始，每次加1   类似于枚举
    const (
        a = 3 << iota
        b
        c
        d = "ha"
        e
        f = 100
        g
        h = iota
        i
    )
    fmt.Println(a,b,c,d,e,f,g,h,i)

    //channel
    ch := make(chan string)
    ch_int := make(chan int, 1)
    //ch <- "hello channe1"
    go func() {
        // 此处没有通道操作，只是打印一条消息
        ch <- "hello channel2"
        ch_int <- 1
        fmt.Println("This goroutine is running.")
    }()

    select {
        case <-ch:
            fmt.Println("This case is running.1")
        case <-ch_int:
            fmt.Println("This case is running.2")
    }
    
    msg := <-ch
    //msg_int := <-ch_int
    fmt.Println(msg)
    //fmt.Println(msg_int)
}