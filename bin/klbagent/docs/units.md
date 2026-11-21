# units (systemd / service shapes)

One deployment shape: **one process per service**; all services use the same package code.

| shape | entry | pump | use when |
|-------|-------|------|----------|
| **multi process** | `alua pkg run <name> …` (one unit per service) | each process | isolate failures, per-service restart, per-service env |

Service names: `serve` (HTTP peer), `proxy` (SOCKS5/HTTP CONNECT/tunnel), `svn_https` (HTTPS edge + in-process `ksvnra`).

Ports come from per-service env (`ALUA_SVN_LISTEN`, `ALUA_ENTRY_PORT`, serve pool) or an explicit positional port.

## Multi process (one unit per service)

```ini
# ~/.config/systemd/user/alua-serve.service
[Unit]
Description=alua serve (HTTP peer)
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/alua --serve 12817 readonly
Restart=on-failure
RestartSec=3

[Install]
WantedBy=default.target
```

```ini
# ~/.config/systemd/user/alua-proxy.service
[Unit]
Description=alua proxy
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/alua pkg run proxy serve
Restart=on-failure
RestartSec=3

[Install]
WantedBy=default.target
```

```ini
# ~/.config/systemd/user/alua-svn.service
[Unit]
Description=alua svn_https
After=network.target

[Service]
Type=simple
ExecStart=/usr/local/bin/alua pkg run svn_https 8443
Restart=on-failure
RestartSec=3
Environment=ALUA_SVN_SVNSERVE=127.0.0.1:3690
Environment=ALUA_SVN_REPO_ROOT=/work/svn/repos

[Install]
WantedBy=default.target
```

```
systemctl --user daemon-reload
systemctl --user enable --now alua-serve alua-proxy alua-svn
```

Units for the same service must not be enabled twice: they would fight over the same ports (start fails fast on bind error).

## Low ports (< 1024)

User units cannot bind low ports. Use a **system** unit with:

```ini
[Service]
User=<run user>
ExecStart=/usr/local/bin/alua pkg run svn_https 443
AmbientCapabilities=CAP_NET_BIND_SERVICE
CapabilityBoundingSet=CAP_NET_BIND_SERVICE
NoNewPrivileges=no
```

Do **not** `setcap` the `alua` binary. The same hint is printed by `svn_https` on a bind attempt without the capability.

## Rule of thumb

* Need independent restart / isolation / different env → one unit per service (`alua pkg run`).
