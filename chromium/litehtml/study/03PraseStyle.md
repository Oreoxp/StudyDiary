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

​		在解析样式规则的时候，我们使用了 **style** 的 `add` 方法，

```c++
void add(const string& txt, const string& baseurl = "", document_container* container = nullptr)
{
    parse(txt, baseurl, container);
}
```





























我们接着往下分析，当解析完 CSS 后，`document::createFromString` 继续执行：

```c++
	// Let's process created elements tree
	if (doc->m_root){
		doc->container()->get_media_features(doc->m_media);

		doc->m_root->set_pseudo_class(_root_, true);

		// apply master CSS
		doc->m_root->apply_stylesheet(doc->m_master_css);

		// parse elements attributes
		doc->m_root->parse_attributes();

		// parse style sheets linked in document
		media_query_list::ptr media;
		for (const auto& css : doc->m_css){
			if (!css.media.empty()){
				media = media_query_list::create_from_string(css.media, doc);
			}else{
				media = nullptr;
			}
			doc->m_styles.parse_stylesheet(css.text.c_str(), css.baseurl.c_str(), doc, media);
		}
		// Sort css selectors using CSS rules.
		doc->m_styles.sort_selectors();

		// get current media features
		if (!doc->m_media_lists.empty()){
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
```

#### 第一步：

​		首先，调用了 `container_cairo::get_media_features` 获取<u>媒体特性</u>。<u>媒体特性</u>是为了允许根据设备的特性（如屏幕尺寸、分辨率、方向等）应用不同的 CSS 规则。例如，<u>通过媒体查询，您可以在小屏幕设备上应用一种样式，而在大屏幕设备上应用另一种样式。</u>

#### 第二步：

​		`doc->m_root->set_pseudo_class(_root_, true);` 设置**伪类（pseudo-class）**

​		伪类（pseudo-class）是一种用于定义元素特殊状态的CSS选择器。常见的伪类包括：

- `:hover`：当用户悬停在元素上时应用样式。

- `:active`：当元素被激活时（例如被点击）应用样式。

- `:focus`：当元素获得焦点时应用样式。

- `:first-child`：选择作为父元素的第一个子元素的元素。

- `:nth-child(n)`：选择作为父元素的第n个子元素的元素。

  ​        伪类在网页交互和动态样式应用中起到重要作用。在 litehtml 中，`_root_`  伪类通常用于标识文档的根元素，即 `<html>` 标签。这是一个自定义的伪类，用于在渲染引擎内部区分根元素和其他普通元素。

  ​		这行代码的作用是将 `_root_` 伪类设置为 `true`，表示 `doc->m_root` 这个根元素处于 `_root_` 状态

#### 第三步  解析应用主CSS样式表的代码：

`litehtml::html_tag::apply_stylesheet` 实现为：

```c++
void litehtml::html_tag::apply_stylesheet( const litehtml::css& stylesheet ){
    if(is_root()){
        int i = 0;
        i++;
    }

    // 遍历样式表中的每一个选择器
    for(const auto& sel : stylesheet.selectors()){
        // 优化选择器匹配
        {
            const auto& r = sel->m_right;
            if (r.m_tag != star_id && r.m_tag != m_tag)
                continue;

            if (!r.m_attrs.empty()){
                const auto& attr = r.m_attrs[0];
                if (attr.type == select_class &&
                    std::find(m_classes.begin(), m_classes.end(), attr.name) == m_classes.end())
                    continue;
            }
        }

        // 检查选择器是否与当前元素匹配
        int apply = select(*sel, false);

        if(apply != select_no_match){
            used_selector::ptr us = std::unique_ptr<used_selector>(new used_selector(sel, false));

            if(sel->is_media_valid()){
                auto apply_before_after = [&](){
                    const auto& content_property = sel->m_style->get_property(_content_);
                    bool content_none = content_property.is<string>() && content_property.get<string>() == "none";
                    bool create = !content_none && (sel->m_right.m_attrs.size() > 1 || sel->m_right.m_tag != star_id);

                    element::ptr el;
                    if(apply & select_match_with_after){
                        el = get_element_after(*sel->m_style, create);
                    } else if(apply & select_match_with_before){
                        el = get_element_before(*sel->m_style, create);
                    } else{
                        return;
                    }

                    if(el){
                        if(!content_none){
                            el->add_style(*sel->m_style);
                        } else{
                            el->parent()->removeChild(el);
                        }
                    } else{
                        if(!content_none){
                            add_style(*sel->m_style);
                        }
                    }
                    us->m_used = true;
                };

                if(apply & select_match_pseudo_class){
                    if(select(*sel, true)){
                        if((apply & (select_match_with_after | select_match_with_before))){
                            apply_before_after();
                        } else{
                            add_style(*sel->m_style);
                            us->m_used = true;
                        }
                    }
                } else if((apply & (select_match_with_after | select_match_with_before))){
                    apply_before_after();
                } else{
                    add_style(*sel->m_style);
                    us->m_used = true;
                }
            }
            m_used_styles.push_back(std::move(us));
        }
    }

    // 递归应用样式表到子元素
    for(auto& el : m_children) {
        if(el->css().get_display() != display_inline_text) {
            el->apply_stylesheet(stylesheet);
        }
    }
}

```

