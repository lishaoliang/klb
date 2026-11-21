--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   stage.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  deploy.build_stage
-- @note   从本机源码树生成 usr/local stage
-- @history 修改历史
--  \n [2026-10] 从 init.lua 拆出
--]]


local util = require("klbagent.core.tools.deploy.util")


local M = {}


-- @brief 从本机源码树生成 usr/local stage (供 source_install / remote.deploy)
-- @return ok[boolean]
-- @return out[string]
local function _build_stage(source_tree, stage_root)
	local script = string.format([[
#!/bin/bash
set -euo pipefail
_ws_root=%s
STAGE=%s
PREFIX="$STAGE/usr/local"
rm -rf "$STAGE"
mkdir -p "$PREFIX"
make -C "$_ws_root/klb" -f - _stage <<EOF
include Makefile
.PHONY: _stage
_stage:
	\$(call DO_install_by_path, $PREFIX)
EOF
mkdir -p "$PREFIX/include/pubfw" "$PREFIX/lib"
cp -f "$_ws_root/pubfw/inc/pubfw.h" "$PREFIX/include/pubfw/pubfw.h" 2>/dev/null || true
cp -f "$_ws_root/pubfw/lib/libpubfw.so" "$PREFIX/lib/" 2>/dev/null || true
cp -f "$_ws_root/pubfw/lib/libpubfw.a" "$PREFIX/lib/" 2>/dev/null || true
rsync -a --delete "$_ws_root/klb/bin/klbagent/" "$PREFIX/share/klua/klbagent/"
test -x "$PREFIX/bin/alua"
echo STAGE_OK
]], util.sh_quote(source_tree), util.sh_quote(stage_root))

	local tmp_dir = "/tmp"
	local home_tmp = os.getenv("HOME")
	if type(home_tmp) == "string" and home_tmp ~= "" then
		tmp_dir = home_tmp .. "/.tmp"
	end

	util.run("mkdir -p " .. util.sh_quote(tmp_dir))
	local sp = string.format("%s/alua_stage_%d.sh", tmp_dir, math.floor(os.time()))
	local ok_w, err_w = util.write_file(sp, script)
	if not ok_w then
		return false, "write stage script fail: " .. tostring(err_w)
	end

	local ok, out = util.run("bash " .. util.sh_quote(sp))
	util.run("rm -f " .. util.sh_quote(sp))
	return ok, out
end


M.build_stage = function (source_tree, stage_root)
	return _build_stage(source_tree, stage_root)
end


return M

-- end
