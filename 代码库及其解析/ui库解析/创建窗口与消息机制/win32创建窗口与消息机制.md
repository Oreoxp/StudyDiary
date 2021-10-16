windows创建一个普通的窗口代码：

```c++
#include "stdafx.h" //注意，这个向导产生的头文件不能去掉 
#include <windows.h>
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM) ;

int WINAPI WinMain (HINSTANCE hInstance, 
                    HINSTANCE hPrevInstance,
                    PSTR szCmdLine, 
                    int iCmdShow) {
  static TCHAR szAppName[] = TEXT ("HelloWin"); 
  
	HWND   hwnd ;
 	MSG    msg ;
  
	WNDCLASS wc ;
	wc.style         = CS_HREDRAW | CS_VREDRAW ;
  wc.lpfnWndProc   = WndProc ;
  wc.cbClsExtra    = 0 ;
  wc.cbWndExtra    = 0 ;
  wc.hInstance     = hInstance ;
  wc.hIcon         = LoadIcon (NULL, IDI_APPLICATION) ;
  wc.hCursor       = LoadCursor (NULL, IDC_ARROW) ;
  wc.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH) ;
  wc.lpszMenuName  = NULL ;
  wc.lpszClassName = szAppName ;
  if(!RegisterClass (&wc)) {
          MessageBox (NULL, TEXT (
            					"This program requires Windows NT!"), 
                      szAppName, MB_ICONERROR) ;
       		return 0 ;
  }
  hwnd = CreateWindow (szAppName,    // window class name
                       TEXT ("欢迎你的到来!"), // window caption
                       WS_OVERLAPPEDWINDOW,  // window style
                       CW_USEDEFAULT,    // initial x position
                       CW_USEDEFAULT,    // initial y position
                       CW_USEDEFAULT,    // initial x size
                       CW_USEDEFAULT,    // initial y size
                       NULL,           // parent window handle
                       NULL,             // window menu handle
                       hInstance,    //program instance handle
                       NULL) ;         // creation parameters
  ShowWindow (hwnd, iCmdShow);
  UpdateWindow (hwnd);
  
  while (GetMessage (&msg, NULL, 0, 0)) {
      TranslateMessage (&msg) ;
         DispatchMessage (&msg) ;
 	}
  return msg.wParam ;

}
LRESULT CALLBACK WndProc (HWND hwnd, 
                          UINT message, 
                          WPARAM wParam, 
                          LPARAM lParam) {
    HDC         hdc ;
    PAINTSTRUCT ps ;
    RECT        rect ;
    switch (message)
    {
 		case WM_PAINT:
        hdc = BeginPaint (hwnd, &ps) ;
         GetClientRect (hwnd, &rect) ;
        DrawText (hdc, TEXT ("你好,欢迎你来到VC之路!"), 
                  -1, 
                  &rect,DT_SINGLELINE|DT_CENTER|DT_VCENTER) ;
        EndPaint (hwnd, &ps) ;
        return 0 ;
    case WM_DESTROY:
          PostQuitMessage (0) ;
         return 0 ;
     }
   return DefWindowProc(hwnd, message, wParam, lParam) ;
}
```

我们可以把这个程序支解为四块:

[TOC]

## 建立注册窗口类

#### 建立窗口类

​		WinMain() 是程序的入口,它相当于一个中介人的角色,把应用程序(指小窗口)介绍给windows.首要的一步是登记应用程序的窗口类.

​		窗口种类是定义窗口属性的模板,这些属性包括窗口式样,鼠标形状,菜单等等,窗口种类也指定处理该类中所有窗口消息的窗口函数.只有先建立窗口种类,才能根据窗口种类来创建Windows应用程序的一个或多个窗口.创建窗口时,还可以指定窗口独有的附加特性.窗口种类简称窗口类,窗口类不能重名.在建立窗口类后,必须向Windows登记.

​		建立窗口类就是用WNDCLASS结构定义一个结构变量,在这个程序中就是指WNDCLASS wc ;然后用自己设计的窗口属性的信息填充结构变量wc的域。

#### 注册窗口类

