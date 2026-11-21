# skill: runtime (L2)

| field | value |
|-------|-------|
| title | runtime / host |
| summary | version, ping, selfcheck, hostinfo, cwd, env, tls, reload, parallel (session=core.session 内调); identity/token = `ssid.*` |
| when_to_read | after L1; any session start / install check / host morph; identity or temp token |
| category | runtime |
| l3_paths | `tools/runtime.md`, `tools/ssid.md` |

```
alua call docs.read {"path":"tools/runtime.md"}
alua call docs.read {"path":"tools/ssid.md"}
alua call runtime.selfcheck {}
alua call runtime.hostinfo {}
alua call ssid.whoami {}
```
