# klb

流媒体服务器

* MNP协议
* HTTP-MNP协议
* HTTP-FLV服务端
* WS-FLV服务端

## 目的
* 熟悉Go语言
* 熟悉混合编程
* 熟悉流媒体

## 下载
* 本项目需要使用go编译,所以需要下载到"GOPATH"目录下
```
go get github.com/lishaoliang/klb
go get gitee.com/lishaoliang/klb
```

## 编译
```
Linux版本
make

Linux版本,带全参数
make OS=linux ARCH=amd64 TAGS="debug normal"

Windows版本
make OS=windows ARCH=amd64 TAGS="debug normal"
```

## 安装/卸载
* 安装路径: "/usr/local/lib/klb1.X"
* 执行路径: "/usr/local/bin/klb"
```
make install
make uninstall
```

## 运行
* 配置文件 ./lib/conf.lua
* 当前目录运行
```
vi ./lib/conf.lua
local test = true

cd ./lib
./klb  ./server.lua
```

* 安装运行
```
klb server
```

* 测试媒体
```
媒体数据: 将*.flv文件放入 ./lib/media 目录
http://127.0.0.1:8000
http://127.0.0.1:8000/flv.html
```
