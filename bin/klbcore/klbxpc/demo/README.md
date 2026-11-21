# xpc 双进程手测 demo

路径: `klb/bin/klbcore/klbxpc/demo/`

## 运行

```bash
KLB=/tmp/<tree>/klb
export LD_LIBRARY_PATH="$KLB/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

# 终端1 (transport 可选: msgq|unix|npipe|sysmq; 默认 msgq)
"$KLB/lib/klua" "$KLB/bin/klbcore/klbxpc/demo/server.lua" xdemo1 sysmq

# 终端2 (须与 server 同 transport)
"$KLB/lib/klua" "$KLB/bin/klbcore/klbxpc/demo/client.lua" xdemo1 sysmq
```

须使用**已编入 kxpc** 的 `klua`/`libklb`(本树 `make` 产物). 默认模块名 `xdemo1`；可传第 1 参数自定义（须唯一；长度上限见 `klbxpc.NAME_MAX`=107；建议短名以免残留对象导致 create 失败）。第 2 参数为传输。

## 覆盖

| 步骤 | API |
|------|-----|
| server | `kxpc.new_module[_unix|_npipe|_sysmq]` / `recv` / `response` |
| client | `open[_unix|_npipe|_sysmq]` / `post` / `request` / `notify` |
| 结束 | client `post("quit")` → server `ksys.exit` |

脚本层 `klbcore.klbxpc` 为 `kxpc` 的协程封装; 本 demo 直接用 `kxpc` 便于无 kco 依赖的双进程冒烟.

**sysmq**: POSIX mqueue; 残留队列在 `/dev/mqueue` (如 `/klb.xpc.in.<name>`); owner `destroy` 才 `mq_unlink`.