##### 1. 遍历样式表中的选择器

```c++
for(const auto& sel : stylesheet.selectors()) {
```

遍历传入的样式表中的每一个选择器。

##### 2. 选择器匹配优化

```c++
const auto& r = sel->m_right;
if (r.m_tag != star_id && r.m_tag != m_tag)
    continue;

if (!r.m_attrs.empty()) {
    const auto& attr = r.m_attrs[0];
    if (attr.type == select_class &&
        std::find(m_classes.begin(), m_classes.end(), attr.name) == m_classes.end())
        continue;
}
```

在这段代码中，通过快速检查选择器的标记名称和属性来优化选择器匹配。`m_tag`是当前元素的标签名称，`m_classes`是当前元素的类名列表。

##### 3. 选择器匹配

```c++
int apply = select(*sel, false);
```

调用 `select` 方法来检查选择器是否与当前元素匹配。`select` 方法返回一个整数，表示匹配的结果。

##### 5. 如果选择器匹配，应用样式

```c++
if(apply != select_no_match) {
    used_selector::ptr us = std::unique_ptr<used_selector>(new used_selector(sel, false));
```

如果选择器匹配，则创建一个 `used_selector` 对象，用于存储已使用的选择器。

##### 6. 检查媒体查询

```c++
if(sel->is_media_valid()) {
```

检查选择器的媒体查询是否有效。如果无效，则不应用该选择器的样式。

##### 7. 处理伪元素和伪类

```c++
auto apply_before_after = [&]() {
    const auto& content_property = sel->m_style->get_property(_content_);
    bool content_none = content_property.is<string>() && content_property.get<string>() == "none";
    bool create = !content_none && (sel->m_right.m_attrs.size() > 1 || sel->m_right.m_tag != star_id);

    element::ptr el;
    if(apply & select_match_with_after) {
        el = get_element_after(*sel->m_style, create);
    } else if(apply & select_match_with_before){
        el = get_element_before(*sel->m_style, create);
    } else{
        return;
    }

    if(el){
        if(!content_none){
            el->add_style(*sel->m_style);
        } else{
            el->parent()->removeChild(el);
        }
    } else{
        if(!content_none){
            add_style(*sel->m_style);
        }
    }
    us->m_used = true;
};
```

处理伪元素（如 `::before` 和 `::after`）和伪类（如 `:hover` 和 `:active`）。如果选择器匹配伪元素或伪类，则创建或更新伪元素，并应用相应的样式。

##### 8. 应用样式

```c++
if(apply & select_match_pseudo_class){
    if(select(*sel, true)){
        if((apply & (select_match_with_after | select_match_with_before))){
            apply_before_after();
        } else{
            add_style(*sel->m_style);
            us->m_used = true;
        }
    }
} else if((apply & (select_match_with_after | select_match_with_before))){
    apply_before_after();
} else{
    add_style(*sel->m_style);
    us->m_used = true;
}
```

根据选择器的匹配结果，应用样式到当前元素。如果选择器匹配伪类，则进一步检查并应用伪类样式。否则，直接应用样式。

##### 9. 存储已使用的选择器

```c++
m_used_styles.push_back(std::move(us));
```

将已使用的选择器存储到 `m_used_styles` 列表中。

##### 10. 递归应用样式表到子元素

```c++
for(auto& el : m_children) {
    if(el->css().get_display() != display_inline_text) {
        el->apply_stylesheet(stylesheet);
    }
}
```

递归地将样式表应用到当前元素的子元素上。这确保了整个元素树都能正确应用样式。



#### 第四步：初始化 m_css

