from socket import *
# 该socket模块形成了在Python中所有网络通信的基础。包括了这行，我们将能够在程序中创建套接字。

serverPort = 12000
serverSocket = socket(AF_INET, SOCK_DGRAM)

serverSocket.bind(('',serverPort))
#上面行将端口号12000与个服务器的套接字绑定（即分配）在一起。

print ("the server is ready to receive ")

while 1:
    message, clientAddress = serverSocket.recvfrom(2048)
    #这行代码类似于我们在UDPClient中看到的。当某分组到达该服务器的套接字时，该分组的数据被放置到变量message中，其源地址被放置到变量clientAddress中。
    # 变量clientAddress包含了客户的IP地址和客户的端口号。这里，UDPServer将利用该地址信息，因为它提供了返回地址，类似于普通邮政邮件的返回地址。
    # 使用该源地址信息，服务器此时知道了它应当将回答发向何处

    modifiedMessage = message.upper()
    #此行是这个简单应用程序的关键部分。它获取由客户发送的行并使用方法upper()将其转换为大写。

    serverSocket.sendto(modifiedMessage, clientAddress)
    #最后一行将该客户的地址（IP地址和端口号）附到大写报文上，并将所得的分组发送到服务器的套接字中。