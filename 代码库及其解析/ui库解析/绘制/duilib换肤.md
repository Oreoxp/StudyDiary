# duilib换肤

[TOC]



## 函数介绍

​		Duilib是一个以贴图为主要表现手段的界面库，实现换肤非常简单，可以通过给控件设置不同的图片来实现换肤，比如给需要换肤的控件调用CControlUI::SetBkImage。但是针对换肤功能，Duilib提供了更为简单的方法，即使用CPaintManagerUI::ReloadSkin。

​		假设我们给程序创建了两套皮肤，分别打包成skin1.zip和skin2.zip，在程序运行的时候，执行:
​			CPaintManagerUI::SetResourceZip(_T(“skin2.zip”)); // 或者skin1.zip
​			CPaintManagerUI::ReloadSkin();

​		这样简单的两行代码，就实现了全部窗口从skin1皮肤到skin2皮肤的切换。你也可以随时再次调用上面两行代码，把皮肤切换回去。

## 三、配置

​		首先，我们再编写一个test2.xml和test3.xml
修改其中的颜色分别为a0a000、a0a0a0![在这里插入图片描述](https://img-blog.csdnimg.cn/20210503004941893.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L0x5UmljczE5OTY=,size_16,color_FFFFFF,t_70)
![在这里插入图片描述](https://img-blog.csdnimg.cn/20210503005050957.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L0x5UmljczE5OTY=,size_16,color_FFFFFF,t_70)
将三个xml分别压缩放置于debug目录下：
![在这里插入图片描述](https://img-blog.csdnimg.cn/20210503005303104.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L0x5UmljczE5OTY=,size_16,color_FFFFFF,t_70)

## 四、代码

代码做如下修改
![在这里插入图片描述](https://img-blog.csdnimg.cn/20210503005155596.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L0x5UmljczE5OTY=,size_16,color_FFFFFF,t_70)

## 五、运行

编译运行，点击测试按钮，我们可以看到三种颜色的切换
![在这里插入图片描述](https://img-blog.csdnimg.cn/20210503005322398.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L0x5UmljczE5OTY=,size_16,color_FFFFFF,t_70)
![在这里插入图片描述](https://img-blog.csdnimg.cn/20210503005405347.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L0x5UmljczE5OTY=,size_16,color_FFFFFF,t_70)
![在这里插入图片描述](https://img-blog.csdnimg.cn/20210503005423435.png?x-oss-process=image/watermark,type_ZmFuZ3poZW5naGVpdGk,shadow_10,text_aHR0cHM6Ly9ibG9nLmNzZG4ubmV0L0x5UmljczE5OTY=,size_16,color_FFFFFF,t_70)