```c++
doc->m_root->compute_styles();
```

##### 计算样式

`compute_styles` 方法的主要功能是根据 HTML 元素的属性和样式表来计算元素的最终样式属性，并将这些属性应用到元素及其子元素上。以下是该方法的详细解析：

###### 1. 处理元素的 `style` 属性

```c++
const char* style = get_attr("style");
document::ptr doc = get_document();

if (style) {
    m_style.add(style, "", doc->container());
}

m_style.subst_vars(this);
```

- 获取元素的 `style` 属性。
- 如果存在 `style` 属性，将其添加到元素的样式集合中。
- 使用 `subst_vars` 方法处理样式中的变量。

###### 2. 计算元素的 CSS 属性

```c++
m_css.compute(this, doc);
```

- 调用 `m_css.compute` 方法计算 CSS 属性。这一步将解析所有与该元素相关的 CSS 属性，并将其应用到元素上。

###### 3. 递归处理子元素

```c++
if (recursive) {
    for (const auto& el : m_children) {
        el->compute_styles();
    }
}
```

- 如果 `recursive` 参数为 `true`，递归地对所有子元素调用 `compute_styles` 方法。

##### 解析 `litehtml::css_properties::compute` 方法

`compute` 方法负责具体计算并设置各个 CSS 属性。以下是该方法的详细解析：

###### 1. 计算字体属性

```c++
compute_font(el, doc);
int font_size = get_font_size();
```

- 调用 `compute_font` 方法计算字体相关的属性。
- 获取字体大小。

###### 2. 计算基本 CSS 属性

```c++
m_color = el->get_property<web_color>(_color_, true, web_color::black, offset(m_color));
m_el_position = (element_position) el->get_property<int>(_position_, false, element_position_static, offset(m_el_position));
// 其他类似属性的计算...
```

- 通过 `get_property` 方法获取各个 CSS 属性的值。如果属性在元素中未定义，则使用默认值。
- 属性包括颜色、位置、显示方式、浮动、清除、盒子模型等。

###### 3. 特殊处理 `display` 属性

```c++
if (m_display == display_none) {
    m_float = float_none;
} else if (m_el_position == element_position_absolute || m_el_position == element_position_fixed) {
    m_float = float_none;
    // 其他特殊处理...
} else if (m_float != float_none) {
    // 处理浮动属性
} else if (el->is_root()) {
    // 处理根元素
}
```

- 根据 `display` 属性的值对其他属性进行特殊处理。
- 例如，如果 `display` 为 `none`，则不应用 `float` 属性。

###### 4. 计算宽度、高度、边距和填充等属性

```c++
m_css_width = el->get_property<css_length>(_width_, false, _auto, offset(m_css_width));
// 其他类似属性的计算...

doc->cvt_units(m_css_width, font_size);
// 其他属性的单位转换...
```

- 计算元素的宽度、高度、最小宽度、最大宽度、边距和填充等属性。
- 调用 `cvt_units` 方法将这些属性的单位转换为像素值。

###### 5. 计算边框属性

```c++
m_css_borders.left.color = el->get_property<web_color>(_border_left_color_, false, m_color, offset(m_css_borders.left.color));
// 其他类似属性的计算...

doc->cvt_units(m_css_borders.left.width, font_size);
// 其他属性的单位转换...
```

- 计算元素的边框颜色、样式和宽度。
- 根据边框样式调整宽度（例如，如果边框样式为 `none` 或 `hidden`，则宽度为 0）。

###### 6. 计算边框半径

```c++
m_css_borders.radius.top_left_x = el->get_property<css_length>(_border_top_left_radius_x_, false, 0, offset(m_css_borders.radius.top_left_x));
// 其他类似属性的计算...

doc->cvt_units(m_css_borders.radius.top_left_x, font_size);
// 其他属性的单位转换...
```

- 计算边框半径属性，包括左上、右上、左下和右下角的半径。
- 调用 `cvt_units` 方法将这些属性的单位转换为像素值。

###### 7. 计算其他 CSS 属性

```c++
m_border_collapse = (border_collapse) el->get_property<int>(_border_collapse_, true, border_collapse_separate, offset(m_border_collapse));
m_css_border_spacing_x = el->get_property<css_length>(__litehtml_border_spacing_x_, true, 0, offset(m_css_border_spacing_x));
// 其他类似属性的计算...

doc->cvt_units(m_css_border_spacing_x, font_size);
// 其他属性的单位转换...
```

