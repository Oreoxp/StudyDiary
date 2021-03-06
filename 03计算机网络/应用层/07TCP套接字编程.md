### TCP套接字编程

​		我们已经看到了一些重要的网络应用，下面就探讨一下网络应用程序是如何实际编写的。在2.1节讲过，典型的网络应用是由一对程序（即客户程序和服务器程序）组成的，它们位于两个不同的端系统中。当运行这两个程序时，创建了一个客户进程和一个服务器进程，同时它们通过从套接字读出和写入数据彼此之间进行通信。开发者创建一个网络应用时，其主要任务就是编写客户程序和服务器程序的代码。

​		网络应用程序有两类。一类是实现在协议标准（如一个RFC或某种其他标准文档）中所定义的操作；这样的应用程序又称为“开放”的，因为定义其操作的这些规则人所共知。对于这样的实现，客户程序和服务器程序必须遵守由该RFC所规定的规则。例如，某客户程序可能是FTP协议客户端的一种实现，如在2.3节所描述，该协议由RFC 959明确定义；类似地，其服务器程序能够是FTP服务器协议的一种实现，也明确由RFC 959定义。如果一个开发者编写客户程序的代码，另一个开发者编写服务器程序的代码，并且两者都完全遵从该RFC的各种规则，那么这两个程序将能够交互操作。实际上，今天大多数网络应用程序涉及客户和服务器程序间的通信，这些程序都是由不同的程序员单独开发的。例如，与Apache Web服务器通信的Firefox浏览器，或与BitTorrent跟踪器通信的BitTorrent客户。

​		另一类网络应用程序是专用的网络应用程序。在这种情况下，由客户和服务器程序应用的应用层协议没有公开发布在某RFC中或其他地方。某单独的开发者（或开发团队）创建了客户和服务器程序，并且该开发者用他的代码完全控制程序的功能。但是因为这些代码并没有实现一个开放的协议，其他独立的开发者将不能开发出和该应用程序交互的代码。

​		在本节中，我们将考察研发一个客户-服务器应用程序中的关键问题，我们将“亲历亲为”来实现一个非常简单的客户-服务器应用程序代码。在研发阶段，开发者必须最先做的一个决定是，应用程序是运行在TCP上还是运行在UDP上。前面讲过TCP是面向连接的，并且为两个端系统之间的数据流动提供可靠的字节流通道。UDP是无连接的，从一个端系统向另一个端系统发送独立的数据分组，不对交付提供任何保证。前面也讲过当客户或服务器程序实现了一个由某RFC定义的协议，它应当使用与该协议关联的周知端口号；与之相反，当研发一个专用应用程序，研发者必须注意避免使用这样的周知端口号。（端口号已在2.1节简要讨论过。它们将在第3章中更为详细地涉及。）

​		我们通过一个简单的UDP应用程序和一个简单的TCP应用程序来介绍UDP和TCP套接字编程。我们用Python语言来呈现这些简单的TCP和UDP程序。也可以用Java、C或C++来编写这些程序，而我们选择用Python最主要原因是Python清楚地揭示了关键的套接字概念。使用Python，代码的行数更少，并且向新编程人员解释每一行代码不会有太大困难。如果你不熟悉Python，也用不着担心，只要你有过一些用Java、C或C++编程的经验，就应该很容易看得懂下面的代码。

​		如果读者对用Java进行客户-服务器编程感兴趣，建议你去查看与本书配套的Web网站。事实上，能够在那里找到用Java编写的本节中的所有例子（和相关的实验）。如果读者对用C进行客户-服务器编程感兴趣，有一些优秀参考资料可供使用［Donahoo 2001;Stevens 1997;Frost 1994;Kurose 1996］。我们下面的Python例子具有类似于C的外观和感觉。

#### 2.7.1　UDP套接字编程

​		在本小节中，我们将编写使用UDP的简单客户-服务器程序；在下一小节中，我们将编写使用TCP的简单程序。

​		2.1节讲过，运行在不同机器上的进程彼此通过向套接字发送报文来进行通信。我们说过每个进程好比是一座房子，该进程的套接字则好比是一扇门。应用程序位于房子中门的一侧；运输层位于该门朝外的另一侧。应用程序开发者在套接字的应用层一侧可以控制所有东西；然而，它几乎无法控制运输层一侧。

