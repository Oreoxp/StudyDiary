### 消息处理函数表

       MFC和VCL在对消息进行封装的时候，都没有使用虚函数机制。原因是虚函数带来了不必要的空间开销。那么它们用什么来代替虚函数，即可减少空间浪费，也可实现类似虚函数的多态呢？让我们从一个例子开始。

假设父类ParentWnd处理了100个消息，并且将这100个处理函数声明为虚函数；此时有一个子类ChildWnd它只需要处理2个消息，另外98个交由ParentWnd默认处理，但是ChildWnd的虚表仍然占有100个函数指针，其中2个指向自己的实现，另外98个指向父类的实现，情况大概像下面这样：



      指向父类实现的函数指针浪费了空间，当控件类非常多的时候，这种浪费就非常明显。因此必须走另一条路，将不必要的函数指针去掉，如下图所示：




       ChildWnd去掉函数指针之后，当有一个消息需要Fun100处理时，ChildWnd就束手无策了。需要一个方法让ChildWnd能够找到父类的表，我们再对这个数据结构进行改进如下：



    现在看来好多了，如果ChildWnd有一个消息需要Fun1处理，则查找ChildWnd的MsgHandlers，找到Fun1函数指针调用之；如果需要Fun100处理，发现ChildWnd的MsgHandlers没有Fun100，则通过ParentTable找到父类的MsgHandlers继续查找。如此一直查找，到最后再找不到，就调用DefWindowProc作默认处理。

 


       MFC和VCL都是通过类似的方法实现消息分派的。只是VCL有编译器的支持，直接将这个表放到VMT中，因此实现起来非常简单，只需在控件类里作如下声明：

procedure WMMButtonDown(var Message: TWMMButtonDown); message WM_MBUTTONDOWN;

TObject.Dispatch会将WM_MBUTTONDOWN正确分派到WMMButtonDown。

       MFC就没有这么简单，它需要手工去构建这个表，如果一个类想处理消息，它必须声明一些结构和函数，代码类似这样：

struct AFX_MSGMAP_ENTRY

{

    UINT nMessage;   // 消息
    
    AFX_PMSG pfn;    // 消息处理函数

};

 

struct AFX_MSGMAP

{

    const AFX_MSGMAP* pBaseMap;  //指向基类的消息映射
    
    const AFX_MSGMAP_ENTRY* lpEntries; //消息映射表

};

 

class CMFCTestView : public CView

{

protected:

    void OnLButtonDown(UINT nFlags, CPoint point);

private:

    static const AFX_MSGMAP_ENTRY _messageEntries[];

protected:

    static const AFX_MSGMAP messageMap;
    
    virtual const AFX_MSGMAP* GetMessageMap() const;

};

仔细看_messageEntries和messageMap的声明，是不是和上面的图非常相似，接下来看看实现部分如何初始化这个表：

const AFX_MSGMAP* CMFCTestView::GetMessageMap() const

    { return & CMFCTestView::messageMap; }

 


const AFX_MSGMAP CMFCTestView::messageMap =

{ & CView::messageMap, & CMFCTestView::_messageEntries[0] };

 

const AFX_MSGMAP_ENTRY CMFCTestView::_messageEntries[] =

{

    { WM_LBUTTONDOWN, (AFX_PMSG)&OnLButtonDown }
    
    {0, (AFX_PMSG)0 }

};

 

void CMFCTestView::OnLButtonDown(UINT nFlags, CPoint point)

{

    CView::OnLButtonDown(nFlags, point);

}

       messageMap的第一个成员指向其父类的messageMap，即& CView::messageMap；第二个成员则指向下面的消息映射表；

GetMessageMap是一个虚函数，显然是为了父类分派消息的时候能够找到正确的消息映射结构，后面会看到这一点。

       _messageEntries数组为消息映射表，第一个元素处理WM_LBUTTONDOWN消息，其处理函数是OnLButtonDown，这个函数在CMFCTestView声明和实现；第二个元素标识了映射表的结尾。
    
       现在，你想处理什么消息，都可以往_messageEntries里加新的元素并指定你的处理函数，只是如果每一个类都需要手工写这些代码，那将是很繁琐的事情；幸好C++有宏，可以用宏来作一些简化的代码，先将消息映射的声明用DECLARE_MESSAGE_MAP()表示，则类声明变成下面这样：

class CMFCTestView : public CView

{

DECLARE_MESSAGE_MAP()

};

       而实现部分，变成了下面这样：

BEGIN_MESSAGE_MAP(CMFCTestView, CView)

    ON_WM_LBUTTONDOWN()

END_MESSAGE_MAP()

       这就是MFC的消息映射宏，实际的代码和这里有一些出入，不过大体是差不多的，其核心作用就是构造消息处理函数表。现在打开VC去看看那几个宏，是不是觉得其实很简单。
    
       建好消息映射表后，接下来要看看消息如何流到指定的处理函数里。

