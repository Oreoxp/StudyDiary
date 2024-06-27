[TOC]

# 重要成员

## render_item

```c++
  class render_item : public std::enable_shared_from_this<render_item> {
  protected:
      //指向与此渲染项关联的元素
      std::shared_ptr<element>                    m_element;
      //指向此渲染项的父项
      std::weak_ptr<render_item>                  m_parent;
      //包含此渲染项的子项列表
      std::list<std::shared_ptr<render_item>>     m_children;
      //此渲染项的边距
      margins						                m_margins;
      //此渲染项的填充
      margins						                m_padding;
      //此渲染项的边框
      margins						                m_borders;
      //此渲染项的位置
      position					                m_pos;
      //标记此渲染项是否应跳过渲染
      bool                                        m_skip;
      //包含定位的渲染项
      std::vector<std::shared_ptr<render_item>>   m_positioned;
      
	//计算包含块上下文
	containing_block_context calculate_containing_block_context(const containing_block_context& cb_context);
      
    //计算包含块长度
	void calc_cb_length(const css_length& len, int percent_base, containing_block_context::typed_int& out_value) const;
      
	virtual int _render(int /*x*/, int /*y*/, const containing_block_context& /*containing_block_size*/, formatting_context* /*fmt_ctx*/, bool /*second_pass = false*/) {
		return 0;
	}

  public:
      explicit render_item(std::shared_ptr<element>  src_el);

      virtual ~render_item() = default;

      std::list<std::shared_ptr<render_item>>& children() {
          return m_children;
      }

      position& pos() {
          return m_pos;
      }

      bool skip() const {
          return m_skip;
      }

      void skip(bool val) {
          m_skip = val;
      }

      int right() const {
          return left() + width();
      }

      int left() const {
          return m_pos.left() - m_margins.left - m_padding.left - m_borders.left;
      }

      int top() const {
          return m_pos.top() - m_margins.top - m_padding.top - m_borders.top;
      }

      int bottom() const {
          return top() + height();
      }

      int height() const {
          return m_pos.height + m_margins.height() + m_padding.height() + m_borders.height();
      }

      int width() const {
          return m_pos.width + m_margins.width() + m_padding.width() + m_borders.width();
      }

      int padding_top() const {
          return m_padding.top;
      }

      int padding_bottom() const {
          return m_padding.bottom;
      }

      int padding_left() const {
          return m_padding.left;
      }

      int padding_right() const {
          return m_padding.right;
      }

      int border_top() const {
          return m_borders.top;
      }

      int border_bottom() const {
          return m_borders.bottom;
      }

      int border_left() const {
          return m_borders.left;
      }

      int border_right() const {
          return m_borders.right;
      }

      int margin_top() const {
          return m_margins.top;
      }

      int margin_bottom() const {
          return m_margins.bottom;
      }

      int margin_left() const {
          return m_margins.left;
      }

      int margin_right() const {
          return m_margins.right;
      }

      std::shared_ptr<render_item> parent() const {
          return m_parent.lock();
      }

      margins& get_margins() {
          return m_margins;
      }

      margins& get_paddings() {
          return m_padding;
      }

      void set_paddings(const margins& val) {
          m_padding = val;
      }

      margins& get_borders() {
          return m_borders;
      }

	/**
	 * Top offset to the element content. Includes paddings, margins and borders.
	 */
      int content_offset_top() const {
          return m_margins.top + m_padding.top + m_borders.top;
      }

	/**
	 * Bottom offset to the element content. Includes paddings, margins and borders.
	 */
      inline int content_offset_bottom() const {
          return m_margins.bottom + m_padding.bottom + m_borders.bottom;
      }

	/**
	 * Left offset to the element content. Includes paddings, margins and borders.
	 */
      int content_offset_left() const {
          return m_margins.left + m_padding.left + m_borders.left;
      }

	/**
	 * Right offset to the element content. Includes paddings, margins and borders.
	 */
      int content_offset_right() const {
          return m_margins.right + m_padding.right + m_borders.right;
      }

	/**
	 * Sum of left and right offsets to the element content. Includes paddings, margins and borders.
	 */
      int content_offset_width() const {
          return content_offset_left() + content_offset_right();
      }

	/**
	 * Sum of top and bottom offsets to the element content. Includes paddings, margins and borders.
	 */
      int content_offset_height() const {
          return content_offset_top() + content_offset_bottom();
      }

	int render_offset_left() const {
		if(css().get_box_sizing() == box_sizing_content_box) {
			return m_margins.left + m_borders.left + m_padding.left;
		}
		return m_margins.left;
	}

	int render_offset_right() const {
		if(css().get_box_sizing() == box_sizing_content_box) {
			return m_margins.right + m_borders.right + m_padding.right;
		}
		return m_margins.right;
	}

	int render_offset_width() const {
		return render_offset_left() + render_offset_right();
	}

	int render_offset_top() const {
		if(css().get_box_sizing() == box_sizing_content_box) {
			return m_margins.top + m_borders.top + m_padding.top;
		}
		return m_margins.top;
	}

	int render_offset_bottom() const {
		if(css().get_box_sizing() == box_sizing_content_box) {
			return m_margins.bottom + m_borders.bottom + m_padding.bottom;
		}
		return m_margins.bottom;
	}

	int render_offset_height() const {
		return render_offset_top() + render_offset_bottom();
	}

	int box_sizing_left() const {
		if(css().get_box_sizing() == box_sizing_border_box) {
			return m_padding.left + m_borders.left;
		}
		return 0;
	}

	int box_sizing_right() const {
		if(css().get_box_sizing() == box_sizing_border_box) {
			return m_padding.right + m_borders.right;
		}
		return 0;
	}

	int box_sizing_width() const {
		return box_sizing_left() + box_sizing_right();
	}

	int box_sizing_top() const {
		if(css().get_box_sizing() == box_sizing_border_box) {
			return m_padding.top + m_borders.top;
		}
		return 0;
	}

	int box_sizing_bottom() const {
		if(css().get_box_sizing() == box_sizing_border_box) {
			return m_padding.bottom + m_borders.bottom;
		}
		return 0;
	}

	int box_sizing_height() const {
		return box_sizing_top() + box_sizing_bottom();
	}

      void parent(const std::shared_ptr<render_item>& par) {
          m_parent = par;
      }

      const std::shared_ptr<element>& src_el() const {
          return m_element;
      }

	const css_properties& css() const {
		return m_element->css();
	}

      void add_child(const std::shared_ptr<render_item>& ri) {
          m_children.push_back(ri);
          ri->parent(shared_from_this());
      }

	bool is_root() const {
		return m_parent.expired();
	}

      bool collapse_top_margin() const {
          return !m_borders.top &&
                 !m_padding.top &&
                 m_element->in_normal_flow() &&
                 m_element->css().get_float() == float_none &&
                 m_margins.top >= 0 &&
			   !is_flex_item() &&
                 !is_root();
      }

      bool collapse_bottom_margin() const {
          return !m_borders.bottom &&
                 !m_padding.bottom &&
                 m_element->in_normal_flow() &&
                 m_element->css().get_float() == float_none &&
                 m_margins.bottom >= 0 &&
                 !is_root();
      }

      bool is_visible() const {
          return !(m_skip || src_el()->css().get_display() == display_none || src_el()->css().get_visibility() != visibility_visible);
      }

	bool is_flex_item() const {
		auto par = parent();
		if(par && (par->css().get_display() == display_inline_flex || par->css().get_display() == display_flex)) {
			return true;
		}
		return false;
	}
      
	//渲染此项及其子项。
	//计算大纲，移动位置并调用 _render 方法。
	int render(int x, int y, const containing_block_context& containing_block_size, formatting_context* fmt_ctx, bool second_pass = false);
    //应用相对偏移量，根据 left, right, top 和 bottom 属性调整位置。
	void apply_relative_shift(const containing_block_context &containing_block_size);
    //计算此项的边距、填充和边框的百分比值。
	void calc_outlines( int parent_width );
    //计算自动边距。如果左右边距都是自动的，则将它们设置为相等的值以使项居中。
	int calc_auto_margins(int parent_width);	// returns left margin

      //初始化此项及其子项
      virtual std::shared_ptr<render_item> init();
      virtual void apply_vertical_align() {}
	/**
	 * Get first baseline position. Default position is element bottom without bottom margin.
	 * @returns offset of the first baseline from element top
	 */
	virtual int get_first_baseline() { return height() - margin_bottom(); }
	/**
	 * Get last baseline position.  Default position is element bottom without bottom margin.
	 * @returns offset of the last baseline from element top
	 */
	virtual int get_last_baseline() { return height() - margin_bottom(); }

      virtual std::shared_ptr<render_item> clone() {
          return std::make_shared<render_item>(src_el());
      }
      //将内联项拆分为三个部分：左部分，中间项（块级项）和右部分。
      std::tuple<
              std::shared_ptr<litehtml::render_item>,
              std::shared_ptr<litehtml::render_item>,
              std::shared_ptr<litehtml::render_item>
              > split_inlines();
      //获取所有定位的子项，并将它们存储在 m_positioned 中。
      bool fetch_positioned();
      //渲染定位的子项
      void render_positioned(render_type rt = render_all);
      //添加一个定位的子项
      void add_positioned(const std::shared_ptr<litehtml::render_item> &el);
      //获取此项及其子项需要重绘的区域
      void get_redraw_box(litehtml::position& pos, int x = 0, int y = 0);
      //计算文档的大小
      void calc_document_size( litehtml::size& sz, litehtml::size& content_size, int x = 0, int y = 0 );
	virtual void get_inline_boxes( position::vector& /*boxes*/ ) const {};
	virtual void set_inline_boxes( position::vector& /*boxes*/ ) {};
	virtual void add_inline_box( const position& /*box*/ ) {};
	virtual void clear_inline_boxes() {};
      //绘制堆叠上下文中的子项
      void draw_stacking_context( uint_ptr hdc, int x, int y, const position* clip, bool with_positioned );
      //根据 draw_flag 和 zindex 绘制子项
      virtual void draw_children( uint_ptr hdc, int x, int y, const position* clip, draw_flag flag, int zindex );
      virtual int get_draw_vertical_offset() { return 0; }
      //获取指定点处的子项
      virtual std::shared_ptr<element> get_child_by_point(int x, int y, int client_x, int client_y, draw_flag flag, int zindex);
      //获取指定点处的元素
      std::shared_ptr<element> get_element_by_point(int x, int y, int client_x, int client_y);
      //检查指定点是否在此项内
      bool is_point_inside( int x, int y );
      //转储此项的调试信息
      void dump(litehtml::dumper& cout);
      //获取此项的放置位置
      position get_placement() const;
      /**
       * Returns the boxes of rendering element. All coordinates are absolute
       *
       * @param redraw_boxes [out] resulting rendering boxes
       * @return
       */
      //获取此项的渲染盒
      void get_rendering_boxes( position::vector& redraw_boxes);
};
```

### 主要功能解析

- **渲染功能**：`render` 方法负责渲染此项及其子项。它首先计算大纲并调整位置，然后调用 `_render` 方法进行实际渲染。
- **计算功能**：`calc_outlines`, `calc_auto_margins` 和 `apply_relative_shift` 方法负责计算此项的边距、填充、边框和相对偏移量。
- **布局功能**：`split_inlines`, `fetch_positioned` 和 `render_positioned` 方法处理内联项的拆分、定位子项的获取和渲染。
- **绘制功能**：`draw_stacking_context` 和 `draw_children` 方法负责绘制此项及其子项。
- **交互功能**：`get_child_by_point` 和 `get_element_by_point` 方法用于在指定点获取子项和元素。
- **调试功能**：`dump` 方法用于转储此项的调试信息。
- **初始化功能**：`init` 方法用于初始化此项及其子项。







































# Render

















