# auto
` auto 不能推到出 & `
````c++
const XmlStanza::StanzaList& children() const {
  return children_;
}
//构造一个临时变量
auto children = list->children();
//使用的引用访问
auto& children = list->children();
````

`auto& 不会去掉 函数返回值的const 属性`
````c++
const XmlStanza::StanzaList& children() const {
  return children_;
}

auto& children = list->children();
//错误 children 是带有const 的引用，不能改变
children = xxxx;

````


`怪异的写法`
````c++
//难看的写法
auto params = FRAMEWORK::Variant();
//正常写法
FRAMEWORK::Variant params；
````

`即使用了auto 我们也要知道变量的类型,否则容易踩坑`
````c++
    //url 是一个 char*,是一个指针
    auto url = prop->GetString("url");
    //lambda 捕获了一个 char*
    looper_->RunOrPostRunnable([url]() {
        
    });

````


auto 用的时候需要对变量的真正类型心知肚明，不要滥用，我们项目里面的使用场景
````c++
 //变量名太长
 auto request = base::make_unique<BASE_PROCESSING::IPCClientChannel::Request>();
 
//for 
 for (auto& fun : log_interceptor_func_) {
    if (fun(*log, filter_content)) {
        return;
    }
}

//明确类型的强制转换
 auto uv_req = static_cast<uv_req_t*>(req);

````