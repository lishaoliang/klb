--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   css_loader.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  样式, 一般 样式与图片资源是对应的
-- @note   与 klbcore.klbui.csser (parse 管线) 不同; 启动前须 res.configure({ css_dirs = {...} })
-- @history 修改历史
--  \n [2026] 创建文件
--  \n [2026] load_css 按 S/M/L 写入字号阶梯, 供 S000_css 等通过 getter 读取
--]]
local lfs = require("lfs")
local klbui = require("klbcore.klbui")
local pathex = require("klbcore.util.pathex")
local tableex = require("klbcore.util.tableex")


local css_loader = {}


local css_dirs = {}			-- CSS 主题目录 (可直接 lfs 扫描的路径)


-- @brief 当前默认 css 值
local defaultCssCur = {}

-- @brief 当前全局 css
--	所有全局CSS属性
local globalCssCur = {}

-- @brief 不支持全局css控件的 所有css属性
local allCssCur = {}

-- @brief 字体大小, 取值: ['S', 'M', 'L']
local font_size_str = 'M'


-- 基础标准字号 (M 档默认 24, 对齐 pref DEFAULT_FONT / klbui_default_css.md)
local font_size_S = 16		-- 字号 --
local font_size_M = 20		-- 字号 -
local font_size = 24		-- 当前 标准字号
local font_size_L = 26		-- 字号 +
local font_size_XL = 28		-- 字号 ++
local font_size_max = 42	-- 字号 max


-- @brief 按字号档位写入像素阶梯
-- @param [in] tier[string]	'S' / 'M' / 'L'
-- @return 无
local function ApplyFontTier(tier)
	if 'S' == tier then
		font_size_S = 14
		font_size_M = 15
		font_size = 16
		font_size_L = 18
		font_size_XL = 20
		font_size_max = 36
	elseif 'L' == tier then
		font_size_S = 24
		font_size_M = 28
		font_size = 32
		font_size_L = 34
		font_size_XL = 36
		font_size_max = 48
	else
		font_size_S = 18
		font_size_M = 20
		font_size = 24
		font_size_L = 26
		font_size_XL = 28
		font_size_max = 42
	end
end


-- @brief 拷贝CSS
-- @param [in]	dst[table]		目标CSS
-- @param [in]	src[table]		源CSS
-- @return dst
local function CopyCss(dst, src)
	if 'table' ~= type(src) then
		return dst
	end

	for k, v in pairs(src) do
		if 'table' == type(v) then
			if 'table' == type(dst[k]) then
				CopyCss(dst[k], v)
			else
				dst[k] = CopyCss({}, v)
			end
		elseif 'number' == type(v) or 'string' == type(v) or 'boolean' == type(v) then
			dst[k] = v
		else

		end
	end

	return dst
end


-- @brief 加载css文件
local function LoadCssFile(path)
	print('Lua. LoadCssFile, path = ', path)

	local success, result = pcall(dofile, path)		-- 保护模式运行
	if not success then
		print('Lua. LoadCssFile error! path = ', path)
		return
	end

	if 'table' ~= type(result) then
		return
	end

	local default_t = ('table' == type(result['default'])) and result['default'] or {}
	local global_t = ('table' == type(result['global'])) and result['global'] or {}

	-- 拷贝
	CopyCss(defaultCssCur, default_t)
	CopyCss(globalCssCur, global_t)
end


-- @brief 在指定路径中寻找 '*.lua' 文件, 并加载
-- @param [in]	path[string]		[必须]路径: eg. './'
-- @return 无
local function FindCssFile(path, css_files)
	for file in lfs.dir(path) do
		if file ~= '.' and file ~= '..' then
			local fullpath = table.concat({path, '/', file})	-- path .. '/' .. file

			local attr = lfs.attributes(fullpath)
			if attr.mode == 'file' then				-- 文件
				if pathex.ext(fullpath) == 'lua' then
					-- 文件命名格式形如: '*.lua'
					table.insert(css_files, fullpath)
				end
			end
		end
	end
end


-- @brief 合并生成当前CSS
-- @param [in]	src[table]			来源src的 css
-- @return [table]
function css_loader.css(src)
	local css = {}

	-- 先拷贝 全局CSS
	-- 若有同字段, 则覆盖
	CopyCss(css, allCssCur)			-- 拷贝全局CSS

	-- 再拷贝源CSS, 合并之后得到当前CSS
	CopyCss(css, src)				-- 拷贝来源

	return css
end


-- @brief 按'class'名称, 获取对应的CSS集合
-- @param [in]	class_name[string]	'class'名称, eg. 'home.p2p.kstatic.offline'
-- @param [in]	css_src[table]		[可选]来源src的 源css, eg. {}
-- @return [table]
function css_loader.css_by_class(class_name, css_src)
	local css = {}

	-- 获取当前生效的所有 CSS
	local c = css_loader.css(css_src)
	local css_class = c['class'] or {}

	if css_class[class_name] then
		CopyCss(css, css_class[class_name])
	end

	return css
end


