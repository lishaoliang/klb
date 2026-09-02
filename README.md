# klb (klua)

klb 是面向嵌入式的 Lua 编程方案 (klua): 设备上的应用用 Lua 写, C 做扩展与裁剪.
并已集成常用库, 开箱 `require` (协程、GUI、网络、cjson/lfs 等).

* C, Lua, 嵌入式, GUI, 流媒体
* 开源仓库：<https://gitee.com/klua/klb>
* 镜像：<https://github.com/lishaoliang/klb>
* 许可：[LGPL-3.0](LICENSE)
* 文档：<https://gitee.com/klua/klua_doc>
* 组织：<https://gitee.com/klua>

## klua

* 写业务: Lua
* 用能力: `require("kco")` / `require("kgui")` 等 C 绑定
* 用脚本库: `require("klbcore.*")` (GUI 页面, net/rtsp 等)
* 运行: 可执行 `klua`, 或把 `libklb` 链进产品进程

Lua API 文档: <https://gitee.com/klua/klua_doc> (`lua/klua/`, `lua/klbcore/`)

## 文件目录

```text
.
 ->bin  -------------------- lua脚本 (发布含 klbcore)
   ->klbcore  -------------- lua支持库 (klbui / net / klbrtsp / klbsmp 等)

 ->inc  -------------------- C头文件
 ->src_c  ------------------ C实现文件
   ->klbapp  --------------- C应用壳
   ->klbbase  -------------- C基础2
   ->klbformat  ------------ C文件结构
   ->klbgui  --------------- C简易gui
   ->klbmem  --------------- C内存
   ->klbnet  --------------- C网络
   ->klbplatform  ---------- C平台相关
   ->klbthird  ------------- 第三方库
   ->klbutil  -------------- C基础1
   ->klua  ----------------- lua基础2
   ->compat  --------------- 兼容层
   ->libavutil  ------------ ffmpeg基础代码
   ->zlib-1.2.11  ---------- zlib
   ->qrencode-4.1.1  ------- QR码

 ->src_packages  ----------- 扩展包
   ->klbwui  --------------- GUI控件包

 ->proj/klua  -------------- klua可执行入口
 ->proj  ------------------- 工程文件 (VS2015)

 --Makefile
```

## 推荐

同组织相关仓库, 可对照阅读、组合使用:

* [klua_doc](https://gitee.com/klua/klua_doc) — 文档仓 (klb / pfs / Lua API)
* [portfs](https://gitee.com/klua/portfs) — 跨平台可移植文件系统
* [wlua](https://gitee.com/klua/wlua) — Windows 桌面 Lua 宿主 (SDL, 链接 libklb)
* [pubfw](https://gitee.com/klua/pubfw) — 公共框架 (目前空壳)
