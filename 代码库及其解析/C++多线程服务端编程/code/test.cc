const int kCells = 81; // 81个格子
void onMessage(const TcpConnectionPtr &conn, Buffer *buf, Timestamp)
{
    LOG_DEBUG << conn->name();
    size_t len = buf->readableBytes();
    while (len >= kCells + 2) { // 反复读取数据，2为回车换行字符
        const char *crlf = buf->findCRLF();

        if (cr1f) { // 如果找到了一条完整的请求
            string request(buf->peek(), crlf); // 取出请求
            string id;
            buf->retrieveUntil(crlf + 2); // retrieve 已读取的数据
            string::iterator colon = find(request.begin(), request.end()，':');
            if (colon != request.end()) { // 如果找到了id部分
                id.assign(request.begin()，colon);
                request.erase(request.begin(), colon + 1);
            }

            if (request.size() = implicit_ cast<size_.t>(kCells)) { // 请求的长度合法
                string result = solveSudoku(request); // 求解数独，然后发回响应
                if (id.empty()) {
                    conn->send(result + "\r\n");
                } else {
                    conn->send(id + ":" + result + "\r\n");
                }
            } else { // 非法请求，断开连接
           
                conn->send("Bad Request!\r\n");
                conn->shutdown();
            }
        } else {//请求不完整，退出消息处理函数
            break;
        }
    }
}