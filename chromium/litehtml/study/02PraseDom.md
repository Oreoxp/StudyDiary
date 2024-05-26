[TOC]





​		当 HTML 下载完成后会使用 `litehtml` 库来解析 HTML 并构建 DOM 树。以下是详细的工作流程，展示了 `litebrowser` 下载 HTML 后是如何解析 HTML 并生成 DOM 树的。

## 下载完成 HTML 内容

​		下载过程请查看前个文档。当下载完成后会调用 web_file 的 OnFinsh 回调，web_file::OnFinish 实现如下：

```c++
void web_file::OnFinish(int status, const std::string& errorMsg, const std::string url) {
    if (m_ofs.is_open()) {
        m_ofs.close();
    }
    if (status != 200) {
        switch (m_type) {
        case web_file_document:
            m_page->on_document_error(status, errorMsg);
            break;
        case web_file_waited:
            m_page->on_waited_finished(status, m_file);
            break;
        }
    } else {
        switch (m_type) {
        case web_file_document:
            m_page->on_document_loaded(m_file, m_encoding, url);
            break;
        case web_file_image_redraw:
            m_page->on_image_loaded(m_file, url, true);
            break;
        case web_file_image_rerender:
            m_page->on_image_loaded(m_file, url, false);
            break;
        case web_file_waited:
            m_page->on_waited_finished(status, m_file);
            break;
        }
    }
}
```

​		当请求错误时进入错误处理逻辑，如果是正确（==200），就根据 file 类型进行下一步解析，这里探究当 type 为 **web_file_document** 时，加载document解析。

## 解析 HTML 内容

​		web_page::on_document_loaded 的实现如下：

```c++
void web_page::on_document_loaded(const std::string& file, const std::string& encoding, const std::string& realUrl)
{
	if (!realUrl.empty())
	{
		m_url = realUrl;
	}
	// 读取 HTML 文件内容
	char* html_text = load_text_file(file, true, "UTF-8", encoding);
    
	// 检查是否成功读取文件内容
	if(!html_text)
	{
		LPCSTR txt = "<h1>Something Wrong</h1>";
		html_text = new char[lstrlenA(txt) + 1];
		lstrcpyA(html_text, txt);
	}
    
	// 创建 litehtml 文档
	m_doc = litehtml::document::createFromString(html_text, this);
	delete html_text;

    // 通知页面加载完成
	PostMessage(m_parent->wnd(), WM_PAGE_LOADED, 0, 0);
}
```

### 读取文件并转码

​		首先调用的是 load_text_file 方法，`web_page::load_text_file` 方法首先读取指定文件的内容，并根据需要进行编码转换。对于 HTML 文件，方法会检查 `<meta>` 标签中的 `charset` 属性来确定编码，并使用 `IMultiLanguage` 接口进行编码转换，确保返回的文本内容为 UTF-8 编码。`web_page::load_text_file` 实现如下：

```c++
char* web_page::load_text_file(const std::string& path, bool is_html, const std::string& defEncoding, const std::string& forceEncoding) {
	char* ret = NULL;
    //文件读取
	HANDLE fl = CreateFile(path.c_str(), GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if(fl != INVALID_HANDLE_VALUE) {
		DWORD size = GetFileSize(fl, NULL);
		ret = new char[size + 1];

		DWORD cbRead = 0;
		if(size >= 3) {
			ReadFile(fl, ret, 3, &cbRead, NULL);
			if(ret[0] == '\xEF' && ret[1] == '\xBB' && ret[2] == '\xBF') {
				ReadFile(fl, ret, size - 3, &cbRead, NULL);
				ret[cbRead] = 0;
			} else {
				ReadFile(fl, ret + 3, size - 3, &cbRead, NULL);
				ret[cbRead + 3] = 0;
			}
		}
		CloseHandle(fl);
	}

	// try to convert encoding
	if(is_html) {
		std::string encoding;
		if (!forceEncoding.empty()) {
			encoding = forceEncoding;
		} else {
			char* begin = StrStrIA((LPSTR)ret, "<meta");
			while (begin && encoding.empty()) {
				char* end = StrStrIA(begin, ">");
				char* s1 = StrStrIA(begin, "Content-Type");
				if (s1 && s1 < end) {
					s1 = StrStrIA(begin, "charset");
					if (s1) {
						s1 += strlen("charset");
						while (!isalnum(s1[0]) && s1 < end) {
							s1++;
						}
						while ((isalnum(s1[0]) || s1[0] == '-') && s1 < end) {
							encoding += s1[0];
							s1++;
						}
					}
				}
				if (encoding.empty()) {
					begin = StrStrIA(begin + strlen("<meta"), "<meta");
				}
			}

			if (encoding.empty() && !defEncoding.empty()) {
				encoding = defEncoding;
			}
		}

		if(!encoding.empty()) {
			if(!StrCmpI(encoding.c_str(), "UTF-8")) {
				encoding.clear();
			}
		}

		if(!encoding.empty()) {
			CoInitialize(NULL);

			IMultiLanguage* ml = NULL;
			HRESULT hr = CoCreateInstance(CLSID_CMultiLanguage, NULL, CLSCTX_INPROC_SERVER, IID_IMultiLanguage, (LPVOID*) &ml);	

			MIMECSETINFO charset_src = {0};
			MIMECSETINFO charset_dst = {0};

			BSTR bstrCharSet = SysAllocString(cairo_font::utf8_to_wchar(encoding).c_str());
			ml->GetCharsetInfo(bstrCharSet, &charset_src);
			SysFreeString(bstrCharSet);

			bstrCharSet = SysAllocString(L"utf-8");
			ml->GetCharsetInfo(bstrCharSet, &charset_dst);
			SysFreeString(bstrCharSet);

			DWORD dwMode = 0;
			UINT  szDst = (UINT) strlen((LPSTR) ret) * 4;
			LPSTR dst = new char[szDst];

			if(ml->ConvertString(&dwMode, charset_src.uiInternetEncoding, charset_dst.uiInternetEncoding, (LPBYTE) ret, NULL, (LPBYTE) dst, &szDst) == S_OK)
			{
				dst[szDst] = 0;
				delete ret;
				ret = dst;
			} else {
				delete dst;
			}
			CoUninitialize();
		}
	}

	return ret;
}
```

