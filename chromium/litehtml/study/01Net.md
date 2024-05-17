[TOC]



# 网络

​		当我们从地址栏输入网址之后，需要使用网络下载网页内容，所以网络在网页加载过程中是很重要的一步。让我们从网络的打开与下载完成来分析litehtml的获取网页流程。



## 一些准备

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

### tordex::http 

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
```

​		在 `tordex` 命名空间中，`http` 和 `http_request` 类是基于 `WinHTTP` 库构建的，用于处理 HTTP 请求和管理连接。

#### class `http_request`

这个类代表单个 HTTP 请求。它具有以下关键特性和方法：

1. **成员变量**:
   - `m_hConnection`, `m_hRequest`: 分别用于存储连接和请求的句柄。
   - `m_sync`: 一个临界区，用于在多线程环境中同步。
   - `m_buffer`: 数据缓冲区，用于接收HTTP响应数据。
   - `m_error`, `m_status`: 分别用于存储错误代码和HTTP状态代码。
   - `m_content_length`, `m_downloaded_length`: 分别存储内容的总长度和已下载的长度。
   - `m_url`: 请求的URL。
   - `m_refCount`: 引用计数，用于管理对象的生命周期。
2. **方法**:
   - `OnFinish`, `OnData`, `OnHeadersReady`: 这些是虚拟方法，应在派生类中实现，以处理完成事件、数据接收和头部准备完毕的逻辑。
   - `create`, `cancel`: 用于启动和取消HTTP请求。
   - `lock`, `unlock`: 锁定和解锁临界区，用于线程安全。
   - `get_content_length`, `get_status_code`: 获取内容长度和状态代码。
   - `add_ref`, `release`: 用于引用计数管理。
3. **回调处理**:
   - `onSendRequestComplete`, `onHeadersAvailable`, `onHandleClosing`, `onRequestError`, `onReadComplete`: 这些方法处理不同的回调事件。

```c++
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

#### class `http`

这个类用于管理HTTP会话和多个请求。

1. **成员变量**:
   - `m_hSession`: 会话句柄。
   - `m_requests`: 存储所有活动的 `http_request` 对象。
   - `m_sync`: 用于同步的临界区。
   - `m_maxConnectionsPerServer`: 每个服务器的最大连接数。
2. **方法**:
   - `open`, `close`: 开启和关闭HTTP会话。
   - `download_file`: 发起文件下载请求。
   - `stop`: 停止所有活动请求。
   - `lock`, `unlock`: 管理临界区，以保证线程安全。
   - `remove_request`: 从列表中移除并释放一个请求。
3. **回调**:
   - `http_callback`: 静态回调函数，根据不同的事件调用 `http_request` 对象的相应方法。



```c++
class web_page : public windows_container
{
	CHTMLViewWnd*				m_parent;
	LONG						m_refCount;
public:
	tordex::http				m_http;
	std::string					m_url;
	litehtml::document::ptr		m_doc;
	std::wstring				m_caption;
	std::wstring				m_cursor;
	std::string					m_base_path;
	HANDLE						m_hWaitDownload;
	std::wstring				m_waited_file;
	std::wstring				m_hash;
	cairo_images_cache			m_images;
public:
	web_page(CHTMLViewWnd* parent);
	virtual ~web_page();

	void load(LPCWSTR url);
	// encoding: as specified in Content-Type HTTP header
	//   it is NULL for local files or if Content-Type header is not present or Content-Type header doesn't contain "charset="
	void on_document_loaded(LPCWSTR file, LPCWSTR encoding, LPCWSTR realUrl);
	void on_image_loaded(LPCWSTR file, LPCWSTR url, bool redraw_only);
	void on_document_error(DWORD dwError, LPCWSTR errMsg);
	void on_waited_finished(DWORD dwError, LPCWSTR file);
	void add_ref();
	void release();
	void get_url(std::wstring& url);

	// litehtml::document_container members
	void set_caption(const char* caption) override;
	void set_base_url(const char* base_url) override;
	void import_css(litehtml::string& text, const litehtml::string& url, litehtml::string& baseurl) override;
	void on_anchor_click(const char* url, const litehtml::element::ptr& el) override;
	void set_cursor(const char* cursor) override;
	void load_image(const char* src, const char* baseurl, bool redraw_on_ready) override;
	void make_url(const char* url, const char* basepath, litehtml::string& out) override;

	cairo_surface_t* get_image(const std::string& url) override;
	void get_client_rect(litehtml::position& client) const  override;
private:
	char*	load_text_file(LPCWSTR path, bool is_html, LPCWSTR defEncoding = L"UTF-8", LPCWSTR forceEncoding = NULL);
	BOOL	download_and_wait(LPCWSTR url);
};
```

