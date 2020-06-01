duilib的所有控件均绘制在唯一的真实窗口之中，本篇就具体看下这个绘制的过程。所有的绘制过程均在WM_PAINT消息处理过程中完成。

​		由窗口及消息篇可以看到，窗口消息处理最终流到了CPaintManagerUI::MessageHandler中。包括WM_PAINT在内消息均在此函数中处理，这里我们仅关注WM_PAINT消息

```C++
case WM_PAINT:
        {
            // Should we paint?
            RECT rcPaint = { 0 };
            if( !::GetUpdateRect(m_hWndPaint, &rcPaint, FALSE) ) return true;
            if( m_pRoot == NULL ) {
                PAINTSTRUCT ps = { 0 };
                ::BeginPaint(m_hWndPaint, &ps);
                ::EndPaint(m_hWndPaint, &ps);
                return true;
            }            
            //我们是否需要调整大小？
            //这是我们在窗体上布置控件的时间。
            //我们甚至可以从WM_SIZE消息中延迟此操作，因为可以调整大小
            //非常庞大的操作。
            if( m_bUpdateNeeded ) {
                m_bUpdateNeeded = false;
                RECT rcClient = { 0 };
                ::GetClientRect(m_hWndPaint, &rcClient);
                if( !::IsRectEmpty(&rcClient) ) {
                    if( m_pRoot->IsUpdateNeeded() ) {
                        m_pRoot->SetPos(rcClient);
                        if( m_hDcOffscreen != NULL ) ::DeleteDC(m_hDcOffscreen);
                        if( m_hDcBackground != NULL ) ::DeleteDC(m_hDcBackground);
                        if( m_hbmpOffscreen != NULL ) ::DeleteObject(m_hbmpOffscreen);
                        if( m_hbmpBackground != NULL ) ::DeleteObject(m_hbmpBackground);
                        m_hDcOffscreen = NULL;
                        m_hDcBackground = NULL;
                        m_hbmpOffscreen = NULL;
                        m_hbmpBackground = NULL;
                    }
                    else {
                        CControlUI* pControl = NULL;
                        while( pControl = m_pRoot->FindControl(__FindControlFromUpdate, NULL, UIFIND_VISIBLE | UIFIND_ME_FIRST) ) {
                            pControl->SetPos( pControl->GetPos() );
                        }
                    }
                    //我们希望在第一次初始化窗口时通知它
                    //具有正确的布局。 窗口表单会花费时间
                    //提交滑动/动画。
                    if( m_bFirstLayout ) {
                        m_bFirstLayout = false;
                        SendNotify(m_pRoot, _T("windowinit"),  0, 0, false);
                    }
                }
            }
           //将焦点设置为第一个控件？
            if( m_bFocusNeeded ) {
                SetNextTabControl();
            }
            //
             //渲染屏幕
            //
             //准备屏幕外的位图？
            if( m_bOffscreenPaint && m_hbmpOffscreen == NULL )
            {
                RECT rcClient = { 0 };
                ::GetClientRect(m_hWndPaint, &rcClient);
                m_hDcOffscreen = ::CreateCompatibleDC(m_hDcPaint);
                m_hbmpOffscreen = ::CreateCompatibleBitmap(m_hDcPaint, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top); 
                ASSERT(m_hDcOffscreen);
                ASSERT(m_hbmpOffscreen);
            }
            // Begin Windows paint
            PAINTSTRUCT ps = { 0 };
            ::BeginPaint(m_hWndPaint, &ps);
            if( m_bOffscreenPaint )
            {
                HBITMAP hOldBitmap = (HBITMAP) ::SelectObject(m_hDcOffscreen, m_hbmpOffscreen);
                int iSaveDC = ::SaveDC(m_hDcOffscreen);
                if( m_bAlphaBackground ) {
                    if( m_hbmpBackground == NULL ) {
                        RECT rcClient = { 0 };
                        ::GetClientRect(m_hWndPaint, &rcClient);
                        m_hDcBackground = ::CreateCompatibleDC(m_hDcPaint);;
                        m_hbmpBackground = ::CreateCompatibleBitmap(m_hDcPaint, rcClient.right - rcClient.left, rcClient.bottom - rcClient.top); 
                        ASSERT(m_hDcBackground);
                        ASSERT(m_hbmpBackground);
                        ::SelectObject(m_hDcBackground, m_hbmpBackground);
                        ::BitBlt(m_hDcBackground, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left,
                            ps.rcPaint.bottom - ps.rcPaint.top, ps.hdc, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
                    }
                    else
                        ::SelectObject(m_hDcBackground, m_hbmpBackground);
                    ::BitBlt(m_hDcOffscreen, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left,
                        ps.rcPaint.bottom - ps.rcPaint.top, m_hDcBackground, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
                }
                m_pRoot->DoPaint(m_hDcOffscreen, ps.rcPaint);//绘制控件
                for( int i = 0; i < m_aPostPaintControls.GetSize(); i++ ) {
                    CControlUI* pPostPaintControl = static_cast<CControlUI*>(m_aPostPaintControls[i]);
                    pPostPaintControl->DoPostPaint(m_hDcOffscreen, ps.rcPaint);
                }
                ::RestoreDC(m_hDcOffscreen, iSaveDC);
                ::BitBlt(ps.hdc, ps.rcPaint.left, ps.rcPaint.top, ps.rcPaint.right - ps.rcPaint.left,
                    ps.rcPaint.bottom - ps.rcPaint.top, m_hDcOffscreen, ps.rcPaint.left, ps.rcPaint.top, SRCCOPY);
                ::SelectObject(m_hDcOffscreen, hOldBitmap);

                if( m_bShowUpdateRect ) {
                    HPEN hOldPen = (HPEN)::SelectObject(ps.hdc, m_hUpdateRectPen);
                    ::SelectObject(ps.hdc, ::GetStockObject(HOLLOW_BRUSH));
                    ::Rectangle(ps.hdc, rcPaint.left, rcPaint.top, rcPaint.right, rcPaint.bottom);
                    ::SelectObject(ps.hdc, hOldPen);
                }
            }
            else
            {
                // A standard paint job
                int iSaveDC = ::SaveDC(ps.hdc);
                m_pRoot->DoPaint(ps.hdc, ps.rcPaint);//绘制控件
                ::RestoreDC(ps.hdc, iSaveDC);
            }
            // All Done!
            ::EndPaint(m_hWndPaint, &ps);
        }
        //如果任何绘画要求再次调整大小，我们将需要
        //使整个窗口再次无效。
        if( m_bUpdateNeeded ) {
            ::InvalidateRect(m_hWndPaint, NULL, FALSE);
        }
        return true;
```