​		在创建窗口之前，是必须要注册窗口类的，注册窗口类用的API函数是RegisterClass,注册失败的话，就会出现一个对话框如程序所示，函数RegisterClass返回0值,也只能返回0值，因为注册不成功，程序已经不能再进行下去了。

​		在参数 wc 中除了传递一些窗口参数，还会把 WndProc() 作为函数指针传递过去，在处理消息时进行回调。

在本程序中注册窗口类如下:

```c++
if (!RegisterClass (&wc)) {
　　　　　　　MessageBox (NULL, 
                   TEXT ("This program requires Windows NT!"),
　　　　　　　				szAppName,
                   MB_ICONERROR) ;
　　　　　　　return 0 ;
}
```

## 创建窗口

​		注册窗口类后，就可以创建窗口了，本程序中创建窗口的有关语句如下:

```c++
hwnd = CreateWindow (szAppName,           // window class name
                     TEXT ("欢迎你的到来!"),// window caption
                      WS_OVERLAPPEDWINDOW,// window style
                      CW_USEDEFAULT,    // initial x position
                      CW_USEDEFAULT,     // initial y position
                      CW_USEDEFAULT,     // initial x size
                      CW_USEDEFAULT,     // initial y size
                      NULL,            // parent window handle
                      NULL,              // window menu handle
                      hInstance,    // program instance handle
                      NULL) ;           // creation parameters
```

参数1 szAppName: 登记的窗口类名，这个类名刚才咱们在注册窗口时已经定义过了。

参数2：用来表明窗口的标题。

参数3 WS_OVERLAPPEDWINDOW: 用来表明窗口的风格，如有无最大化，最小化按纽啊什么的。

参数4,5 CW_USEDEFAULT: 用来表明程序运行后窗口在屏幕中的坐标值。

参数6,7 CW_USEDEFAULT: 用来表明窗口初始化时(即程序初运行时)窗口的大小，即长度与宽度。

参数8 NULL: 在创建窗口时可以指定其父窗口，这里没有父窗口则参数值为0。

参数9 NULL: 用以指明窗口的菜单，菜单以后会讲，这里暂时为0。

最后一个参数是附加数据，一般都是0。

​		CreateWindow() 的返回值是已经创建的窗口的句柄，应用程序使用这个句柄来引用该窗口。如果返回值为 0，就应该终止该程序，因为可能某个地方出错了。如果一个程序创建了多个窗口，则每个窗口都有各自不同的句柄。

## 显示和更新窗口

​		API函数 <u>CreateWindow</u> 创建完窗口后，要想把它显示出现，还必须调用另一个 API 函数 ShowWindows。形式为:

`ShowWindow(hwnd, iCmdShow);` 

​		其第一个参数是窗口句柄，告诉 <u>ShowWindow()</u> 显示哪一个窗口，而第二个参数则告诉它如何显示这个窗口：最小化 (SW_MINIMIZE) ，普通(SW_SHOWNORMAL)，还是最大化 (SW_SHOWMAXIMIZED) 。WinMain 在创建完窗口后就调用 <u>ShowWindow</u> 函数，并把 iCmdShow 参数传送给这个窗口。你可把 iCmdShow 改变为这些参数试试。

​		WinMain() 调用完 ShowWindow 后，还需要调用函数 <u>UpdateWindow</u>，最终把窗口显示了出来。调用函数 UpdateWindow 将产生一个 WM_PAINT 消息，这个消息将使窗口重画，即使窗口得到更新.

​		主窗口显示出来了，WinMain就开始处理消息了，怎么做的呢？

​		Windows为每个正在运行的应用程序都保持一个消息队列。当你按下鼠标或者键盘时，Windows并不是把这个输入事件直接送给应用程序，而是将输入的事件先翻译成一个消息，然后把这个消息放入到这个应用程序的消息队列中去。应用程序又是怎么来接收这个消息呢？这就讲讲消息循环了。

​		应用程序的WinMain函数通过执行一段代码从她的队列中来检索Windows送往她的消息。然后WinMain就把这些消息分配给相应的窗口函数以便处理它们，这段代码是一段循环代码，故称为"消息循环"。这段循环代码是什么呢？好，往下看:

在咱们的第二只小板凳中,这段代码就是:

```c++
.....

MSG msg; //定义消息名
while (GetMessage (&msg, NULL, 0, 0)) {
    TranslateMessage (&msg) ; //翻译消息
    DispatchMessage (&msg) ; //撤去消息
}
   return msg.wParam ;
```

​		消息循环以 GetMessage 调用开始，它从消息队列中取出一个消息：

​			GetMessage(&msg,NULL,0,0)，
​				第一个参数是要接收消息的 MSG 结构的地址，
​				第二个参数表示窗口句柄, NULL 则表示要获取该应用程序创建的所有窗口的消息；
​				第三，四参数指定消息范围。后面三个参数被设置为默认值，这就是说你打算接收发送到属于这个应用程序的任何一个窗口的所有消息。

​		在接收到除 WM_QUIT 之外的任何一个消息后，GetMessage() 都返回 TRUE。如果 GetMessage 收到一个 WM_QUIT 消息，则返回 FALSE，如收到其他消息，则返回 TRUE。因此，在接收到 WM_QUIT 之前，带有 GetMessage() 的消息循环可以一直循环下去。只有当收到的消息是 WM_QUIT 时，GetMessage 才返回FALSE，结束消息循环，从而终止应用程序。 均为 NULL 时就表示获取所有消息。

​		消息用 GetMessage 读入后(注意这个消息可不是 WM_QUIT 消息)，它首先要经过函数 TranslateMessage() 进行翻译，这个函数会转换成一些键盘消息，它检索匹配的 WM_KEYDOWN 和 WM_KEYUP 消息，并为窗口产生相应的 ASCII 字符消息 (WM_CHAR) , 它包含指定键的 ANSI 字符.但对大多数消息来说它并不起什么作用，所以现在没有必要考虑它。

​		下一个函数调用 DispatchMessage() 要求 Windows 将消息传送给在 MSG 结构中为窗口所指定的窗口过程。我们在讲到登记窗口类时曾提到过，登记窗口类时，我们曾指定 Windows 把函数 WindosProc 作为咱们这个窗口的窗口过程(就是指处理这个消息的东东)。就是说，Windows 会调用函数 WindowsProc() 来处理这个消息。在 WindowProc() 处理完消息后，代码又循环到开始去接收另一个消息，这样就完成了一个消息循环。

## 终止应用程序

​		Windows 是一种非剥夺式多任务操作系统。只有的应用程序交出 CPU 控制权后，Windows 才能把控制权交给其他应用程序。当 GetMessage 函数找不到等待应用程序处理的消息时，自动交出控制权，Windows 把 CPU 的控制权交给其他等待控制权的应用程序。由于每个应用程序都有一个消息循环，这种隐式交出控制权的方式保证合并各个应用程序共享控制权。一旦发往该应用程序的消息到达应用程序队列，即开始执行 GetMessage 语句的下一条语句。

​		当 WinMain 函数把控制返回到 Windows 时，应用程序就终止了。应用程序的启动消息循环前要检查引导出消息循环的每一步，以确保每个窗口已注册，每个窗口都已创建。如存在一个错误，应用程序应返回控制权，并显示一条消息。

​		但是，一旦 WinMain 函数进入消息循环，终止应用程序的唯一办法就是使用PostQuitMessage 把消息 WM_QUIT 发送到应用程序队列。当 GetMessage 函数检索到 WM_QUIT 消息，它就返回 NULL，并退出消息外循环。通常，当主窗口正在删除时(即窗口已接收到一条 WM_DESTROY 消息), 应用程序主窗口的窗口函数就发送一条 WM_QUIT 消息。

​		虽然 WinMain 指定了返回值的数据类型，但 Windows 并不使用返回值。不过，在调试一应用程序时，返回值地有用的。通常，可使用与标准 C 程序相同的返回值约定：0 表示成功，非 0 表示出错。PostQuitMessage 函数允许窗口函数指定返回值，这个值复制到 WM_QUIT 消息的 wParam 参数中。为了的结束消息循环之后返回这个值，我们的第二只小板凳中使用了以下语句：

return msg.wParam ;//表示从 PostQuitMessage 返回的值

