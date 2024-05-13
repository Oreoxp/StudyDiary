[TOC]



# 网络

​		当我们从地址栏输入网址之后，需要使用网络下载网页内容，所以网络在网页加载过程中是很重要的一步。让我们从网络的打开与下载完成来分析litehtml的获取网页流程。



### 一些准备

​		litebrowser 启动后，会生成一个 `web_page`，`web_page`类通常用于表示和管理一个Web页面的内容和状态。它是整个浏览器渲染和交互的核心部分。`web_page`的主要功能包括：

1. **解析和渲染HTML**：`web_page`负责解析HTML文档，并根据CSS样式构建页面的渲染树（render tree）。这是页面可视内容呈现的基础。
2. **管理资源**：这个类通常也管理与网页相关的资源，如图像、CSS文件和JavaScript脚本。它负责加载这些资源，并在必要时触发重新渲染。
3. **处理用户交互**：`web_page`处理用户的输入，例如点击和滚动，然后响应这些输入，可能是通过JavaScript交互或触发页面的重排（reflow）和重绘（repaint）。
4. **执行JavaScript**：如果litebrowser支持JavaScript，`web_page`会处理JavaScript的执行，包括DOM操作和事件响应。
5. **状态管理**：它还管理页面的状态，比如历史记录（前进和后退功能）和cookies。

```c++
m_page_next = new web_page(this);
m_page_next->m_hash	= hash;
m_page_next->load(s_url.c_str());
```

当htmlview 打开一个页面时，会新生成一个 `web_page`（即 m_page_next）并加载这个 URL，在 `web_page`中，使用 tordex::http 来处理网络请求，并在构建时生成他：

```c++

class web_page : public windows_container
{
.....
	tordex::http				m_http;
	std::string					m_url;
.....
};


web_page::web_page(CHTMLViewWnd* parent) {
	m_refCount		= 1;
	m_parent		= parent;
	m_http.open(L"litebrowser/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS);
}
```

tordex::http 的定义为：

```c++
	class http_request
	{
		friend class http;
	public:
		typedef std::vector<http_request*>	vector;
	protected:
		HINTERNET			m_hConnection;
		HINTERNET			m_hRequest;
		CRITICAL_SECTION	m_sync;
		http*				m_http;
		BYTE				m_buffer[8192];
		DWORD				m_error;
		ULONG64				m_content_length;
		ULONG64				m_downloaded_length;
		DWORD				m_status;
		std::wstring		m_url;
		LONG				m_refCount;
	public:
		http_request();
		virtual ~http_request();

		virtual void OnFinish(DWORD dwError, LPCWSTR errMsg) = 0;
		virtual void OnData(LPCBYTE data, DWORD len, ULONG64 downloaded, ULONG64 total) = 0;
		virtual void OnHeadersReady(HINTERNET hRequest);

		BOOL	create(LPCWSTR url, HINTERNET hSession);
		void	cancel();
		void	lock();
		void	unlock();
		ULONG64	get_content_length();
		DWORD	get_status_code();
		void	add_ref();
		void	release();

	protected:
		DWORD	onSendRequestComplete();
		DWORD	onHeadersAvailable();
		DWORD	onHandleClosing();
		DWORD	onRequestError(DWORD dwError);
		DWORD	onReadComplete(DWORD len);
		DWORD	readData();
		void	set_parent(http* parent);
	};





	class http
	{
		friend class http_request;

		HINTERNET				m_hSession;
		http_request::vector	m_requests;
		CRITICAL_SECTION		m_sync;
		DWORD					m_maxConnectionsPerServer;
	public:
		http();
		virtual ~http();

		void	set_max_connections_per_server(DWORD max_con);
		BOOL	open(LPCWSTR pwszUserAgent, DWORD dwAccessType, LPCWSTR pwszProxyName, LPCWSTR pwszProxyBypass);
		BOOL	download_file(LPCWSTR url, http_request* request);
		void	stop();
		void	close();

		void lock();
		void unlock();

	private:
		static VOID CALLBACK http_callback(HINTERNET hInternet, DWORD_PTR dwContext, DWORD dwInternetStatus, LPVOID lpvStatusInformation, DWORD dwStatusInformationLength);

	protected:
		void remove_request(http_request* request);
	};
```

​		在 `tordex` 命名空间中，`http` 和 `http_request` 类是基于 `WinHTTP` 库构建的，用于处理 HTTP 请求和管理连接。





### 开始下载

### 

当在地址栏输入网址后，web_page 会 load 这个 url:

```c++
void web_page::load( LPCWSTR url ) {
	m_url = cairo_font::wchar_to_utf8(url);
	m_base_path	= m_url;
	if(PathIsURL(url)) {
		m_http.download_file( url, new web_file(this, web_file_document) );
	} else {
		on_document_loaded(url, NULL, NULL);
	}
}
```

在调用系统函数 PathIsURL 后，开始调用 m_http 的 download_file 函数开始下载。

### 完毕