### web_page

#### class `web_page`

这个类继承自 `windows_container` 并主要用于**管理和展示一个网页的内容**。

#### 主要特性和方法

1. **成员变量**:
   - `m_parent`: 指向父窗口的指针，用于事件回传和界面更新。
   - `m_refCount`: 引用计数，用于管理对象的生命周期。
   - `m_http`: 用于管理HTTP请求的 `http` 对象。
   - `m_url`: 当前加载的URL。
   - `m_doc`: 一个 `litehtml::document` 对象，代表解析后的HTML文档。
   - `m_caption`, `m_cursor`: 分别存储页面的标题和光标状态。
   - `m_base_path`: 基本URL路径，用于解析相对URL。
   - `m_images`: 图片缓存，用于存储和重用网页中的图片。
   - `m_hWaitDownload`: 用于同步的句柄，特别是在文件下载等待时使用。
2. **方法**:
   - `load`: 加载指定的URL。
   - `on_document_loaded`, `on_image_loaded`, `on_document_error`, `on_waited_finished`: 这些回调函数用于处理文档加载完成、图片加载、加载错误和等待操作完成的情况。
   - `set_caption`, `set_base_url`, `set_cursor`: 设置页面的标题、基本URL和光标。
   - `import_css`, `on_anchor_click`, `load_image`, `make_url`: 用于处理CSS导入、链接点击、图片加载和URL生成。
   - `add_ref`, `release`: 引用计数的管理。
   - `get_url`, `get_client_rect`: 获取当前页面的URL和客户区域的位置。
   - `download_and_wait`: 启动一个下载操作并等待其完成。

```c++
enum web_file_type
{
	web_file_document,
	web_file_image_redraw,
	web_file_image_rerender,
	web_file_waited,
};

class web_file : public tordex::http_request
{
	WCHAR			m_file[MAX_PATH];
	web_page*		m_page;
	web_file_type	m_type;
	HANDLE			m_hFile;
	LPVOID			m_data;
	std::wstring	m_realUrl;
	std::wstring	m_encoding;
public:
	web_file(web_page* page, web_file_type type, LPVOID data = NULL);
	virtual ~web_file();

	virtual void OnFinish(DWORD dwError, LPCWSTR errMsg);
	virtual void OnData(LPCBYTE data, DWORD len, ULONG64 downloaded, ULONG64 total);
	virtual void OnHeadersReady(HINTERNET hRequest);

};
```

#### class `web_file`

这是 `tordex::http_request` 的派生类，专门用于处理Web文件下载任务。

#### 主要特性和方法

1. **成员变量**:
   - `m_file`: 本地文件路径，用于存储下载的数据。
   - `m_page`: 指向关联的 `web_page` 对象的指针。
   - `m_type`: 文件类型，用于区分不同的下载任务（例如文档、图片等）。
   - `m_hFile`: 文件句柄，用于写入下载的数据。
   - `m_data`: 额外的数据指针，可能用于特定情况下的数据存储。
   - `m_realUrl`, `m_encoding`: 实际的URL和编码信息。
2. **方法**:
   - `OnFinish`, `OnData`, `OnHeadersReady`: 用于处理下载完成、数据接收和HTTP头部准备的回调。
   - 构造函数和析构函数管理对象的创建、初始化和清理。













## 开始下载

### 下载网页流程

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

在调用**系统函数** PathIsURL 后，如果是 url ，开始调用 m_http 的 download_file 函数开始下载；如果不是，就相当于**文档加载完成**，对本地文档进行开始解析。这里只分析下载流程。

​		**`http::download_file`** 函数实现如下：

```c++
BOOL tordex::http::download_file( LPCWSTR url, http_request* request )
{
	if(request)
	{
		request->set_parent(this);
		if(request->create(url, m_hSession))
		{
			lock();
			m_requests.push_back(request);
			unlock();
			return TRUE;
		}
	}
	return FALSE;
}
```

​		首先，download_file 函数会，判断 request 是否有效，接着设置 request 的持有者为自己（应该是为了关闭时调用持有者析构自己）。然后 request 使用 url 和 session 来生成请求，如果成功就将自己放到 http 的请求数组中方便管理。

​		m_hSession 为 winhttp 库在初始化时生成的 session 用于 http 中的保持上下文。

​		http_request::create 函数实现如下：