- 计算其他属性，如边框折叠、边框间距、偏移量等。

通过以上步骤，`compute_styles` 和 `css_properties::compute` 方法共同完成了元素样式的计算和应用，为后续的渲染和布局打下基础。



#### 第五步：创建渲染树

#####  创建渲染树

```c++
doc->m_root_render = doc->m_root->create_render_item(nullptr);
```

##### 创建渲染项

`create_render_item` 方法用于创建渲染树（rendering tree）。渲染树是 HTML 元素树的一个投影，用于布局和绘制。

- **渲染项**：每个 HTML 元素对应一个渲染项（render item）。渲染项包含布局和绘制所需的信息。
- **递归创建**：从根元素开始递归创建每个子元素的渲染项。

`create_render_item` 实现为：

```c++
std::shared_ptr<render_item> element::create_render_item(const std::shared_ptr<render_item>& parent_ri) {
	std::shared_ptr<render_item> ret;

	if(css().get_display() == display_table_column ||
	   css().get_display() == display_table_column_group ||
	   css().get_display() == display_table_footer_group ||
	   css().get_display() == display_table_header_group ||
	   css().get_display() == display_table_row_group) {
		ret = std::make_shared<render_item_table_part>(shared_from_this());
	} else if(css().get_display() == display_table_row) {
		ret = std::make_shared<render_item_table_row>(shared_from_this());
	} else if(css().get_display() == display_block ||
				css().get_display() == display_table_cell ||
				css().get_display() == display_table_caption ||
				css().get_display() == display_list_item ||
				css().get_display() == display_inline_block) {
		ret = std::make_shared<render_item_block>(shared_from_this());
	} else if(css().get_display() == display_table || css().get_display() == display_inline_table) {
		ret = std::make_shared<render_item_table>(shared_from_this());
	} else if(css().get_display() == display_inline || css().get_display() == display_inline_text) {
		ret = std::make_shared<render_item_inline>(shared_from_this());
	} else if(css().get_display() == display_flex || css().get_display() == display_inline_flex) {
		ret = std::make_shared<render_item_flex>(shared_from_this());
	}
	if(ret) {
		if (css().get_display() == display_table ||
			css().get_display() == display_inline_table ||
			css().get_display() == display_table_caption ||
			css().get_display() == display_table_cell ||
			css().get_display() == display_table_column ||
			css().get_display() == display_table_column_group ||
			css().get_display() == display_table_footer_group ||
			css().get_display() == display_table_header_group ||
			css().get_display() == display_table_row ||
			css().get_display() == display_table_row_group) {
			get_document()->add_tabular(ret);
		}

		ret->parent(parent_ri);
		for(const auto& el : m_children) {
			auto ri = el->create_render_item(ret);
			if(ri) {
				ret->add_child(ri);
			}
		}
	}
	return ret;
}
```

以下是该函数的详细解析：

###### 1. 根据 `display` 属性创建对应的渲染项

```c++
std::shared_ptr<render_item> ret;

if (css().get_display() == display_table_column ||
    css().get_display() == display_table_column_group ||
    css().get_display() == display_table_footer_group ||
    css().get_display() == display_table_header_group ||
    css().get_display() == display_table_row_group) {
    ret = std::make_shared<render_item_table_part>(shared_from_this());
} else if (css().get_display() == display_table_row) {
    ret = std::make_shared<render_item_table_row>(shared_from_this());
} else if (css().get_display() == display_block ||
           css().get_display() == display_table_cell ||
           css().get_display() == display_table_caption ||
           css().get_display() == display_list_item ||
           css().get_display() == display_inline_block) {
    ret = std::make_shared<render_item_block>(shared_from_this());
} else if (css().get_display() == display_table || css().get_display() == display_inline_table) {
    ret = std::make_shared<render_item_table>(shared_from_this());
} else if (css().get_display() == display_inline || css().get_display() == display_inline_text) {
    ret = std::make_shared<render_item_inline>(shared_from_this());
} else if (css().get_display() == display_flex || css().get_display() == display_inline_flex) {
    ret = std::make_shared<render_item_flex>(shared_from_this());
}
```

- 根据元素的 `display` 属性创建对应的渲染项实例，并将当前元素的 `shared_ptr` 传递给渲染项的构造函数。

###### 2. 特殊处理表格相关元素