​		现在我们仔细观察使用UDP套接字的两个通信进程之间的交互。在发送进程能够将数据分组推出套接字之门之前，当使用UDP时，必须先将目的地址附在该分组之上。在该分组传过发送方的套接字之后，因特网将使用该目的地址通过因特网为该分组选路到接收进程的套接字。当分组到达接收套接字时，接收进程将通过该套接字取回分组，进而检查分组的内容并采取适当的动作。

​		因此你可能现在想知道，附在分组上的目的地址包含了什么？如你所期待的，目的主机的IP地址是目的地址的一部分。通过在分组中包括目的地的IP地址，因特网中的路由器将能够通过因特网将分组选路到目的主机。但是因为一台主机可能运行许多网络应用进程，每个进程具有一个或多个套接字，所以在目的主机指定特定的套接字也是必要的。当生成一个套接字时，就为它分配一个称为**端口号（port number）**的标识符。因此，如你所期待的，分组的目的地址也包括该套接字的端口号。归纳起来，发送进程为分组附上的目的地址是由目的主机的IP地址和目的地套接字的端口号组成的。此外，如我们很快将看到的那样，发送方的源地址也是由源主机的IP地址和源套接字的端口号组成，该源地址也要附在分组之上。然而，将源地址附在分组之上通常并不是由UDP应用程序代码所为，而是由底层操作系统自动完成的。

​		我们将使用下列简单的客户-服务器应用程序来演示对于UDP和TCP的套接字编程：
1）客户从其键盘读取一行字符并将数据向服务器发送。
2）服务器接收该数据并将这些字符转换为大写。
3）服务器将修改的数据发送给客户。
4）客户接收修改的数据并在其监视器上将该行显示出来。
​		图2-28着重显示了客户和服务器的主要与套接字相关的活动，两者通过UDP运输服务进行通信。

