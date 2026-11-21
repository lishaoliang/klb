# L1 peers / mesh (AI only)

Pref: `peers.json` (+ optional `peers.<id>.env` for tokens).  
Tools: `peer.list` / `peer.ping` / `peer.approve` / `peer.revoke`.  
CLI: `alua peer …`.  
`remote.call` / `pull` / `push` / `deploy` / `rpc_call` accept `peer=<id>` when **trusted**.

| Must | Must not |
|------|----------|
| Write full ids: `linux` / `wsl` / `windows` | Store `peer=win` |
| Approve before default `peer=` jump | Auto-trust LAN discovery |
| Keep raw in `serve.env`; temp tickets in ssid **memory** only (business calls pass **no** `token_ref`) | Put long tokens in JSON; write `temp.*.env` |
| Respect peer **profile** on far side | Treat mesh as canceling serve token |

Details → `tools/peers.md` / `tools/remote.md`.