在::BeginPaint(m_hWndPaint, &ps)和::EndPaint(m_hWndPaint, &ps)中间是窗口绘制部分，duilib包含了两种方式：双缓存方式（解决闪烁问题）和标准方式，默认为双缓存方式。两种方式最终都调用了m_pRoot->DoPaint，m_pRoot为控件容器，且DoPaint为虚函数，实际调用了CContainerUI::DoPaint

```c++
 void CContainerUI::DoPaint(HDC hDC, const RECT& rcPaint)
{
    RECT rcTemp = { 0 };
    if( !::IntersectRect(&rcTemp, &rcPaint, &m_rcItem) ) return;

    CRenderClip clip;
    CRenderClip::GenerateClip(hDC, rcTemp, clip);
    CControlUI::DoPaint(hDC, rcPaint);

    if( m_items.GetSize() > 0 ) {
        RECT rc = m_rcItem;
        rc.left += m_rcInset.left;
        rc.top += m_rcInset.top;
        rc.right -= m_rcInset.right;
        rc.bottom -= m_rcInset.bottom;
        if( m_pVerticalScrollBar && m_pVerticalScrollBar->IsVisible() ) rc.right -= m_pVerticalScrollBar->GetFixedWidth();
        if( m_pHorizontalScrollBar && m_pHorizontalScrollBar->IsVisible() ) rc.bottom -= m_pHorizontalScrollBar->GetFixedHeight();

        if( !::IntersectRect(&rcTemp, &rcPaint, &rc) ) {
            for( int it = 0; it < m_items.GetSize(); it++ ) {
                CControlUI* pControl = static_cast<CControlUI*>(m_items[it]);
                if( !pControl->IsVisible() ) continue;
                if( !::IntersectRect(&rcTemp, &rcPaint, &pControl->GetPos()) ) continue;
                if( pControl ->IsFloat() ) {
                    if( !::IntersectRect(&rcTemp, &m_rcItem, &pControl->GetPos()) ) continue;
                    pControl->DoPaint(hDC, rcPaint);
                }
            }
        }
        else {
            CRenderClip childClip;
            CRenderClip::GenerateClip(hDC, rcTemp, childClip);
            for( int it = 0; it < m_items.GetSize(); it++ ) {
                CControlUI* pControl = static_cast<CControlUI*>(m_items[it]);
                if( !pControl->IsVisible() ) continue;
                if( !::IntersectRect(&rcTemp, &rcPaint, &pControl->GetPos()) ) continue;
                if( pControl ->IsFloat() ) {
                    if( !::IntersectRect(&rcTemp, &m_rcItem, &pControl->GetPos()) ) continue;
                    CRenderClip::UseOldClipBegin(hDC, childClip);
                    pControl->DoPaint(hDC, rcPaint);
                    CRenderClip::UseOldClipEnd(hDC, childClip);
                }
                else {
                    if( !::IntersectRect(&rcTemp, &rc, &pControl->GetPos()) ) continue;
                    pControl->DoPaint(hDC, rcPaint);
                }
            }
        }
    }

    if( m_pVerticalScrollBar != NULL && m_pVerticalScrollBar->IsVisible() ) {
        if( ::IntersectRect(&rcTemp, &rcPaint, &m_pVerticalScrollBar->GetPos()) ) {
            m_pVerticalScrollBar->DoPaint(hDC, rcPaint);
        }
    }

    if( m_pHorizontalScrollBar != NULL && m_pHorizontalScrollBar->IsVisible() ) {
        if( ::IntersectRect(&rcTemp, &rcPaint, &m_pHorizontalScrollBar->GetPos()) ) {
            m_pHorizontalScrollBar->DoPaint(hDC, rcPaint);
        }
    }
}
```