例如：当 Windows 自身终止时，它会撤消每个窗口，但不把控制返回给应用程序的消息循环，这意味着消息循环将永远不会检索到 WM_QUIT 消息，并且的循环之后的语句也不能再执行。Windows 的终止前的确发送一消息给每个应用程序，因而标准 C 程序通常会的结束前清理现场并释放资源，但 Windows 应用程序必须随每个窗口的撤消而被清除，否则会丢失一些数据。

## 窗口过程

​		如前所述，**<u>函数 GetMessage 负责从应用程序的消息队列中取出消息</u>**，而**<u>函数DispatchMessage() 要求 Windows 将消息传送给在 MSG 结构中为窗口所指定的窗口过程</u>**。然后出台的就是这个窗口过程了，这个窗口过程的任务是干什么呢？就是最终用来处理消息的，就是消息的处理器而已，那么这个函数就是 WindowProc,在 Visual C++6.0 中按 F1 启动 MSDN，按下面这个路径走下来：

```c++
PlatForm SDK-->User Interface services-->
Windows user Interface-->Windowing-->
Window Procedures-->Window Procedure Reference-->
Windows Procedure Functions-->WindowProc

LRESULT CALLBACK WindowProc
(
	HWND hwnd, // handle to window
	UINT uMsg, // message identifier
	WPARAM wParam, // first message parameter
	LPARAM lParam // second message parameter
);

WndProc
```



下面讲解：

​		不知你注意到了没有，这个函数的参数与刚刚提到的GetMessage调用把返回的MSG结构的前四个成员相同。如果消息处理成功，WindowProc的返回值为0.

​		Windows 的启动应用程序时，先调用 WinMain 函数，然后调用窗口过程，注意：在我们的这个程序中，只有一个窗口过程，实际上，也许有不止一个的窗口过程。例如，每一个不同的窗口类都有一个与之相对应的窗口过程。无论 Windows何时想传递一个消息到一窗口，都将调用相应的窗口过程。当 Windows 从环境，或从另一个应用程序，或从用户的应用程序中得到消息时，它将调用窗口过程并将信息传给此函数。总之，窗口过程函数处理所有传送到由此窗口类创建的窗口所得到的消息。并且窗口过程有义务处理 Windows 扔给它的任何消息。我们在学习Windows 程序设计的时候，最主要的就是学习这些消息是什么以及是什么意思，它们是怎么工作的。

​		令我们不解的是，在程序中我们看不出来是哪一个函数在调用窗口过程。它其实是一个回调函数.前面已经提到，Windows 把发生的输入事件转换成输入消息放到消息队列中，而消息循环将它们发送到相应的窗口过程函数，真正的处理是在窗口过程函数中执行的，在 Windows 中就使用了回调函数来进行这种通信。

​		回调函数是输出函数中特殊的一种，它是指那些在 Windows 环境下直接调用的函数。一个应用程序至少有一个回调函数，因为在应用程序处理消息时，Windows 调用回调函数。这种回调函数就是我们前面提到的窗口过程，它对对应于一个活动的窗口，回调函数必须向 Windows 注册，Windows 实施相应操作即行回调。

​		每个窗口必须有一个窗口过程与之对应，且 Windows 直接调用本函数，因此，窗口函数必须采用 FAR PASCAL 调用约定。在我们的第二只小板凳中，我们的窗口函数为 WndProc，必须注意这里的函数名必须是前面注册的窗口类时，向域wc.lpfnWndProc 所赋的 WndProc 。函数 WndProc 就是前面定义的窗口类所生成的所有窗口的窗口函数。

​		在我们的这个窗口函数中，WndProc 处理了共有两条消息：WM_PAINT 和 WM_DESTROY.

​		窗口函数从 Windows 中接收消息，这些消息或者是由 WinMain 函数发送的输入消息，或者是直接来自 Windows 的窗口管理消息。窗口过程检查一条消息，然后根据这些消息执行特定的动作未被处理的消息通过 DefWindowProc 函数传回给Windows 作缺省处理。

​		可以发送窗口函数的消息约有 220 种，所有窗口消息都以 WM_ 开头，这些消息在头文件中被定义为常量。引起 Windows 调用窗口函数的原因有很多，，如改变窗口大小啊，改变窗口在屏幕上的位置啊什么的。



