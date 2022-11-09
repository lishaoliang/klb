# KLB项目

* C/C++, Lua

## 目的
* 1. 搭建基础C/C++开发环境
* 2. 搭建Lua基础开发环境
* 3. 搭建流媒体开发环境
* 4. 搭建Gui开发环境


## 示例

* demo示例, 暂只支持windows

* 基础库示例

```
cd ./bin

./klua.exe ./help_lua/k_test/t_kco.lua
./klua.exe ./help_lua/k_test/t_ksys.lua
./klua.exe ./help_lua/k_test/t_krand.lua
```


* 命令行示例1 : http-flv流服务器

```
1. step1 在win命令行CMD中

cd ./bin
./klua.exe ./sample1/http_flv.lua


2. step2 使用Google Chrome/Firefox等支持wasm的浏览器测试http-flv流媒体

http://127.0.0.1:3456/httpflv.html

```

* 界面示例

```
cd ./bin
./wlua.exe ./sample_ui/main_ui.lua
```


## 文件目录

```
./klb
 ->bin  -------------------- lua,调试,生成等目录 
   ->help_lua  ------------- lua帮助文件: 仿C提供的接口提示
   ->kbase  ---------------- lua基础封装
   ->knet  ----------------- lua网络封装
   ->res  ------------------ 资源目录: 字体,网页,JS,H264等文件
   ->sample_ui  ------------ lua-ui示例
   ->sample1  -------------- lua示例1
   ->sample2  -------------- lua示例2
   --klua.exe  ------------- klua命令行程序
   --wlua.exe  ------------- wlua界面程序

 ->docs
   ->cn
   ->en
   ->licenses

 ->lib
 ->proj
 ->src

 ->inc  -------------------- C头文件
 ->src_c  ------------------ C实现文件
   ->klbbase  -------------- C基础2
   ->klbformat  ------------ C文件结构
   ->klbgui  --------------- C简易gui
   ->klbmem  --------------- C内存
   ->klbnet  --------------- C网络
   ->klbplatform  ---------- C平台相关
   ->klbthird  ------------- 第三方库
   ->klbutil  -------------- C基础1
   ->klua  ----------------- lua基础2
     ->extension  ---------- 标准C扩展
     ->klua_base  ---------- lua基础2
     ->klua_format  -------- lua文件结构
     ->klua_multithread  --- lua多线程相关
     ->klua_net  ----------- lua网络部分
     ->klua_platform  ------ lua平台相关: 使用操作系统平台相关函数
     ->klua_util  ---------- lua基础1
     ->lpeg-1.0.2  --------- lua正则
     ->lsqlite3  ----------- lua数据库
     ->lua-5.4.1  ---------- lua虚拟机
     ->lua-cjson-2.1.0  ---- json解析
     ->luafilesystem-2.0  -- 操作文件系统
     ->LuaXML_130610  ------ xml解析
     --klua.c  ------------- klua提供的库
     --klua_env.c  --------- lua附加环境
   ->libavutil  ------------ ffmpeg基础代码

 ->inc_hpp  ---------------- C++头文件
 ->src_cpp    -------------- C++实现文件
   ->klbbase  -------------- C++基础2
   ->klbmem  --------------- C++内存
   ->klbnet  --------------- C++网络
   ->klbplatform  ---------- C++平台相关
   ->klbutil --------------- C++基础1

 ->src_packages  ----------- lua单项附加库
   ->kpa_flv  -------------- kpa_flv库
   ->kpa_http  ------------- kpa_http库
   ->kpa_mnp  -------------- kpa_mnp库
   ->kpa_rtsp -------------- kpa_rtsp库
   ->kpa_sip  -------------- kpa_sip库
   ->kpa_ws  --------------- kpa_ws库

 ->third

 ->klua  ------------------- klua命令行
 ->wlua  ------------------- wlua窗口

 --Makefile
```
