### 属性的解析和存储

​		解析属性的逻辑是在CDialogBuilder:: Create()的for中。解析方式没有自行脑补的那么高深，完全就是通过字符串判断来做解析的。通过字符串比较出设置的属性和值然后调用API函数进行设置。

​		属性解析全部都是存储在CPaintManagerUI::m_SharedResInfo中。m_SharedResInfo是一个结构体对象。Image:m_ImageHash、Font:m_CustomFonts、Default:m_AttrHash、MultiLanguage:m_MultiLanguageHash。

```c++
//资源类型
typedef struct DUILIB_API tagTResInfo
{
    //默认禁用颜色
    DWORD m_dwDefaultDisabledColor;
    //默认字体颜色
    DWORD m_dwDefaultFontColor;
    //超链接字体颜色
    DWORD m_dwDefaultLinkFontColor;
    //超链接鼠标悬停字体颜色
    DWORD m_dwDefaultLinkHoverFontColor;
    //默认选择背景颜色
    DWORD m_dwDefaultSelectedBkColor;
    //当前使用的默认字体
    TFontInfo m_DefaultFontInfo;
    //自定义字体
    CDuiStringPtrMap m_CustomFonts;
    //图片资源存储
    CDuiStringPtrMap m_ImageHash;
    //默认属性列表
    CDuiStringPtrMap m_AttrHash;
    //多语言
    CDuiStringPtrMap m_MultiLanguageHash;
} TResInfo;
```

​		不过上面也可能存储在TResInfo m_ResInfo;中。bool变量控制存储在不同的成员属性中。存储在共享资源或者是非共享资源是通过每个属性的add函数最后一个参数bShared和CPaintManagerUI::m_bForceUseSharedRes两者一个为真将存储在m_SharedResInfo否则CPaintManagerUI::m_ResInfo。

### Window属性

​		window属性设置解析在专门的CPaintManagerUI::SetWindowAttribute()中。duilib对window属性提供如下设置：size、sizebox、caption、roundcorner、mininfo、maxinfo、showdirty、noactivate、opacity、layeredopacity、layeredimage、disabledfontcolor、defaultfontcolor、linkfontcolor、linkhoverfontcolor、selectedcolor。偶尔几个属性也存储在CPaintManagerUI::m_bForceUseSharedRes或者CPaintManagerUI::m_ResInfo中。大部分写在成员变量中（size:m_szInitWindowSize; sizebox:m_rcSizeBox;caption:m_rcCaption;roundcorner:m_szRoundCorner;mininfo:m_szMinWindow;maxinfo:m_szMaxWindow;showdirty:m_bShowUpdateRect;noactivate:m_bNoActivate;opacity:m_nOpacity;layeredopacity:m_nOpacity;m_bLayeredChanged;layeredimage:TDrawInfo m_diLayered.sDrawString;）。

### 控件是如何组装存储的

​		在CDialogBuilder::_Parse()中，还是通过循环的方式遍历的XML来组织控件。循环内部通过递归_Parse()来检索每项XML子节点控件

```
if( node.HasChildren() )
{
    //解析控件中的子控件
    _Parse(&node, pControl, pManager);
}
```

 		遍历到深处没有子节点时通过获取父控件的容器指针IContainerUI*将子控件new出来的控件指针存储在IContainerUI:: m_items中。

```C++
//如果当前容器为NULL就获取父控件的容器
if( pContainer == NULL )
    pContainer = static_cast<IContainerUI*>(pParent->GetInterface(DUI_CTR_ICONTAINER));
if( pContainer == NULL )
    return NULL;
//将当前控件添加到父控件容器中。
if( !pContainer->Add(pControl) )
{
    pControl->Delete();
    continue;
}
```

对于控件的属性，先搜索并设置Define标签给定的默认属性。

```c++
// Init default attributes
if( pManager )
{
    //如果有pManager那么就尝试通过控件名称来获取pManager中存储的默认属性列表。
    pControl->SetManager(pManager, NULL, false);
    LPCTSTR pDefaultAttributes = pManager->GetDefaultAttributeList(pstrClass);
    if( pDefaultAttributes )
    {
        //有默认属性列表那么就设置默认属性
        pControl->SetAttributeList(pDefaultAttributes);
    }
}
```

设置默认属性后再设置用户自己配置的属性列表。

```c++
// Process attributes
if( node.HasAttributes() )
{
    //如果有属性那么就设置用户设置的一些属性
    // Set ordinary attributes
    int nAttributes = node.GetAttributeCount();
    for( int i = 0; i < nAttributes; i++ )
    {
        pControl->SetAttribute(
            node.GetAttributeName(i),
            node.GetAttributeValue(i));
    }
}
```

通过duilib提供的简略文档和整理现有的Duilib控件类可以确定都是继承CControlUI类。

![](.\img\CControlUI.png)