[TOC]



# Build & Tools

本章代码：Go_In_Action/chapter2

## 包

​        所有 Go 语言的程序都会组织成若干**组**文件，每**组**文件被称为一个包。这样每个包的代码都可以作为很小的复用单元，被其他项目引用。让我们看看标准库中的 http 包是怎么利用包的特性组织功能的：

```
net/http/
	cgi/
	cookiejar/
		testdata/
	fcgi/
	httptest/
	httputil/ 
	pprof/
	testdata/
```

​        这些目录包括一系列以.go 为扩展名的相关文件。这些目录将实现 HTTP 服务器、客户端、测试工具和性能调试工具的相关代码拆分成功能清晰的、小的代码单元。以 cookiejar 包为例，这个包里包含与存储和获取网页会话上的 cookie 相关的代码。每个包都可以单独导入和使用，以便开发者可以根据自己的需要导入特定功能。例如，如果要实现 HTTP 客户端，只需要导入 http 包就可以。

​        所有的.go 文件，除了空行和注释，都应该在第一行声明自己所属的包。每个包都在一个单独的目录里。不能把多个包放到同一个目录中，也不能把同一个包的文件分拆到多个不同目录中。这意味着，**<u>同一个目录下的所有.go 文件必须声明同一个包名。</u>**



### main 包名

​        <u>在 Go 语言里，命名为 main 的包具有特殊的含义。Go 语言的编译程序会试图把这种名字的包编译为二进制可执行文件。所有用 Go 语言编译的可执行程序都必须有一个名叫 main 的包。</u>

​        当编译器发现某个包的名字为 main 时，它一定也会发现名为 main() 的函数，否则不会创建可执行文件。main()函数是程序的入口，所以，如果没有这个函数，程序就没有办法开始执行。程序编译时，会使用声明 main 包的代码所在的目录的目录名作为二进制可执行文件的文件名。

> 命令和包： Go 文档里经常使用命令（command）这个词来指代可执行程序，如命令行应用程序。这会让新手在阅读文档时产生困惑。记住，在 Go 语言里，命令是指任何可执行程序。作为对比，包更常用来指语义上可导入的功能单元。



## 导入

​        我们已经了解如何把代码组织到包里，现在让我们来看看如何导入这些包，以便可以访问包内的代码。import 语句告诉编译器到磁盘的哪里去找想要导入的包。导入包需要使用关键字 import，它会告诉编译器你想引用该位置的包内的代码。如果需要导入多个包，习惯上是将 import 语句包装在一个导入块中，代码展示了一个例子：

```go
import (
	"fmt"
	"strings" //strings 包提供了很多关于字符串的操作，如查找、替换或
              //者变换。可以通过访问 http://golang.org/pkg/strings/或者在终端
              //运行 godoc strings 来了解更多关于 strings 包的细节。
)
```

​		编译器会使用 Go 环境变量设置的路径，通过引入的相对路径来查找磁盘上的包。标准库中的包会在安装 Go 的位置找到。Go 开发者创建的包会在 GOPATH 环境变量指定的目录里查找。GOPATH 指定的这些目录就是开发者的个人工作空间。

​		一旦编译器找到一个满足 import 语句的包，就停止进一步查找。有一件重要的事需要记住，编译器会首先查找 Go 的安装目录，然后才会按顺序查找 GOPATH 变量里列出的目录。如果编译器查遍 GOPATH 也没有找到要导入的包，那么在试图对程序执行 run 或者 build的时候就会出错。本章后面会介绍如何通过 go get 命令来修正这种错误。

### 远程导入

​        目前的大势所趋是，使用分布式版本控制系统（Distributed Version Control Systems，DVCS）来分享代码，如 GitHub、Launchpad 还有 Bitbucket。Go 语言的工具链本身就支持从这些网站及类似网站获取源代码。Go 工具链会使用导入路径确定需要获取的代码在网络的什么地方。

​        例如：

​        import "github.com/spf13/viper"

​        用导入路径编译程序时，go build 命令会使用 GOPATH 的设置，在磁盘上搜索这个包。事实上，这个导入路径代表一个 URL，指向 GitHub 上的代码库。<u>如果路径包含 URL，可以使用 Go 工具链从 DVCS 获取包，并把包的源代码保存在 GOPATH 指向的路径里与 URL 匹配的目录里。这个获取过程使用 go get 命令完成。go get 将获取任意指定的 URL 的包，或者一个已经导入的包所依赖的其他包。由于 go get 的这种递归特性，这个命令会扫描某个包的源码树，获取能找到的所有依赖包。</u>



### 命名导入 

​        如果要导入的多个包具有相同的名字，会发生什么？例如，既需要 network/convert 包来转换从网络读取的数据，又需要 file/convert 包来转换从文本文件读取的数据时，就会同时导入两个名叫 convert 的包。这种情况下，重名的包可以通过命名导入来导入。命名导入是指，在 import 语句给出的包路径的左侧定义一个名字，将导入的包命名为新名字。

