## 一、下载depot_tools

下载地址：https://storage.googleapis.com/chrome-infra/depot_tools.zip



## 二、环境变量

1. 把depot_tools加入到环境变量PATH中；添加系统变量DEPOT_TOOLS_WIN_TOOLCHAIN=0

2. 打开CMD终端(不是powershell), 在外部执行，执行

   ```bash
    gclient
    
    fetch v8
    
    
    git pull origin master
 gclient sync
   ```
   

## 三、构建项目

```bash
cd ~\v8\src #进入v8 src目录
gn gen --ide=vs out\default --args="is_component_build = true is_debug = true v8_optimized_debug = false"
```

如果出现vs版本对不上的问题，解决方式为先设置一下环境变量：

```
set GYP_MSVS_OVERRIDE_PATH=C:\path\to\your\VS2022
```



## 四、编译

构建完成后，在src\out\default下，能看到all.sln，双击打开

能看到v8_hello_world这个方案，鼠标右击“设为启动项目”，再次鼠标右击“生成”，这样就开始编译了



如果出现'ninja.exe' 不是内部或外部命令，也不是可运行的程序错误，将任意一个ninja.exe复制到depot_tools文件夹，因为gclient之后会删除这个文件，后面再叫上就行。