#### 		编码处理逻辑

1. **检测编码**：
   - 如果 `forceEncoding` 不为空，直接使用它作为编码。
   - 否则，检查 HTML `<meta>` 标签中的 `charset` 属性，提取编码信息。
   - 如果未找到编码信息，使用 `defEncoding` 作为默认编码。
2. **处理 UTF-8**：
   - 如果编码是 `UTF-8`，无需转换。
3. **编码转换**：
   - 初始化 COM 库并创建 `IMultiLanguage` 接口实例。
   - 获取源编码和目标编码的信息。
   - 使用 `IMultiLanguage::ConvertString` 方法将源编码的字符串转换为目标编码（UTF-8）。
   - 转换成功后，更新缓冲区内容。
   - 清理资源。



然后返回结果，返回包含文件内容的缓冲区指针。需要注意的是，调用该方法后，**调用者负责释放返回的缓冲区**。

### 创建 `litehtml` 文档

​		读取完文件并转码之后，使用 litehtml 解析 html 文档，`litehtml::document::createFromString` 方法将 HTML 字符串解析为 `litehtml` 文档对象：

```c++
namespace litehtml {
document::ptr document::createFromString(
	const estring& str, 
	document_container* container, 
	const string& master_styles, 
	const string& user_styles ) {
	// Create litehtml::document
	document::ptr doc = make_shared<document>(container);

	// Parse document into GumboOutput
	GumboOutput* output = doc->parse_html(str);

	// Create litehtml::elements.
	elements_list root_elements;
	doc->create_node(output->root, root_elements, true);
	if (!root_elements.empty())
	{
		doc->m_root = root_elements.back();
	}
	// Destroy GumboOutput
	gumbo_destroy_output(&kGumboDefaultOptions, output);

	if (master_styles != "") {
		doc->m_master_css.parse_stylesheet(master_styles.c_str(), nullptr, doc, nullptr);
		doc->m_master_css.sort_selectors();
	}
	if (user_styles != "") {
		doc->m_user_css.parse_stylesheet(user_styles.c_str(), nullptr, doc, nullptr);
		doc->m_user_css.sort_selectors();
	}

	// Let's process created elements tree
	if (doc->m_root) {
		doc->container()->get_media_features(doc->m_media);

		doc->m_root->set_pseudo_class(_root_, true);

		// apply master CSS
		doc->m_root->apply_stylesheet(doc->m_master_css);

		// parse elements attributes
		doc->m_root->parse_attributes();

		// parse style sheets linked in document
		media_query_list::ptr media;
		for (const auto& css : doc->m_css) {
			if (!css.media.empty()) {
				media = media_query_list::create_from_string(css.media, doc);
			}
			else {
				media = nullptr;
			}
			doc->m_styles.parse_stylesheet(css.text.c_str(), css.baseurl.c_str(), doc, media);
		}
		// Sort css selectors using CSS rules.
		doc->m_styles.sort_selectors();

		// get current media features
		if (!doc->m_media_lists.empty()) {
			doc->update_media_lists(doc->m_media);
		}

		// Apply parsed styles.
		doc->m_root->apply_stylesheet(doc->m_styles);

		// Apply user styles if any
		doc->m_root->apply_stylesheet(doc->m_user_css);

		// Initialize m_css
		doc->m_root->compute_styles();

		// Create rendering tree
		doc->m_root_render = doc->m_root->create_render_item(nullptr);

		// Now the m_tabular_elements is filled with tabular elements.
		// We have to check the tabular elements for missing table elements 
		// and create the anonymous boxes in visual table layout
		doc->fix_tables_layout();

		// Finally initialize elements
		// init() returns pointer to the render_init element because it can change its type
		doc->m_root_render = doc->m_root_render->init();
	}

	return doc;
}
}
```













### 解析 HTML 字符串