​		例如，若用户已经使用了标准库里的 fmt 包，现在要导入自己项目里名叫 fmt 的包，就可以通过代码所示的命名导入方式，在导入时重新命名自己的包：

```go
01 package main
02
03 import (
04 		"fmt"
05 		myfmt "mylib/fmt"
06 )
07
08 func main() {
09 		fmt.Println("Standard Library")
10 		myfmt.Println("mylib/fmt")
11 }
```



## 函数 init

​		每个包可以包含任意多个 init 函数，这些函数都会在程序执行开始的时候被调用。所有被编译器发现的 init 函数都会安排在 main 函数之前执行。init 函数用在设置包、初始化变量或者其他要在程序运行前优先完成的引导工作。

​		以数据库驱动为例，database 下的驱动在启动时执行 init 函数会将自身注册到 sql 包里，因为 sql 包在编译时并不知道这些驱动的存在，等启动之后 sql 才能调用这些驱动。让我们看看这个过程中 init 函数做了什么，如下代码所示：

```go
01 package postgres
02
03 import (
04 		"database/sql"
05 )
06
07 func init() {
08 		sql.Register("postgres", new(PostgresDriver)) //创建一个 postgres 驱动的实例。这里为了展现init 的作用，没有展现其定义细节。
09 }
```

​		这段示例代码包含在 PostgreSQL 数据库的驱动里。如果程序导入了这个包，就会调用 init 函数，促使 PostgreSQL 的驱动最终注册到 Go 的 sql 包里，成为一个可用的驱动。

​        在使用这个新的数据库驱动写程序时，我们使用空白标识符来导入包，以便新的驱动会包含到 sql 包。如前所述，不能导入不使用的包，为此使用空白标识符重命名这个导入可以让 init 函数发现并被调度运行，让编译器不会因为包未被使用而产生错误。



## 使用 Go 的工具

### 编译和运行

我们会使用下面的样例代码：

```go
01 package main
02
03 import (
04 		"fmt"
05 		"io/ioutil"
06 		"os"
07
08 		"github.com/goinaction/code/chapter3/words"
09 )
10
11 // main 是应用程序的入口
12 func main() {
13 		filename := os.Args[1]
14
15 		contents, err := ioutil.ReadFile(filename)
16 		if err != nil {
17 			fmt.Println(err)
18 			return
19 		} 
20
21 		text := string(contents)
22
23 		count := words.CountWords(text)
24 		fmt.Printf("There are %d words in your text．\n", count)
25 }
```



​		大部分 Go 工具的命令都会接受一个包名作为参数。回顾一下已经用过的命令，会想起 build 命令可以简写。在不包含文件名时，go 工具会默认使用当前目录来编译。

​		`go  build` 

​		因为构建包是很常用的动作，所以也可以直接指定包：
​		`go  build  github.com/goinaction/code/chapter3/wordcount`

​		也可以在指定包的时候使用通配符。3 个点表示匹配所有的字符串。例如，下面的命令会编译 chapter3 目录下的所有包：
​		`go  build  github.com/goinaction/code/chapter3/...`

​        除了指定包，大部分 Go 命令使用短路径作为参数。例如，下面两条命令的效果相同：

​		`go build wordcount.go`

​		`go build .`

要执行程序，需要首先编译，然后执行编译创建的 wordcount 或者 wordcount.exe 程序。不过这里有一个命令可以在一次调用中完成这两个操作：

​        `go run wordcount.go`

go run 命令会先构建 wordcount.go 里包含的程序，然后执行构建后的程序。这样可以节省好多录入工作量。



### go vet

​        vet 命令会帮开发人员检测代码的常见错误：

- Printf 类函数调用时，类型匹配错误的参数。
- 定义常用的方法时，方法签名的错误。
- 错误的结构标签。
- 没有指定字段名的结构字面量。

`go vet main.go`

`main.go:6: no formatting directive in Printf call`

​        go vet 工具不能让开发者避免严重的逻辑错误，或者避免编写充满小错的代码。不过，正像刚才的实例中展示的那样，这个工具可以很好地捕获一部分常见错误。每次对代码先执行 go vet 再将其签入源代码库是一个很好的习惯。



### Go 代码格式化

​		fmt 工具会将开发人员的代码布局成和 Go 源代码类似的风格，不用再为了大括号是不是要放到行尾，或者用 tab（制表符）还是空格来做缩进而争论不休。使用 go fmt 后面跟文件名或者包名，就可以调用这个代码格式化工具。fmt 命令会自动格式化开发人员指定的源代码文件并保存。下面是一个代码执行 go fmt 前和执行 go fmt 后几行代码的对比：

`if err != nil { return err }`

在对这段代码执行 go fmt 后，会得到：

```go
if err != nil {
		return err
}
```

很多 Go 开发人员会配置他们的开发环境，在保存文件或者提交到代码库前执行 go fmt。































