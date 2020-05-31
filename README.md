# klb

流媒体服务器

* MNP协议
* HTTP-MNP协议
* HTTP-FLV服务端
* WS-FLV服务端

## 编译
```
Linux版本
make OS=linux ARCH=amd64 TAGS="debug normal"

Windows版本
make OS=windows ARCH=amd64 TAGS="debug normal"
```

## 运行
```
cd ./lib
./klua  ./server/server.lua
```

```
媒体数据: 将111.flv文件放入 ./klb/lib/ 目录
http://127.0.0.1:8000
http://127.0.0.1:8000/flv.html
```