```c++
if (ret) {
    if (css().get_display() == display_table ||
        css().get_display() == display_inline_table ||
        css().get_display() == display_table_caption ||
        css().get_display() == display_table_cell ||
        css().get_display() == display_table_column ||
        css().get_display() == display_table_column_group ||
        css().get_display() == display_table_footer_group ||
        css().get_display() == display_table_header_group ||
        css().get_display() == display_table_row ||
        css().get_display() == display_table_row_group) {
        get_document()->add_tabular(ret);
    }
```

- 如果渲染项不为空，并且元素是表格相关元素，将渲染项添加到文档的表格元素列表中。

###### 3. 设置渲染项的父节点并递归创建子节点的渲染项

```c++
	ret->parent(parent_ri);
    for (const auto& el : m_children) {
        auto ri = el->create_render_item(ret);
        if (ri) {
            ret->add_child(ri);
        }
    }
}
return ret;
```

- 设置渲染项的父节点。
- 递归地为所有子元素创建渲染项，并将子渲染项添加到当前渲染项的子节点列表中。

##### 详细解析

###### 1. 创建渲染项

根据元素的 `display` 属性创建对应类型的渲染项：

- `display_table_column`, `display_table_column_group`, `display_table_footer_group`, `display_table_header_group`, `display_table_row_group` 对应 `render_item_table_part`。
- `display_table_row` 对应 `render_item_table_row`。
- `display_block`, `display_table_cell`, `display_table_caption`, `display_list_item`, `display_inline_block` 对应 `render_item_block`。
- `display_table`, `display_inline_table` 对应 `render_item_table`。
- `display_inline`, `display_inline_text` 对应 `render_item_inline`。
- `display_flex`, `display_inline_flex` 对应 `render_item_flex`。

###### 2. 添加表格元素到文档的表格元素列表

如果元素是表格相关元素（例如 `table`, `table-row`, `table-cell` 等），将其渲染项添加到文档的表格元素列表中。这是为了在后续布局过程中对表格元素进行特殊处理。

###### 3. 设置父节点并递归创建子节点

- 将当前渲染项与父渲染项关联。
- 递归地为所有子元素创建渲染项，并将子渲染项添加到当前渲染项的子节点列表中。



### 第六步. 修复表格布局

```c++
doc->fix_tables_layout();
```

#### 表格布局修复

`fix_tables_layout` 方法用于修复表格布局，确保表格元素符合 CSS 规范。具体步骤包括：

- **检查缺失的表格元素**：确保表格中包含 `table`、`tr`、`td` 等必需的元素。如果缺少这些元素，可能会创建匿名盒子（anonymous boxes）来补充。
- **创建匿名盒子**：根据 CSS 规范，如果表格布局中缺少必要的元素，需要创建匿名盒子来补充这些元素。



### 第七步. 初始化元素

```c++
doc->m_root_render = doc->m_root_render->init();
```

#### 初始化渲染项

`init` 方法用于初始化渲染项。具体步骤包括：

- **确定元素类型**：根据 CSS 规则和文档结构，确定每个渲染项的类型。例如，某些元素可能会变成块级元素或内联元素。
- **设置布局属性**：为每个渲染项设置布局属性，如宽度、高度、边距、填充等。
- **递归初始化**：递归初始化每个子渲染项，确保整个渲染树正确初始化。

`init` 实现如下：