### 消息流向

       我们可以认为消息的最初进入点是CWnd::WindowProc，在以后的文章会说明消息如何流到这个函数。CWnd是所有窗口类的基类，CMFCTestView当然也是CWnd的子孙类。WindowProc的代码很简单，调用OnWndMsg进行消息分派，如果没有处理函数，则调用DefWindowProc作默认处理：

LRESULT CWnd::WindowProc(UINT message, WPARAM wParam, LPARAM lParam)

{

    // OnWndMsg does most of the work, except for DefWindowProc call
    
    LRESULT lResult = 0;
    
    if (!OnWndMsg(message, wParam, lParam, &lResult))
    
        lResult = DefWindowProc(message, wParam, lParam);
    
    return lResult;

}

       最重要的是OnWndMsg成员函数，我将里面的代码作了简化，去掉了命令通知消息和一些优化代码，最终的代码如下：

BOOL CWnd::OnWndMsg(UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult)

{

    LRESULT lResult = 0;
    
    const AFX_MSGMAP* pMessageMap;
    
    //取得消息映射结构，GetMessageMap为虚函数，所以实际取的是CMFCTestView的消息映射
    
    pMessageMap = GetMessageMap();
    
    // 查找对应的消息处理函数
    
    for (pMessageMap != NULL; pMessageMap = pMessageMap->pBaseMap)
    
        if (message < 0xC000)
    
            if ((lpEntry = AfxFindMessageEntry(pMessageMap->lpEntries, message, 0, 0)) != NULL)
    
                goto LDispatch;
    
    ... ...

LDispatch:

    //通过联合来匹配正确的函数指针类型
    
    union MessageMapFunctions mmf;
    
    mmf.pfn = lpEntry->pfn;
    
    int nSig;
    
    nSig = lpEntry->nSig;
    
    //nSig代表函数指针的类型，通过一个大大的Case来匹配。
    
    switch (nSig)
    
    {
    
    case AfxSig_bD:
    
        lResult = (this->*mmf.pfn_bD)(CDC::FromHandle((HDC)wParam));
    
        break;
    
    ... ...
    
    case AfxSig_vwp:
    
        {
    
            CPoint point((DWORD)lParam);
    
            (this->*mmf.pfn_vwp)(wParam, point);
    
            break;
    
        }
    
    }

   


    return TRUE;

}

       在代码中看到GetMessageMap的调用，根据前面的分析已经知道这是一个虚函数，所以实际调用到的是CMFCTestView::GetMessageMap()，也就是这里取到了CMFCTestView的消息映射结构。
    
       接下来根据当前的消息表寻找对应的消息处理函数，调用AfxFindMessageEntry查找消息映射表，如果找不到就继续到基类去找，这就是For循环做的事情。AfxFindMessageEntry使用内嵌汇编来提高查找的效率， VCL也是这样做的。
    
       如果找到处理函数，则调用goto LDispatch;跳到下面的Case语句，根据nSig判断函数指针的实际类型，最后转换并调用之，此时我们的OnLButtonDown函数就被调用到了。

我们看到了Goto语句的使用，也看到下面大大的Case，这都是OO设计的禁忌，特别是下面的Case，为什么要用Case呢？这是由消息映射宏的设计决定的。看看MFC实际的消息映射表结构是怎么样的：

struct AFX_MSGMAP_ENTRY

{

    UINT nMessage;   // windows message
    
    UINT nCode;      // control code or WM_NOTIFY code
    
    UINT nID;        // control ID (or 0 for windows messages)
    
    UINT nLastID;    // used for entries specifying a range of control id's
    
    UINT nSig;       // signature type (action) or pointer to message #
    
    AFX_PMSG pfn;    // routine to call (or special value)

};

最关键的是nSig和pfn，pfn虽然声明为AFX_PMSG类型的函数指针，但实际的函数类型却各各不同，列其中的几个来看看：

#define ON_WM_MOVE() /

{ WM_MOVE, 0, 0, 0, AfxSig_vvii, /

(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(int, int))&OnMove },

 

#define ON_WM_SIZE() /

{ WM_SIZE, 0, 0, 0, AfxSig_vwii, /

(AFX_PMSG)(AFX_PMSGW)(void (AFX_MSG_CALL CWnd::*)(UINT, int, int))&OnSize },

WM_MOVE消息和WM_SIZE消息的函数类型就不一样，那么CWnd::OnWndMsg在分派的时候如何知道这个函数指针的确切类型呢，答案就是nSig，它指明了处理这个消息的函数类型，如上面的WM_MOVE是AfxSig_vvii。CWnd::OnWndMsg在分派消息的时候，要根据nSig将pfn强制转换成相应的函数类型。