控件容器绘制完自己后，遍历子控件（包括子控件容器）调用其DoPaint，完成子控件绘制

```C++
void CControlUI::DoPaint(HDC hDC, const RECT& rcPaint)
{
    if( !::IntersectRect(&m_rcPaint, &rcPaint, &m_rcItem) ) return;

    // 绘制循序：背景颜色->背景图->状态图->文本->边框
    if( m_cxyBorderRound.cx > 0 || m_cxyBorderRound.cy > 0 ) {
        CRenderClip roundClip;
        CRenderClip::GenerateRoundClip(hDC, m_rcPaint,  m_rcItem, m_cxyBorderRound.cx, m_cxyBorderRound.cy, roundClip);
        PaintBkColor(hDC);
        PaintBkImage(hDC);
        PaintStatusImage(hDC);
        PaintText(hDC);
        PaintBorder(hDC);
    }
    else {
        PaintBkColor(hDC);
        PaintBkImage(hDC);
        PaintStatusImage(hDC);
        PaintText(hDC);
        PaintBorder(hDC);
    }
}
```

最终的绘制都是通过渲染引擎CRenderEngine实现的。

这样看来，整个绘制思路还是很清晰的：CPaintManagerUI::MessageHandler（WM_PAINT）--->CContainerUI::DoPaint--->CControlUI::DoPaint--->CRenderEngine