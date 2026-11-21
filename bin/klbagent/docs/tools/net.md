# net tools

| tool | profile | notes |
|------|---------|-------|
| `net.status` | net | backends + `proxy` (`ALUA_PROXY` / `ALUA_NO_PROXY`) |
| `net.http_get` | net | proxy → chain `open_tunnel` (+TLS for https); else klbhttp / curl |
| `net.http_post` | net | same; body / json |
| `net.session_begin` / `end` | net | cookie jar session |

## Forward proxy (chain shorthand)

| source | meaning |
|--------|---------|
| `args.proxy` | per-call; `http://host:port` / `socks5h://host:port` or `{host,port,type,tls}` |
| `ALUA_PROXY` | process env default when args omit proxy |
| `ALUA_NO_PROXY` / `NO_PROXY` | comma list; `*` / `.dom` / `*.dom` / `host` / `host:port` → direct（与入口 `cfg.no_proxy` 同语义，N2-3） |

Schemes: `http` / `https` → hop `http-connect`; `socks5` / `socks5h` → hop `socks5`.  
Resolved via `packages/proxy/chain.hops_from_proxy` (single-hop). Result may include `proxy={host,port,tls,type,chain}`.

| hop | transport |
|-----|-----------|
| http:// + hops | **chain** `open_tunnel`（含 socks5 / http-connect / 多跳） |
| https + hops | **chain** `open_tunnel` + `kproxy.tcp:upgrade_tls`（v0.1.84；含 socks5） |
| https + http-connect（链失败回退） | klbhttp CONNECT+upgrade / curl |
| 无代理 | klbhttp；否则 curl |

`result.backend` 可为 `chain` / `klbhttp` / `curl`；链路径时 `proxy.chain.via=open_tunnel` 或 `open_tunnel+tls`。

对本机 `alua pkg run proxy serve` (`:12818`) 走 HTTP URL 时推荐:

```
export ALUA_PROXY=socks5h://127.0.0.1:12818
alua call net.http_get '{"url":"http://127.0.0.1:<port>/"}'
```

```
export ALUA_PROXY=http://127.0.0.1:8080
export ALUA_NO_PROXY=127.0.0.1,localhost,.local
alua call runtime.selfcheck {}
alua call net.status {}
alua call config.set '{"module":"runtime","cfg":{"profile":"net"}}'
alua call net.http_get '{"url":"https://example.com/","max_bytes":4096}'
alua call net.http_get '{"url":"https://example.com/","proxy":"http://127.0.0.1:8080"}'
alua call net.http_get '{"url":"http://127.0.0.1/","proxy":"socks5h://127.0.0.1:12818"}'
```

Check flow: `packages/proxy/chain/init.lua` (`hops_from_proxy` / `open_tunnel`).

# end
