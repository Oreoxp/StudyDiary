[TOC]

## 解析流程

当html解析结束后，就开始解析 Stlye 了，入口如下：

```C++
document::ptr document::createFromString(
	const estring& str, 
	document_container* container, 
	const string& master_styles, 
	const string& user_styles )
{
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

	if (master_styles != "")
	{
		doc->m_master_css.parse_stylesheet(master_styles.c_str(), nullptr, doc, nullptr);
		doc->m_master_css.sort_selectors();
	}
	if (user_styles != "")
	{
		doc->m_user_css.parse_stylesheet(user_styles.c_str(), nullptr, doc, nullptr);
		doc->m_user_css.sort_selectors();
	}
```

​		首先解析的是 master css，再解析 user css

<!--`master_styles` 是由浏览器或文档开发者预定义的样式-->

<!--`user_styles` 是由文档的作者或最终用户提供的样式 user_styles 优先级高-->

**`litehtml::css::parse_stylesheet`** 实现如下：

```c++
void litehtml::css::parse_stylesheet(const char* str, const char* baseurl, const std::shared_ptr<document>& doc, const media_query_list::ptr& media) {
	string text = str;

	// remove comments
	string::size_type c_start = text.find("/*");
	while(c_start != string::npos) {
		string::size_type c_end = text.find("*/", c_start + 2);
		if(c_end == string::npos) {
			text.erase(c_start);
			break;
		}
		text.erase(c_start, c_end - c_start + 2);
		c_start = text.find("/*");
	}

	string::size_type pos = text.find_first_not_of(" \n\r\t");
	while(pos != string::npos) {
		while(pos != string::npos && text[pos] == '@') {
			string::size_type sPos = pos;
			pos = text.find_first_of("{;", pos);
			if(pos != string::npos && text[pos] == '{') {
				pos = find_close_bracket(text, pos, '{', '}');
			}
			if(pos != string::npos) {
				parse_atrule(text.substr(sPos, pos - sPos + 1), baseurl, doc, media);
			} else {
				parse_atrule(text.substr(sPos), baseurl, doc, media);
			}

			if(pos != string::npos) {
				pos = text.find_first_not_of(" \n\r\t", pos + 1);
			}
		}

		if(pos == string::npos) {
			break;
		}

		string::size_type style_start	= text.find('{', pos);
		string::size_type style_end	= text.find('}', pos);
		if(style_start != string::npos && style_end != string::npos) {
			auto str_style = text.substr(style_start + 1, style_end - style_start - 1);
			style::ptr style = std::make_shared<litehtml::style>();
			style->add(str_style, baseurl ? baseurl : "", doc->container());

			parse_selectors(text.substr(pos, style_start - pos), style, media);

			if(media && doc) {
				doc->add_media_list(media);
			}

			pos = style_end + 1;
		} else {
			pos = string::npos;
		}

		if(pos != string::npos) {
			pos = text.find_first_not_of(" \n\r\t", pos);
		}
	}
}
```



### 步骤 1：删除注释

首先，函数移除 CSS 样式表中的注释。

```c++
string text = str;

// remove comments
string::size_type c_start = text.find("/*");
while(c_start != string::npos) {
    string::size_type c_end = text.find("*/", c_start + 2);
    if(c_end == string::npos) {
        text.erase(c_start);
        break;
    }
    text.erase(c_start, c_end - c_start + 2);
    c_start = text.find("/*");
}

```

使用 `find` 方法找到注释的起始位置 (`/*`)。

使用 `find` 方法找到注释的结束位置 (`*/`)。

删除注释内容。

如果找不到注释结束符，则删除从起始位置到字符串末尾的所有内容。



### 步骤 2：解析 @ 规则

接下来，函数解析 `@` 开头的规则（如 `@media`, `@import` 等）。

```c++
string::size_type pos = text.find_first_not_of(" \n\r\t");
while(pos != string::npos) {
    while(pos != string::npos && text[pos] == '@') {
        string::size_type sPos = pos;
        pos = text.find_first_of("{;", pos);
        if(pos != string::npos && text[pos] == '{') {
            pos = find_close_bracket(text, pos, '{', '}');
        }
        if(pos != string::npos) {
            parse_atrule(text.substr(sPos, pos - sPos + 1), baseurl, doc, media);
        } else {
            parse_atrule(text.substr(sPos), baseurl, doc, media);
        }

        if(pos != string::npos) {
            pos = text.find_first_not_of(" \n\r\t", pos + 1);
        }
    }
```

- `find_first_not_of` 方法跳过空白字符。
- 查找以 `@` 开头的规则，并使用 `find_first_of` 查找规则的结束位置（`{` 或 `;`）。
- 如果找到 `{`，则调用 `find_close_bracket` 方法找到匹配的 `}`。
- 使用 `parse_atrule` 方法解析 `@` 规则。
- 更新位置索引以继续解析后续内容。



### 步骤 3：解析样式规则

函数继续解析样式规则，找到每个样式规则的选择器和样式体。