```c++
BOOL tordex::http_request::create( LPCWSTR url, HINTERNET hSession )
{
	m_url	= url;
	m_error = ERROR_SUCCESS;

    //使用URL_COMPONENTS和WinHttpCrackUrl解析url
	URL_COMPONENTS urlComp;

	ZeroMemory(&urlComp, sizeof(urlComp));
	urlComp.dwStructSize = sizeof(urlComp);

	urlComp.dwSchemeLength    = -1;
	urlComp.dwHostNameLength  = -1;
	urlComp.dwUrlPathLength   = -1;
	urlComp.dwExtraInfoLength = -1;

	if(!WinHttpCrackUrl(url, lstrlen(url), 0, &urlComp))
	{
		return FALSE;
	}
	//解析URL并存储各部分
	std::wstring host;
	std::wstring path;
	std::wstring extra;

	host.insert(0, urlComp.lpszHostName, urlComp.dwHostNameLength);
	path.insert(0, urlComp.lpszUrlPath, urlComp.dwUrlPathLength);
	if(urlComp.dwExtraInfoLength)
	{
		extra.insert(0, urlComp.lpszExtraInfo, urlComp.dwExtraInfoLength);
	}

	DWORD flags = 0;
	if(urlComp.nScheme == INTERNET_SCHEME_HTTPS)
	{
		flags = WINHTTP_FLAG_SECURE;
	}

    //创建连接和请求
	m_hConnection = WinHttpConnect(hSession, host.c_str(), urlComp.nPort, 0);

	PCWSTR pwszAcceptTypes[] = {L"*/*", NULL};

	path += extra;

	m_hRequest = WinHttpOpenRequest(m_hConnection, L"GET", path.c_str(), NULL, NULL, pwszAcceptTypes, flags);
    
    

	lock();
	if(!m_hRequest)
	{
		WinHttpCloseHandle(m_hConnection);
		m_hConnection = NULL;
		unlock();
		return FALSE;
	}

	DWORD options = WINHTTP_OPTION_REDIRECT_POLICY_ALWAYS;
	WinHttpSetOption(m_hRequest, WINHTTP_OPTION_REDIRECT_POLICY, &options, sizeof(options));

	if(!WinHttpSendRequest(m_hRequest, NULL, 0, NULL, 0, 0, (DWORD_PTR) this))
	{
		WinHttpCloseHandle(m_hRequest);
		m_hRequest = NULL;
		WinHttpCloseHandle(m_hConnection);
		m_hConnection = NULL;
		unlock();
		return FALSE;
	}
	unlock();

	return TRUE;
}
```

1. **初始化和错误设置**
   - `m_url` 被设置为输入的 URL。
   - `m_error` 设置为 `ERROR_SUCCESS`，表示初始没有错误。
2. **解析URL**
   - `URL_COMPONENTS urlComp` 结构用于存储解析后的URL组成部分。
   - `ZeroMemory` 用于初始化 `urlComp` 结构。
   - `urlComp.dwStructSize` 被设置为结构的大小。
   - 其他 `urlComp` 的长度字段 (`dwSchemeLength`, `dwHostNameLength`, `dwUrlPathLength`, `dwExtraInfoLength`) 被设置为 -1，让 `WinHttpCrackUrl` 函数自动计算长度。
   - `WinHttpCrackUrl` 调用尝试解析输入的URL，如果失败则函数返回 `FALSE`。
3. **从解析结果中提取主机名、路径和额外信息**
   - `host`, `path`, `extra` 三个 `std::wstring` 分别从 `urlComp` 结构中提取对应的部分。
4. **连接设置**
   - 根据 URL 协议设置连接标志。如果是 HTTPS，则设置 `WINHTTP_FLAG_SECURE`。
   - 使用 `WinHttpConnect` 建立到指定主机和端口的连接，存储返回的连接句柄到 `m_hConnection`。
5. **创建HTTP请求**
   - `m_hRequest` 通过 `WinHttpOpenRequest` 创建一个HTTP请求。这个函数使用之前获取的连接句柄、HTTP方法（GET）、完整路径（包括额外信息）、接受类型和安全标志。
6. **请求选项和发送**
   - 在发送请求前，设置请求的重定向策略为始终自动重定向。
   - 使用 `WinHttpSendRequest` 发送创建的请求。如果发送失败，关闭请求和连接句柄，函数返回 `FALSE`。
7. **锁定和解锁**
   - 函数中多次调用 `lock()` 和 `unlock()` 方法保证多线程环境下的线程安全。



看到这里，可以发现 download_file 中并没有下载文件的操作，这是因为什么呢？

​		回到构造时刻，download_file  参数 **request** 指针是由 **web_file** new 出来的，`web_file` 继承自 `http_request`，并覆盖了数据接收和请求完成的处理函数。在数据接收时，`web_file::OnData` 函数会将接收到的数据写入到临时文件中。在请求完成时，`web_file::OnFinish` 函数会根据下载的文件类型执行相应的处理逻辑。这样，通过使用 `web_file` 作为请求对象，可以实现文件的下载和存储。







## 重构



























