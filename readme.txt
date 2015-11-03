最近一个windows的项目(手游服务器)中不允许使用boost，这让长期用boost::asio搞linux
服务程序开发的我很不适应，于是抽了一周的时间研究了boost::asio的IOCP实现，并实现了
一个简化版，主要代码仅千行，正好能用于当前的项目。
这个代码不推荐大家项目中使用(直接用boost就得了)，但也许对新手学习asio的内部机制
会有帮助。如果您发现代码有任何问题欢迎联系我。

代码目录：
util：一些公共代码，支持win和linux
asio: 简化的IOCP asio实现
snet: 一个简单的网络库，简单到没法实用(除了加入几十行的包边界的解析)
snet_test: snet的测试代码
test_echo: echo服务器，支持多客户端哦
test_timer: 定时器

另外util及asio编译不依赖c++11的新特性，vs2003以上版本都可编译，但snet里用到了bind,
需要vs2012及以上版本，当然自己实现个简化的bind也是可以的。