```c++
std::shared_ptr<litehtml::render_item> litehtml::render_item_block::init(){
    {
        css_selector sel;
        sel.parse(".inline_rating");
        if(src_el()->select(sel)) {
            int i = 0;
            i++;
        }
    }
    std::shared_ptr<render_item> ret;

    // Initialize indexes for list items
    if(src_el()->css().get_display() == display_list_item && src_el()->css().get_list_style_type() >= list_style_type_armenian) {
        if (auto p = src_el()->parent()) {
            int val = atoi(p->get_attr("start", "1"));
			for(const auto &child : p->children()) {
                if (child == src_el()) {
                    src_el()->set_attr("list_index", std::to_string(val).c_str());
                    break;
                }
                else if (child->css().get_display() == display_list_item)
                    val++;
            }
        }
    }
    // Split inline blocks with box blocks inside
    auto iter = m_children.begin();
    while (iter != m_children.end()) {
        const auto& el = *iter;
        if(el->src_el()->css().get_display() == display_inline && !el->children().empty()) {
            auto split_el = el->split_inlines();
            if(std::get<0>(split_el)) {
                iter = m_children.erase(iter);
                iter = m_children.insert(iter, std::get<2>(split_el));
                iter = m_children.insert(iter, std::get<1>(split_el));
                iter = m_children.insert(iter, std::get<0>(split_el));

                std::get<0>(split_el)->parent(shared_from_this());
                std::get<1>(split_el)->parent(shared_from_this());
                std::get<2>(split_el)->parent(shared_from_this());
                continue;
            }
        }
        ++iter;
    }

    bool has_block_level = false;
	bool has_inlines = false;
    for (const auto& el : m_children) {
		if(!el->src_el()->is_float()) {
			if (el->src_el()->is_block_box()) {
				has_block_level = true;
			} else if (el->src_el()->is_inline()) {
				has_inlines = true;
			}
		}
        if(has_block_level && has_inlines)
            break;
    }
    if(has_block_level) {
        ret = std::make_shared<render_item_block_context>(src_el());
        ret->parent(parent());

        auto doc = src_el()->get_document();
        decltype(m_children) new_children;
        decltype(m_children) inlines;
        bool not_ws_added = false;
        for (const auto& el : m_children) {
            if(el->src_el()->is_inline()) {
                inlines.push_back(el);
                if(!el->src_el()->is_white_space())
                    not_ws_added = true;
            } else {
                if(not_ws_added) {
                    auto anon_el = std::make_shared<html_tag>(src_el());
                    auto anon_ri = std::make_shared<render_item_block>(anon_el);
                    for(const auto& inl : inlines) {
                        anon_ri->add_child(inl);
                    }

                    not_ws_added = false;
                    new_children.push_back(anon_ri);
                    anon_ri->parent(ret);
                }
                new_children.push_back(el);
                el->parent(ret);
                inlines.clear();
            }
        }
        if(!inlines.empty() && not_ws_added) {
            auto anon_el = std::make_shared<html_tag>(src_el());
            auto anon_ri = std::make_shared<render_item_block>(anon_el);
            for(const auto& inl : inlines) {
                anon_ri->add_child(inl);
            }

            new_children.push_back(anon_ri);
            anon_ri->parent(ret);
        }
        ret->children() = new_children;
    }

    if(!ret) {
        ret = std::make_shared<render_item_inline_context>(src_el());
        ret->parent(parent());
        ret->children() = children();
        for (const auto &el: ret->children()) {
            el->parent(ret);
        }
    }

    ret->src_el()->add_render(ret);

    for(auto& el : ret->children()) {
        el = el->init();
    }

    return ret;
}
```

`render_item_block::init` 函数的作用是初始化一个块级渲染项，并根据其子元素的显示特性调整渲染项的结构。以下是该函数的详细解析：

##### 1. 初始化列表项的索引

```c++
// Initialize indexes for list items
if (src_el()->css().get_display() == display_list_item && src_el()->css().get_list_style_type() >= list_style_type_armenian)
{
    if (auto p = src_el()->parent())
    {
        int val = atoi(p->get_attr("start", "1")); 
        for (const auto &child : p->children())
        {
            if (child == src_el())
            {
                src_el()->set_attr("list_index", std::to_string(val).c_str());
                break;
            }
            else if (child->css().get_display() == display_list_item)
                val++;
        }
    }
}
```

- 如果当前元素是列表项，并且其列表样式类型是 Armenian 或以上，则初始化其索引。
- 获取父元素并遍历父元素的子元素，将索引赋值给当前元素。

##### 2. 拆分内联块内包含块级元素的情况

```c++
// Split inline blocks with box blocks inside
auto iter = m_children.begin();
while (iter != m_children.end())
{
    const auto &el = *iter;
    if (el->src_el()->css().get_display() == display_inline && !el->children().empty())
    {
        auto split_el = el->split_inlines();
        if (std::get<0>(split_el))
        {
            iter = m_children.erase(iter);
            iter = m_children.insert(iter, std::get<2>(split_el));
            iter = m_children.insert(iter, std::get<1>(split_el));
            iter = m_children.insert(iter, std::get<0>(split_el));

            std::get<0>(split_el)->parent(shared_from_this());
            std::get<1>(split_el)->parent(shared_from_this());
            std::get<2>(split_el)->parent(shared_from_this());
            continue;
        }
    }
    ++iter;
}
```

- 遍历子元素，如果子元素是内联显示并且包含子元素，则拆分为多个渲染项。
- 更新子元素列表，插入拆分后的渲染项，并设置这些新渲染项的父元素。



