## 处理消息

​		窗口过程处理消息通常以 *switch* 语句开始，对于它要处理的每一条消息 *ID*都跟有一条 *case* 语句。大多数 *windows proc* 都有具有下面形式的内部结构：

```c++
switch(uMsgId) {
case WM_(something):
		//这里此消息的处理过程
		return 0;
case WM_(something else):
		//这里是此消息的处理过程
		return 0;
default:
		//其他消息由这个默认处理函数来处理
		return DefWindowProc(hwnd,uMsgId,wParam,lParam);
}
```

​		在处理完消息后，要返回 0，这很重要-----它会告诉 Windows 不必再重试了。对于那些在程序中不准备处理的消息，窗口过程会把它们都扔给 DefWindowProc 进行缺省处理，而且还要返回那个函数的返回值。在消息传递层次中，可以认为 DefWindowProc 函数是最顶层的函数。这个函数发出WM_SYSCOMMAND 消息，由系统执行 Windows 环境中多数窗口所公用的各种通用操作，例如，画窗口的非用户区，更新窗口的正文标题等等等等。

​		再提示一下，以 WM_ 的消息在 Windows 头文件中都被定义成了常量，如 WM_QUIT = XXXXXXXXXXX，但我们没有必要记住这个数值，也不可能记得住，我们只要知道 WM_QUIT 就 OK 了。

​		在第二只小板凳中我们只让窗口过程处理了两个消息：一个是 WM_PAINT，另一个是 WM_DESTROY，先说说第一个消息--- WM_PAINT.

##### 关于WM_PAINT:

​		无论何时 Windows 要求重画当前窗口时，都会发该消息。也可以这样说：无论何时窗口非法，都必须进行重画。 哎呀，什么又是"非法窗口"？什么又是重画啊？你这人有没有完，嗯？

​		稍安勿燥，我比你还烦呢？我午饭到现在还没吃呢！你有点耐心，来点专业精神好不好？？？我开始在MSDN里面找有关这个方面的内容了，别急，我找找看:

Platform SDK-->Graphics and Multimedia Services-->Windows GDI-->Painting and Drawing-->Using the WM_PAINT Message-----终于找到了。

下面是一大套理论：

​		让我们把 Windows 的屏幕想像成一个桌面，把一个窗口想像成一张纸。当我们把一张纸放到桌面上时，它会盖住其他的纸，这样被盖住的其他纸上的内容都看不到了。但我们只要把这张纸移开，被盖住的其他纸上的内容就会显示出来了---这是一个很简单的道理，谁都明白。

​		对于我们的屏幕来说，当一个窗口被另一窗口盖住时，被盖住的窗口的某些部分就看不到了，我们要想看到被盖住的窗口的全部面貌，就要把另一个窗口移开，但是当我们移开后，事情却起了变化-----很可能这个被盖住的窗口上的信息被擦除了或是丢失了。当窗口中的数据丢失或过期时，窗口就变成非法的了---或者称为"无效"。于是我们的任务就来了，我们必须考虑怎样在窗口的信息丢失时"重画窗口"--使窗口恢复成以前的那个样子。这也就是我们在这第二只小板凳中调用 UpdateWindow 的原因。

你忘记了吗？刚才我们在(三)显示和更新窗口中有下面的一些文字:

​		**<u>WinMain() 调用完 ShowWindow 后，还需要调用函数UpdateWindow，最终把窗口显示了出来。调用函数 UpdateWindow 将产生一个 WM_PAINT 消息，这个消息将使窗口重画，即使窗口得到更新.---这是程序第一次调用了这条消息。</u>**

为重新显示非法区域，Windows 就发送 WM_PAINT 消息实现。要求 Windows 发送 WM_PAINT 的情况有改变窗口大小，对话框关闭，使用了 UpdateWindows 和ScrollWindow 函数等。这里注意，Windows 并非是消息 WM_PAINT 的唯一来源，使用 InvalidateRect 或 InvalidateRgn 函数也可以产生绘图窗口的WM_PAINT消息......