在我看来，这实在是非常拙劣的设计，假如新的Windows出现了一批新的消息，而这些消息需要指定新的函数类型，这个时候要怎么做：

1.         更新AFXMSG_.H头文件，提供新的消息宏和nSig类型。

2.         修改AFXIMPL.H头文件，为MessageMapFunctions联合加上新的函数类型。

3.         修改CWnd::OnWndMsg，在Case后面加上新的转换。

如果要通过修改上层框架的代码来支持消息处理的扩展，那就太不明智了。

       大概MS也考虑到这种弊端，因此给出了一个通用的消息宏：

#define ON_MESSAGE(message, memberFxn) /

{ message, 0, 0, 0, AfxSig_lwl, /

(AFX_PMSG)(AFX_PMSGW)(LRESULT (AFX_MSG_CALL CWnd::*)(WPARAM, LPARAM))&memberFxn },

    这个宏传一个消息名和一个函数名，其函数类型固定是LRESULT (CWnd::*)(WPARAM, LPARAM)。假设我们要处理WM_RBUTTONDOWN消息，可以这样做：
    
       首先在消息映射宏里这样写：

BEGIN_MESSAGE_MAP(CMFCTestView, CView)

    ON_MESSAGE(WM_RBUTTONDOWN, OnRButtonDown)

END_MESSAGE_MAP()

       然后声明和实现OnRButtonDown成员函数：

LRESULT CMFCTestView::OnRButtonDown(WPARAM wParam, LPARAM lParam)

{

    MessageBox("OnRBUttonDown");
    
    return Default();

}

对于其他消息也类似这样处理。这种方式要比前面的好得多，至少消息处理函数的形式统一起来了。而MFC如果都以这种形式作为处理函数，也没有必要在OnWndMsg里面加一个Case来强制转换各种函数类型。

但这种方法的缺点也很明显，就是参数意义很模糊，几乎很难从wParam看出它的作用。有没有一种方法，使得消息宏的声明如ON_MESSAGE一样统一，又能让处理函数的参数变得有意义呢？实际上这样的方法已经在VCL中存在了，那就是用一个布局相容的结构来做参数，如：

// 通用窗口消息结构

typedef struct tagWNDMSG {

    UINT message;
    
    WPARAM wParam;
    
    LPARAM lParam;
    
    HRESULT hr;

} WNDMSG, *PWNDMSG;

// 按钮点下消息结构

typedef struct tagLBUTTONDOWNMSG {

    UINT message;
    
    UINT Flag;
    
    WORD X;
    
    WORD Y;
    
    HRESULT hr;

} LBUTTONDOWNMSG, *PLBUTTONDOWNMSG;

WNDMSG和LBUTTONDOWNMSG在内存中的布局是一样的，可以声明一个一致的函数指针类型，以及一个统一的消息宏，如下面这样：

// 消息处理器类型

typedef void (CWnd:: *MSGHANDER) (WNDMSG &msg);

// 通用消息宏,MsgHandler的参数形式必须与MSGHANDER相同

#define ADDMESSAGEHANDLER(message, MsgHandler) /

    { message, 0, 0, 0, 0, (AFX_PMSG)&MsgHandler },

以后就统一用ADDMESSAGEHANDLER来加消息宏。CWnd:OnWndMsg的代码也会变得干净很多，根据message找到消息处理器后就不需要Case了，只需要将消息参数装进一个WNDMSG结构并传给消息处理器然后调用。代码类似下面这样：

BOOL CWnd::OnWndMsg( UINT message, WPARAM wParam, LPARAM lParam, LRESULT* pResult )

{

    ... ...
    
    //找处理函数
    
    const AFX_MSGMAP_ENTRY* lpEntry = FindMessageEntry(GetMessageMap(), message);  
    
    if (lpEntry != NULL)
    
    {
    
        MessageMapPorc mmp;
    
        mmp.pfn = lpEntry->pfn;
    
        //生成WNDMSG结构
    
        WNDMSG msg;
    
        msg.hr = 0;
    
        msg.wParam = wParam;
    
        msg.lParam = lParam;
    
        //调用处理函数
    
        (this->*mmp.MsgProc)(msg);
    
        *pResult = msg.hr;
    
        return TRUE;
    
    }
    
    else
    
        return FALSE;

}

对于每一个消息处理器，如果没有对应的消息结构，就用通用的WNDMSG做参数，如果有的话可以用该消息结构做参数，只要其内存布局与WNDMSG一样。如下面的代码：

void CWnd::OnBtnDown( LBUTTONDOWNMSG &msg )

{

    ... ...

}

在OnBtnDown函数里，直接访问LBUTTONDOWNMSG里面的X和Y，而不再需要去取lParam的高位和低位。剩下的问题就是针对每个消息提供相应的消息结构。