![image](https://yqfile.alicdn.com/f3a0f4417d3d8aa377ee4f987f52623a4031c0fc.png)


​		现在我们自己动手来查看这对客户-服务器程序，用UDP实现这个简单应用程序。我们在每个程序后也提供一个详细、逐行的分析。我们将以UDP客户开始，该程序将向服务器发送一个简单的应用级报文。服务器为了能够接收并回答该客户的报文，它必须准备好并已经在运行，这就是说，在客户发送其报文之前，<u>服务器必须作为一个进程正在运行</u>。

​		客户程序被称为UDPClient.py，服务器程序被称为UDPServer.py。为了强调关键问题，我们有意提供最少的代码。“好代码”无疑将具有一些更为辅助性的代码行，特别是用于处理出现差错的情况。对于本应用程序，我们任意选择了12000作为服务器的端口号。

1. UDPClient.py
   下面是该应用程序客户端的代码：

![image](https://yqfile.alicdn.com/fdcdcb944e36b1bbd32be71823fbd5f58e39a8e4.png)

现在我们看在UDPClient.py中的各行代码。
![image](https://yqfile.alicdn.com/9db788903d20035264574732c5e49601f0953e7e.png)

​		该socket模块形成了在Python中所有网络通信的基础。包括了这行，我们将能够在程序中创建套接字。
![image](https://yqfile.alicdn.com/4f80607f04838ca3dd98c3a6a90402b4450184f0.png)

​		第一行将变量serverName置为字符串“hostname”。这里，我们提供了或者包含服务器的IP地址（如“128.138.32.126”）或者包含服务器的主机名（如“cis.poly.edu”）的字符串。如果我们使用主机名，则将自动执行DNS lookup从而得到IP地址。第二行将整数变量serverPort置为12000。
![image](https://yqfile.alicdn.com/6d4dc7e03df3da339287d4eda28ec3091ff5fd2d.png)

​		该行创建了客户的套接字，称为clientSocket。第一个参数指示了地址簇；特别是，AF_INET指示了底层网络使用了IPv4。（此时不必担心，我们将在第4章中讨论IPv4。）第二个参数指示了该套接字是SOCK_DGRAM类型的，这意味着它是一个UDP套接字（而不是一个TCP套接字）。值得注意的是，当创建套接字时，我们并没有指定客户套接字的端口号；相反，我们让操作系统为我们做这件事。既然客户进程的门已经创建，我们将要生成通过该门发送的报文。
![image](https://yqfile.alicdn.com/48a75a2d2536ca2a010f9c09f969f421de68567a.png)

​		raw_input()是Python中的内置功能。当执行这条命令时，在客户上的用户将以单词“Input lowercase sentence:”进行提示，用户使用她的键盘来输入一行，这被放入变量message中。既然我们有了一个套接字和一条报文，我们将要通过该套接字向目的主机发送报文。
![image](https://yqfile.alicdn.com/e316361a1c64e7e8a2f29fb2acddcae35d56942b.png)

​		在上述这行中，方法sendto()为报文附上目的地址(serverName，serverPort)并且向进程的套接字clientSocket发送结果分组。（如前面所述，源地址也附到分组上，尽管这是自动完成的，而不是显式地由代码完成的。）经一个UDP套接字发送一个客户到服务器的报文非常简单!在发送分组之后，客户等待接收来自服务器的数据。
![image](https://yqfile.alicdn.com/2e257972d8e67b9dee13e0b1e2ed1e9ee2ef6606.png)

​		对于上述这行，当一个来自因特网的分组到达该客户套接字时，该分组的数据被放置到变量modifiedMessage中，其源地址被放置到变量serverAddress中。变量serverAddress包含了服务器的IP地址和服务器的端口号。程序UDPClient实际上并不需要服务器的地址信息，因为它从起始就已经知道了该服务器地址；而这行Python代码仍然提供了服务器的地址。方法recvfrom也取缓存长度2048作为输入。（该缓存长度用于多种目的。）
![image](https://yqfile.alicdn.com/e5d0d1b83363799740c835e54d365c0c4026f5a6.png)

​		这行在用户显示器上打印出modifiedMessage。它应当是变用户键入的原始行，现在只是变为大写的了。
![image](https://yqfile.alicdn.com/7c277fc2bbe28f84916c5ce4b3613017fa6fe0a2.png)

​		该行关闭了套接字。然后关闭了该进程。

1. UDPServer.py
   现在来看看这个应用程序的服务器端：

![image](https://yqfile.alicdn.com/f0575feafb7314f0bef9ec0bde8e153ca2dec70d.png)

​		注意到UDPServer的开始部分与UDPClient类似。它也是导入套接字模块，也将整数变量serverPort设置为12000，并且也创建套接字类型SOCK_DGRAM（一种UDP套接字）。与UDPClient有很大不同的第一行代码是：
![image](https://yqfile.alicdn.com/f5beb4264076942814a7714b6655f764d330e623.png)

​		上面行将端口号12000与个服务器的套接字绑定（即分配）在一起。因此在UDPServer中，（由应用程序开发者编写的）代码显式地为该套接字分配一个端口号。以这种方式，当任何人向位于该服务器的IP地址的端口12000发送一个分组，该分组将指向该套接字。UDPServer然后进入一个while循环；该while循环将允许UDPServer无限期地接收并处理来自客户的分组。在该while循环中，UDPServer等待一个分组的到达。
![image](https://yqfile.alicdn.com/0862d9caefe537de98bf911dfd11df2317fc0f51.png)

​		这行代码类似于我们在UDPClient中看到的。当某分组到达该服务器的套接字时，该分组的数据被放置到变量message中，其源地址被放置到变量clientAddress中。变量clientAddress包含了客户的IP地址和客户的端口号。这里，UDPServer将利用该地址信息，因为它提供了返回地址，类似于普通邮政邮件的返回地址。使用该源地址信息，服务器此时知道了它应当将回答发向何处。
![image](https://yqfile.alicdn.com/ce8af32abff5875876d64d00fe5019cc46be7542.png)

​		此行是这个简单应用程序的关键部分。它获取由客户发送的行并使用方法upper()将其转换为大写。
![image](https://yqfile.alicdn.com/dc49bca5fc9695b4dfe7e289624b53fe06013f92.png)

​		最后一行将该客户的地址（IP地址和端口号）附到大写报文上，并将所得的分组发送到服务器的套接字中。（如前面所述，服务器地址也附在分组上，尽管这是自动而不是显式地由代码完成的。）因特网则将分组交付到该客户地址。在服务器发送该分组后，它仍维持在while循环中，等待（从运行在任一台主机上的任何客户发送的）另一个UDP分组到达。

​		为了测试这对程序，可在一台主机上运行UDPClient.py，并在另一台主机上运行UDPServer.py。保证在UDPClient.py中包括适当的服务器主机名或IP地址。接下来，在服务器主机上执行编译的服务器程序			

​		UDPServer.py。这在服务器上创建了一个进程，等待着某个客户与之联系。然后，在客户主机上执行编译的客户器程序UDPClient.py。这在客户上创建了一个进程。最后，在客户上使用应用程序，键入一个句子并以回车结束。

​		可以通过稍加修改上述客户和服务器程序来研制自己的UDP客户-服务器程序。例如，不必将所有字母转换为大写，服务器可以计算字母s出现的次数并返回该数字。或者能够修改客户程序，使得收到一个大写的句子后，用户能够向服务器继续发送更多的句子。



### 2.7.2　TCP套接字编程

​		与UDP不同，TCP是一个面向连接的协议。这意味着在客户和服务器能够开始互相发送数据之前，它们先要握手和创建一个TCP连接。TCP连接的一端与客户套接字相联系，另一端与服务器套接字相联系。当创建该TCP连接时，我们将其与客户套接字地址（IP地址和端口号）和服务器套接字地址（IP地址和端口号）关联起来。使用创建的TCP连接，当一侧要向另一侧发送数据时，它只需经过其套接字将数据丢给TCP连接。这与UDP不同，UDP服务器在将分组丢进套接字之前必须为其附上一个目的地地址。

​		现在我们仔细观察一下TCP中客户程序和服务器程序的交互。客户具有向服务器发起接触的任务。服务器为了能够对客户的初始接触做出反应，服务器必须已经准备好。这意味着两件事。第一，与在UDP中的情况一样，TCP服务器在客户试图发起接触前必须作为进程运行起来。第二，服务器程序必须具有一扇特殊的门，更精确地说是一个特殊的套接字，该门欢迎来自运行在任意主机上的客户进程的某些初始接触。使用房子/门来比喻进程/套接字，有时我们将客户的初始接触称为“敲欢迎之门”。

​		随着服务器进程的运行，客户进程能够向服务器发起一个TCP连接。这是由客户程序通过创建一个TCP套接字完成的。当该客户生成其TCP套接字时，它指定了服务器中的欢迎套接字的地址，即服务器主机的IP地址及其套接字的端口号。生成其套接字后，该客户发起了一个三次握手并创建与服务器的一个TCP连接。发生在运输层的三次握手，对于客户和服务器程序是完全透明的。

​		在三次握手期间，客户进程敲服务器进程的欢迎之门。当该服务器“听”到敲门时，它将生成一扇新门（更精确地讲是一个新套接字），它专门用于特定的客户。在我们下面的例子中，欢迎之门是一个我们称为serverSocket的TCP套接字对象；它专门对客户进行连接的新生成的套接字，称为连接套接字（connection Socket）。初次遇到TCP套接字的学生有时会混淆欢迎套接字（这是所有要与服务器通信的客户的起始接触点）和每个新生成的服务器侧的连接套接字（这是随后为与每个客户通信而生成的套接字）。

​		应用程序的观点来看，客户套接字和服务器连接套接字直接通过一根管道连接。如图2-29所示，客户进程可以向它的套接字发送任意字节，并且TCP保证服务器进程能够按发送的顺序接收（通过连接套接字）每个字节。TCP因此在客户和服务器进程之间提供了可靠服务。此外，就像人们可以从同一扇门进和出一样，客户进程不仅能向它的套接字发送字节，也能从中接收字节；类似地，服务器进程不仅从它的连接套接字接收字节，也能向其发送字节。

![image](https://yqfile.alicdn.com/449d86f1fb6c99e9dda16b5e075cf1e77ec2289a.png)


​		我们使用同样简单的客户-服务器应用程序来展示TCP套接字编程：客户向服务器发送一行数据，服务器将这行改为大写并回送给客户。图2-30着重显示了客户和服务器的主要与套接字相关的活动，两者通过TCP运输服务进行通信。

![image](https://yqfile.alicdn.com/4054507a62b63bc6c0ce7c4ac9496aa6979190b6.png)



1. TCPClient.py
   这里给出了应用程序客户端的代码：

![image](https://yqfile.alicdn.com/f02b93c58367636090e932ce7cf14062f10539b1.png)

现在我们查看这些代码中的与UDP实现有很大差别的各行。首当其冲的行是客户套接字的创建。
![image](https://yqfile.alicdn.com/6ed4d7f31f38c385d68b8fda5470f02e56b74e18.png)

​		该行创建了客户的套接字，称为clientSocket。第一个参数仍指示底层网络使用的是IPv4。第二个参数指示该套接字是SOCK_STREAM类型。这表明它是一个TCP套接字（而不是一个UDP套接字）。值得注意的是当我们创建该客户套接字时仍未指定其端口号；相反，我们让操作系统为我们做此事。此时的下一行代码与我们在UDPClient中看到的极为不同：
![image](https://yqfile.alicdn.com/02b3218e08fa64f99566f2f38ca06f6f28853eb9.png)

​		前面讲过在客户能够使用一个TCP套接字向服务器发送数据之前（反之亦然），必须在客户与服务器之间创建一个TCP连接。上面这行就发起了客户和服务器之间的这条TCP连接。connect()方法的参数是这条连接中服务器端的地址。这行代码执行完后，执行三次握手，并在客户和服务器之间创建起一条TCP连接。

​		如同UDPClient一样，上一行从用户获得了一个句子。字符串sentence连续收集字符直到用户键入回车以终止该行为止。代码的下一行也与UDPClient极为不同：
![image](https://yqfile.alicdn.com/9c7a858ae3a044a6c45ca9d2eb468b5a5645379b.png)

​		上一行通过该客户的套接字并进入TCP连接发送字符串sentence。值得注意的是，该程序并未显式地创建一个分组并为该分组附上目的地址，而使用UDP套接字却要那样做。相反，客户程序只是将字符串sentence中的字节放入该TCP连接中去。客户然后就等待接收来自服务器的字节。
![image](https://yqfile.alicdn.com/f36fc4343129bfe0008c0c35528761a4664dabca.png)

​		当字符到达服务器时，它们被放置在字符串modifiedSentence中。字符继续积累在modifiedSentence中，直到收到回车符才会结束该行。在打印大写句子后，我们关闭客户的套接字。
![image](https://yqfile.alicdn.com/2f0e6a55a745207ac19be2ac1aee8a0fbcd56e74.png)

​		最后一行关闭了套接字，因此关闭了客户和服务器之间的TCP连接。它引起客户中的TCP向服务器中的TCP发送一条TCP报文（参见3.5节）。

2.TCPServer.py
现在我们看一下服务器程序。

![image](https://yqfile.alicdn.com/8d450a19413b8609c67e76ba3575d21415fabae4.png)

现在我们来看看上述与UDPServer及TCPClient有显著不同的代码行。与TCPClient相同的是，服务器创建一个TCP套接字，执行：
![image](https://yqfile.alicdn.com/4b05d2b4b8f99d5953675d35781b5ead57e64d2a.png)

​		与UDPServer类似，我们将服务器的端口号serverPort与该套接字关联起来：
![image](https://yqfile.alicdn.com/1f20a1e98bf43c81644d9b2707948887426cc285.png)

​		但对TCP而言，serverSocket将是我们的欢迎套接字。在创建这扇欢迎之门后，我们将等待并聆听某个客户敲门：
![image](https://yqfile.alicdn.com/7c87f0f370caece4c9620437000eb11614cd5f2d.png)

​		该行让服务器聆听来自客户的TCP连接请求。其中参数定义了请求连接的最大数（至少为1）。
![image](https://yqfile.alicdn.com/46a337e251e6f83e8d0ec572654fcda9ec4b4c23.png)

​		当客户敲该门时，程序为serverSocket调用accept()，这在服务器中创建了一个称为connectionSocket的新套接字，由这个特定的客户专用。客户和服务器则完成了握手，在客户的clientSocket和服务器的serverSocket之间创建了一个TCP连接。借助于创建的TCP连接，客户与服务器现在能够通过该连接相互发送字节。使用TCP，从一侧发送的所有字节不仅确保到达另一侧，而且确保按序到达。
![image](https://yqfile.alicdn.com/4f88157871a4f6f95cb586488bc943e26698ed2a.png)

​		在此程序中，在向客户发送修改的句子后，我们关闭了该连接套接字。但由于serverSocket保持打开，所以另一个客户此时能够敲门并向该服务器发送一个句子要求修改。
​		我们现在完成了TCP套接字编程的讨论。建议你在两台单独的主机上运行这两个程序，也可以修改它们以达到稍微不同的目的。你应当将前面两个UDP程序与这两个TCP程序进行比较，观察它们的不同之处。你也应当做在第2、4和7章后面描述的套接字编程作业。最后，我们希望在掌握了这些和更先进的套接字程序后的某天，你将能够编写你自己的流行网络应用程序，变得非常富有和声名卓著，并记得本书的作者!