​		通常情况下用 BeginPaint() 来响应 WM_PAINT 消息。如果要在没有WM_PAINT 的情况下重画窗口，必须使用 GetDC 函数得到显示缓冲区的句柄。这里面不再扩展。详细见MDSN。

​		这个 BeginPaint 函数会执行准备绘画所需的所有步骤，包括返回你用于输入的句柄。结束则是以 EndPaint();

在调用完 BeginPaint 之后，WndProc 接着调用 GetClientRect:

`GetClientRect(hwnd,&rect);`

​		第一个参数是程序窗口的句柄。第二个参数是一个指针，指向一个 RECT 类型的结构。查MSDN，可看到这个结构有四个成员。

​		WndProc做了一件事，他把这个 RECT 结构的指针传送给了 DrawText 的第四个参数。函数 DrawText 的目的就是在窗口上显示一行字----"你好,欢迎你来到VC之路!",有关这个函数的具体用法这里也没必要说了吧。

##### 关于WM_DESTROY

​		这个消息要比 WM_PAINT 消息容易处理得多：只要用户关闭窗口，就会发送WM_DESTROY 消息(在窗口从屏幕上移去后）。

程序通过调用P ostQuitMessage 以标准方式响应 WM_DESTROY 消息:

PostQuitMessage (0) ;

​		这个函数在程序的消息队列中插入一个WM_QUIT消息。在(四)创建消息循环中我们曾有这么一段话：

​		消息循环以GetMessage调用开始，它从消息队列中取出一个消息：

​		.......

​		在接收到除 WM_QUIT 之外的任何一个消息后，GetMessage() 都返回 TRUE。如果 GetMessage 收到一个 WM_QUIT 消息，则返回 FALSE，如收到其他消息，则返回 TRUE。因此，在接收到 WM_QUIT 之前，带有 GetMessage() 的消息循环可以一直循环下去。只有当收到的消息是 WM_QUIT 时，GetMessage 才返回 FALSE，结束消息循环，从而终止应用程序。



# 问题

## GetMessage、 PeekMessage区别？

​		在 Windows 的内部，GetMessage 和 PeekMessage 执行着相同的代码，Peekmessage 和 Getmessage 都是向系统的消息队列中取得消息，并将其放置在指定的结构。

区别：

PeekMessage：
		有消息时返回TRUE，没有消息返回FALSE， 不区分该消息是否为WM_QUIT。

GetMessage ：
1. 有消息时且消息不为WM_QUIT时返回TRUE；
2. 如果有消息且为WM_QUIT则返回FALSE；
3. 如果出现错误，函数返回-1；
4. 没有消息时该函数不返回，会阻塞，直到消息出现。

对于取得消息后的不同行为：

GetMessage ：取得消息后，删除除 WM_PAINT 消息以外的消息。
PeekMessage：取得消息后，根据 wRemoveMsg 参数判断是否删除消息。PM_REMOVE 则删除，PM_NOREMOVE 不删除。

函数原型和消息循环：

GetMessage说明

```c++
//函数原型
BOOL GetMessage(
  LPMSG lpMsg,         // address of structure with message
  HWND hWnd,           // handle of window
  UINT wMsgFilterMin,  // first message
  UINT wMsgFilterMax   // last message
);

//消息循环
//Because the return value can be nonzero, zero, or -1,遇到WM_QUIT退出
while( (bRet = GetMessage( &msg, hWnd, 0, 0 )) != 0) { 
    if (bRet == -1) {
        // handle the error and possibly exit，异常情况处理
    } else {
        TranslateMessage(&msg); 
        DispatchMessage(&msg); 
    }
}
```


PeekMessage说明

```c++
//函数原型：
BOOL PeekMessage(
  LPMSG lpMsg,         // pointer to structure for message
  HWND hWnd,           // handle to window
  UINT wMsgFilterMin,  // first message
  UINT wMsgFilterMax,  // last message
  UINT wRemoveMsg      // removal flags
);

//消息循环
while (TRUE) {        
    if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {        
          if (msg.message == WM_QUIT)        
                  break;        
           TranslateMessage (&msg);        
           DispatchMessage (&msg);        
    } else {        
            // 处理空闲任务        
    }       
}
```

 