-- @brief 按'type'名称, 获取对应的CSS集合
-- @param [in]	type_name[string]	'type'名称, eg. 'kbutton'
-- @param [in]	css_src[table]		[可选]来源src的 源css, eg. {}
-- @return [table]
function css_loader.css_by_type(type_name, css_src)
	local css = {}

	-- 获取当前生效的所有 CSS
	local c = css_loader.css(css_src)
	local css_type = c['type'] or {}

	if css_type[type_name] then
		CopyCss(css, css_type[type_name])
	end

	return css
end


-- @brief 生效全局CSS
-- @return 无
function css_loader.apply()
	-- 支持全局属性
	allCssCur = {}
	allCssCur['type'] = {}

	-- 全局 'type' 属性
	if 'table' == type(globalCssCur['type']) then
		for k, v in pairs(globalCssCur['type']) do
			-- 取当前 全局CSS
			local css = {}
			CopyCss(css, defaultCssCur)	-- 默认设置
			CopyCss(css, v)				-- 全局设置

			if klbui.has_global_css(k) then
				local unset = klbui.global_css(k, css)	-- 支持全局属性的控件, 直接设置属性
				allCssCur['type'][k] = unset			-- 若未完全设置成功, 则将未设置成功的属性保留下来, 后续再设置
			else
				allCssCur['type'][k] = css				-- 不支持全局属性的控件,
			end
		end
	end

	-- 全局 'shwnd', 全局共享窗口CSS样式, eg. combo弹出菜单等
	if 'table' == type(globalCssCur['shwnd']) then
		for k, v in pairs(globalCssCur['shwnd']) do
			klbui.shwnd_css(k, v)
		end
	end

	-- 其他非 'type'/'shwnd' 全局属性
	for k, v in pairs(globalCssCur) do
		if 'type' ~= k and 'shwnd' ~= k then
			allCssCur[k] = v
		end
	end
end


-- @brief 加载 CSS (须先 configure css_dirs)
-- @param [in]	font_str[string]		[可选]字体大小字符串, 取值:['S', 'M', 'L'];  eg. 'M'
-- @return 无
function css_loader.load_css(font_str)
	-- step1. 清空变量值
	defaultCssCur = {}
	globalCssCur = {}
	allCssCur = {}

	-- step2. 初始化 字号字符串, 为CSS提供基础字号准则
	local real_font_str = font_size_str
	if 'string' == type(font_str) then
		real_font_str = font_str
	end

	if 'S' == real_font_str then
		font_size_str = 'S'
	elseif 'L' == real_font_str then
		font_size_str = 'L'
	else
		font_size_str = 'M'
	end

	ApplyFontTier(font_size_str)

	-- step3. 依次从 configure 指定的路径列表中读取 css 定义文件
	-- 并将结果 用 "拷贝 + 覆盖" 的方式, 存储于 defaultCssCur / globalCssCur
	print('Lua. load_css')

	local css_files = {}
	for i = 1, #css_dirs do
		FindCssFile(css_dirs[i], css_files)
	end

	-- Fixed Bug. [2023] 在 arm 下, 读取的文件列表为乱序的, 这里需要 排序!!!
	-- 将文件名排序, 再依次加载
	table.sort(css_files)

	for i = 1, #css_files do
		LoadCssFile(css_files[i])
	end
end


-- @brief 当前 S000 default 基线 (须在 load_css 之后)
-- @return css[table]
function css_loader.default()
	local css = {}

	CopyCss(css, defaultCssCur)
	return css
end


-- @brief 字号字符串常量定义
css_loader.FONT_SIZE_S = 'S'
css_loader.FONT_SIZE_M = 'M'
css_loader.FONT_SIZE_L = 'L'


-- @brief 字号字符串
function css_loader.font_size_str()
	return font_size_str
end


-- @brief 字号 --
function css_loader.font_size_S()
	return font_size_S
end


-- @brief 字号 -
function css_loader.font_size_M()
	return font_size_M
end


-- @brief 当前标准字号
-- @note 页面需要使用非标准字号时, 必须通过此处定义的字号 加或减
function css_loader.font_size()
	return font_size
end


-- @brief 字号 +
function css_loader.font_size_L()
	return font_size_L
end


-- @brief 字号 ++
function css_loader.font_size_XL()
	return font_size_XL
end


-- @brief 字号 max
function css_loader.font_size_max()
	return font_size_max
end


-- @brief 配置 CSS 扫描目录 (可直接扫描的路径)
-- @param [in] opts[table]	键: css_dirs
-- @return css_loader
function css_loader.configure(opts)
	if 'table' ~= type(opts) then
		return css_loader
	end

	if 'table' == type(opts.css_dirs) then
		css_dirs = tableex.copy(opts.css_dirs)
		for i = 1, #css_dirs do
			css_dirs[i] = pathex.trim(css_dirs[i])
		end
	end

	return css_loader
end


-- @brief 追加 CSS 检索目录 (可直接扫描的路径)
-- @param [in] path[string]	[必须]路径
-- @return css_loader
function css_loader.append_path(path)
	if 'string' ~= type(path) then
		return css_loader
	end

	css_dirs[#css_dirs + 1] = pathex.trim(path)

	return css_loader
end


return css_loader
