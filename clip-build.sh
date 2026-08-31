#!/usr/bin/env bash
# clip-build.sh - klb 编译裁剪 (no-* / --disable-* / use-* / --enable-*; min-core=最小)
set -euo pipefail

_klb_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
_clip_lib="${_klb_dir}/../build/scripts/lib/klb-clip.sh"

if [[ ! -f "${_clip_lib}" ]]; then
    echo "clip-build.sh: missing ${_clip_lib}" >&2
    exit 1
fi

# shellcheck disable=SC1091
source "${_clip_lib}"

_print=0
_info=0
_jobs="8"
_compose_tokens=()
_make_args=()
_raw_clip=""

usage() {
    cat <<EOF
用法: ${_klb_dir}/clip-build.sh [裁剪选项] [--] [make 目标...]

记名 (Autotools 对齐):
  no-* / --disable-*     裁掉库 (全量基准)
  use-* / --enable-*     加回库 (须配合 --min-core 或仅写 enable 时默认 min-core; make 直传亦可)
  --min-core             最小预设

示例:
  ./clip-build.sh --min-core --enable zlib --enable lpeg -j8
  ./clip-build.sh --min-core use-zlib use-net-proto --print
  ./clip-build.sh --full --disable-gui --disable-zlib -j8
  ./clip-build.sh --disable gui --disable sqlite --print

兼容:
  --minus LIB ...        同 --disable
  --core-plus LIB ...    同 --min-core --enable

选项:
  --print / --info / -j N / --raw '...' / -h

库名: zlib lpeg sqlite pcre2 cpp gui format qrencode net-proto wui wui-embed wui-sim
EOF
}

_is_make_target() {
    case "$1" in
        all | lib | so | klua | clean | strip | install | local | info)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

_is_clip_token() {
    case "${1,,}" in
        min-core | no-all | --min-core | full | all | --full \
        | minus | --minus | core-plus | --core-plus | --plus | plus \
        | --disable | --disable-* | disable-* | no-* \
        | --enable | --enable-* | enable-* | use | use-*)
            return 0
            ;;
        *)
            return 1
            ;;
    esac
}

while [[ $# -gt 0 ]]; do
    case "$1" in
        --raw)
            shift
            if [[ $# -lt 1 ]]; then
                echo "clip-build.sh: --raw 需要 MY_CLIP 字符串" >&2
                exit 1
            fi
            _raw_clip="$1"
            shift
            ;;
        --print)
            _print=1
            shift
            ;;
        --info)
            _info=1
            shift
            ;;
        -j)
            _jobs="${2:?-j requires N}"
            shift 2
            ;;
        -j*)
            _jobs="${1#-j}"
            shift
            ;;
        -h | --help)
            usage
            exit 0
            ;;
        --)
            shift
            _make_args+=("$@")
            break
            ;;
        --minus)
            _compose_tokens+=(minus)
            shift
            while [[ $# -gt 0 ]] && ! _is_clip_token "$1" && [[ "$1" != -- ]] \
                && [[ "$1" != -j ]] && [[ "$1" != -j* ]] \
                && [[ "$1" != --print ]] && [[ "$1" != --info ]]; do
                _compose_tokens+=("$1")
                shift
            done
            ;;
        --core-plus | --plus)
            _compose_tokens+=(core-plus)
            shift
            while [[ $# -gt 0 ]] && ! _is_clip_token "$1" && [[ "$1" != -- ]] \
                && [[ "$1" != -j ]] && [[ "$1" != -j* ]] \
                && [[ "$1" != --print ]] && [[ "$1" != --info ]]; do
                _compose_tokens+=("$1")
                shift
            done
            ;;
        --disable)
            if [[ $# -lt 2 ]]; then
                echo "clip-build.sh: --disable 需要库名" >&2
                exit 1
            fi
            _compose_tokens+=("--disable-$2")
            shift 2
            ;;
        --enable)
            if [[ $# -lt 2 ]]; then
                echo "clip-build.sh: --enable 需要库名" >&2
                exit 1
            fi
            _compose_tokens+=("--enable-$2")
            shift 2
            ;;
        use)
            if [[ $# -lt 2 ]]; then
                echo "clip-build.sh: use 需要库名" >&2
                exit 1
            fi
            _compose_tokens+=("use-$2")
            shift 2
            ;;
        --min-core | --full | --disable-* | --enable-* | use-* | no-* | min-core | full)
            _compose_tokens+=("$1")
            shift
            ;;
        -*)
            echo "clip-build.sh: unknown option $1" >&2
            usage >&2
            exit 1
            ;;
        *)
            if _is_make_target "$1"; then
                _make_args+=("$1")
                shift
            else
                _compose_tokens+=("$1")
                shift
            fi
            ;;
    esac
done

if [[ -n "${_raw_clip}" ]]; then
    # shellcheck disable=SC2206
    MY_CLIP="$(klb_clip_normalize_my_clip_string ${_raw_clip})"
    _mode="raw"
elif ((${#_compose_tokens[@]} == 0)); then
    echo "clip-build.sh: 须指定 --min-core / --disable / --enable / use / --minus 等" >&2
    usage >&2
    exit 1
else
    MY_CLIP="${_compose_tokens[*]}"
    _mode="compose"
fi

echo "clip-build.sh: MY_CLIP=${MY_CLIP:-<empty>} tokens=${_compose_tokens[*]:-<raw>}"

if [[ "${_print}" -eq 1 ]]; then
    if [[ "${_mode}" == "compose" ]]; then
        klb_clip_resolve compose "${_compose_tokens[@]}"
    else
        printf '%s\n' "${MY_CLIP}"
    fi
    exit 0
fi

if [[ "${_info}" -eq 1 ]]; then
    make -C "${_klb_dir}" MY_CLIP="${MY_CLIP}" info
    exit 0
fi

if ((${#_make_args[@]} == 0)); then
    _make_args=(all klua)
fi

make -j"${_jobs}" -C "${_klb_dir}" MY_CLIP="${MY_CLIP}" "${_make_args[@]}"
