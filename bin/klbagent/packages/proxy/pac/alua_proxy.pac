// alua proxy PAC sample (B-3)
// 1) alua pkg run proxy serve
// 2) Browser: Automatic proxy configuration URL → file:///…/alua_proxy.pac
//    or host this file over http and point PAC URL there.
// Regenerate: alua pkg run proxy pac
function FindProxyForURL(url, host) {
  if (isPlainHostName(host) ||
      shExpMatch(host, "localhost") ||
      shExpMatch(host, "127.*") ||
      shExpMatch(host, "10.*") ||
      shExpMatch(host, "192.168.*") ||
      shExpMatch(host, "172.16.*") ||
      shExpMatch(host, "172.17.*") ||
      shExpMatch(host, "172.18.*") ||
      shExpMatch(host, "172.19.*") ||
      shExpMatch(host, "172.2?.*") ||
      shExpMatch(host, "172.30.*") ||
      shExpMatch(host, "172.31.*") ||
      dnsDomainIs(host, ".local"))
    return "DIRECT";
  return "SOCKS5 127.0.0.1:12818; SOCKS 127.0.0.1:12818; PROXY 127.0.0.1:12818";
}