```c++
if(pos == string::npos) {
        break;
    }

    string::size_type style_start = text.find('{', pos);
    string::size_type style_end = text.find('}', pos);
    if(style_start != string::npos && style_end != string::npos) {
        auto str_style = text.substr(style_start + 1, style_end - style_start - 1);
        style::ptr style = std::make_shared<litehtml::style>();
        style->add(str_style, baseurl ? baseurl : "", doc->container());

        parse_selectors(text.substr(pos, style_start - pos), style, media);

        if(media && doc) {
            doc->add_media_list(media);
        }

        pos = style_end + 1;
    } else {
        pos = string::npos;
    }

    if(pos != string::npos) {
        pos = text.find_first_not_of(" \n\r\t", pos);
    }
}
```

- 使用 `find` 方法找到样式体的开始和结束位置（`{` 和 `}`）。
- 提取样式体内容，并创建一个 `litehtml::style` 对象。
- 使用 `style->add` 方法将样式体内容添加到 `style` 对象中。
- 使用 `parse_selectors` 方法解析选择器，并将选择器与样式体关联。
- 如果存在媒体查询，将其添加到文档的媒体查询列表中。
- 更新位置索引以继续解析后续内容。



### 总结

`litehtml::css::parse_stylesheet` 函数的核心步骤如下：

1. **删除注释**：移除 CSS 样式表中的所有注释。
2. **解析 @ 规则**：处理 `@` 开头的规则（如 `@media`、`@import` 等）。
3. **解析样式规则**：提取并解析每个样式规则的选择器和样式体，将其转换为 `litehtml` 内部数据结构。

该函数的实现逻辑清晰，利用 STL 字符串方法和自定义解析函数（如 `parse_atrule`, `parse_selectors` 等），实现了对 CSS 样式表的有效解析。



## litehtml::style

​		我们在流程中看到了 `litehtml::style` 起到了举足轻重的作用，**`litehtml::style` 类用于解析和存储 CSS 样式属性。它主要负责将 CSS 字符串解析成内部的样式表示形式，并提供访问和操作这些样式的方法。**以下是对该类的详细解析：

```c++
	using property_value_base = std::variant<
		invalid,
		inherit,
		int,
		int_vector,
		css_length,
		length_vector,
		float,
		web_color,
		string,
		string_vector,
		size_vector
	>;

	struct property_value : property_value_base
	{
		bool m_important = false;
		bool m_has_var   = false; // string; parsing is delayed because of var()

		property_value() {}
		template<class T> property_value(const T& val, bool important, bool has_var = false) 
			: property_value_base(val), m_important(important), m_has_var(has_var) {}

		template<class T> bool is() const { return std::holds_alternative<T>(*this); }
		template<class T> const T& get() const { return std::get<T>(*this); }
	};

	typedef std::map<string_id, property_value>	props_map;
```

​		`property_value` 是一个扩展的 `std::variant`，用于表示不同类型的 CSS 属性值。它包含多个可能的类型，例如 `int`、`float`、`string`、`web_color` 等。

​		它还包含两个布尔成员 `m_important` 和 `m_has_var`，分别表示该属性是否为 `important` 以及是否包含 `var()` 函数。

```c++
	class style
	{
	public:
		typedef std::shared_ptr<style>		ptr;
		typedef std::vector<style::ptr>		vector;
	private:
		props_map							m_properties;
		static std::map<string_id, string>	m_valid_values;
	public:
		void add(const string& txt, const string& baseurl = "", document_container* container = nullptr)
		{
			parse(txt, baseurl, container);
		}

		void add_property(string_id name, const string& val, const string& baseurl = "", bool important = false, document_container* container = nullptr);

		const property_value& get_property(string_id name) const;

		void combine(const style& src);
		void clear()
		{
			m_properties.clear();
		}

		void subst_vars(const html_tag* el);

	private:
		void parse_property(const string& txt, const string& baseurl, document_container* container);
		void parse(const string& txt, const string& baseurl, document_container* container);
		void parse_background(const string& val, const string& baseurl, bool important, document_container* container);
		bool parse_one_background(const string& val, document_container* container, background& bg);
		void parse_background_image(const string& val, const string& baseurl, bool important);
		// parse comma-separated list of keywords
		void parse_keyword_comma_list(string_id name, const string& val, bool important);
		void parse_background_position(const string& val, bool important);
		bool parse_one_background_position(const string& val, css_length& x, css_length& y);
		void parse_background_size(const string& val, bool important);
		bool parse_one_background_size(const string& val, css_size& size);
		void parse_font(const string& val, bool important);
		void parse_flex(const string& val, bool important);
		void parse_align_self(string_id name, const string& val, bool important);
		static css_length parse_border_width(const string& str);
		static void parse_two_lengths(const string& str, css_length len[2]);
		static int parse_four_lengths(const string& str, css_length len[4]);
		static void subst_vars_(string& str, const html_tag* el);

		void add_parsed_property(string_id name, const property_value& propval);
		void remove_property(string_id name, bool important);
	};
```

​		`style` 类负责解析和存储 CSS 样式属性。它主要使用一个 `props_map` 映射将属性名 (`string_id`) 与 `property_value` 对象关联起来。

​		`m_valid_values` 是一个静态成员，包含了各种 CSS 属性的有效值，用于在解析过程中进行验证。



























