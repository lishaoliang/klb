# src_3rdparty

第三方开源可裁剪库: 编入 `libklb`, 用独立 `no-<lib>` 裁剪.

* 子目录: 上游原名 + 版本 (例如 `mbedtls-3.6.7/`)
* 不要给上游目录加 `klb` 前缀
* 不要把第三方放进 `src_packages` (那是自有可裁剪包)
* 构建走本仓 `Makefile` / `clip.mk`; 不用上游 CMake
* **zlib** / **qrencode** / **pcre2** / **sds** / **mbedtls** 已迁本目录（见下表）

## 下载来源

须用官方 **release tarball** (含 generated 源). 不要用 GitHub Archive 自动包 (`source.tar.gz` / `archive/refs/tags`).

| 库 | 版本 | 上游下载 | 对照包 | SHA256 |
|----|------|----------|--------|--------|
| zlib | 1.3.2 | https://zlib.net/zlib-1.3.2.tar.gz | 自 `klbpfs/mtd` 迁入；旧 `src_c/zlib-1.2.11` → `backup/src_c/` | — |
| qrencode | 4.1.1 | https://fukuchi.org/works/qrencode/ | 自 `src_c/qrencode-4.1.1` 迁入 | — |
| pcre2 | 10.36 | https://github.com/PCRE2Project/pcre2/releases | 自 `src_c/klbthird/pcre2` 迁入 | — |
| sds | 2.0.0 | https://github.com/antirez/sds | 自 `src_c/klbthird/sds` 迁入 | — |
| mbedTLS | 3.6.7 LTS | https://github.com/Mbed-TLS/mbedtls/releases/download/mbedtls-3.6.7/mbedtls-3.6.7.tar.bz2 | `download/opensrc/mbedtls-3.6.7.tar.bz2` | `a7e8bcbec0e6f761b4af24f25677626b35f762f68eef79c08677a363212d11f6` |

发布页: https://github.com/Mbed-TLS/mbedtls/releases/tag/mbedtls-3.6.7

## zlib-1.3.2

clip `no-zlib` / `use-zlib`; min-core 默认裁.
Lua 绑定仍 `src_c/klua/lua-zlib-1.3`.
对外头副本: `inc/klbthird/zlib.h` / `zconf.h` (与本树同步为 1.3.2).

## qrencode-4.1.1

clip `no-qrencode` / `use-qrencode`; min-core 默认裁.
树内为 libqrencode 源文件子集 (对标 zlib 整库编入).

## pcre2

clip `no-pcre2` / `use-pcre2`; min-core 默认裁.
仅编 `src/*.c`；排除 `pcre2grep.c` / `pcre2_dftables.c` / `pcre2_fuzzsupport.c`；禁扫 `testdata/`.
对外头副本: `inc/klbthird/pcre2.h` / `pcre2posix.h`.

## sds

常开（无 clip 门）；编入 `MY_DIRS`.
对外头副本: `inc/klbthird/sds.h`；源内 `sdsalloc.h` 经 `-I src_3rdparty/sds`.

## mbedtls-3.6.7

clip `no-ssl` / `use-ssl`; min-core 默认裁.
树内只留 `library/*.c` `library/*.h` 与 `include/**/*.h` (对标 zlib / qrencode).
user config: `src_c/klbnet/klb_mbedtls_user_config.h`.
