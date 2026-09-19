# src_vendor

第三方开源可裁剪库: 编入 `libklb`, 用独立 `no-<lib>` 裁剪.

* 子目录: 上游原名 + 版本 (例如 `mbedtls-3.6.7/`)
* 不要给上游目录加 `klb` 前缀
* 不要把第三方放进 `src_packages` (那是自有可裁剪包)
* 构建走本仓 `Makefile` / `clip.mk`; 不用上游 CMake
* 现行 zlib / qrencode 等可暂留 `src_c`

## 下载来源

须用官方 **release tarball** (含 generated 源). 不要用 GitHub Archive 自动包 (`source.tar.gz` / `archive/refs/tags`).

| 库 | 版本 | 上游下载 | 对照包 | SHA256 |
|----|------|----------|--------|--------|
| mbedTLS | 3.6.7 LTS | https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-3.6.7/mbedtls-3.6.7.tar.bz2 | `download/opensrc/mbedtls-3.6.7.tar.bz2` | `a7e8bcbec0e6f761b4af24f25677626b35f762f68eef79c08677a363212d11f6` |

发布页: https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.7

## mbedtls-3.6.7

clip `no-ssl` / `use-ssl`; min-core 默认裁.
树内只留 `library/*.c` `library/*.h` 与 `include/**/*.h` (对标 zlib / qrencode).
user config: `src_c/klbnet/klb_mbedtls_user_config.h`.
