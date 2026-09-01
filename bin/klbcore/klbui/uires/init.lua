--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbui 资源层门面 (configure / lang / image / font / css)
-- @note   产品启动前调用 configure 注入 lang/css/image/font 检索目录
-- @history 修改历史
--  \n [2026] 创建文件
--]]
local lang = require("klbcore.klbui.uires.lang")
local load_images = require("klbcore.klbui.uires.load_images")
local css_loader = require("klbcore.klbui.uires.css_loader")
local search_font = require("klbcore.klbui.uires.search_font")


local res = {}

--------------------------------------------------------------

-- @brief 配置资源检索路径 (lang/image/font/css 为可直接扫描路径)
-- @param [in] opts[table]	键: lang_dirs / image_dirs / font_search_dirs / css_dirs
-- @return res
function res.configure(opts)
	if 'table' ~= type(opts) then
		return res
	end

	lang.configure(opts)
	load_images.configure(opts)
	css_loader.configure(opts)
	search_font.configure(opts)

	return res
end


--------------------------------------------------------------
-- 字体检索

-- @brief 追加字体检索目录 (与 configure.font_search_dirs 一致, 可直接 lfs 扫描)
-- @param [in]	dir[string]		[必须]检索目录
-- @return res
function res.append_font_search_dir(dir)
	search_font.append_path(dir)

	return res
end

-- @brief 检索 ttf 字库资源路径
-- @return [string]	ttf字体路径
function res.search_font()
	return search_font.find()
end


---------------------------------------------------------------
-- 图片

-- @brief 追加图片检索目录 (与 configure.image_dirs 一致, 可直接 lfs 扫描)
-- @param [in]	dir[string]		[必须]检索目录
-- @return res
function res.append_image_search_dir(dir)
	load_images.append_path(dir)

	return res
end

-- @brief 加载所有图片资源 (须先 configure.image_dirs)
-- @return 无
function res.load_images()
	load_images.load()
end


-- @brief 按绝对路径尝试加载单张图片
-- @param [in]	key[string]		图片资源 key (以 '/' 开头, 无扩展名); eg. '/aaa/audio'
-- @param [in]	fullpath[string]	图片文件全路径
-- @return 无
function res.try_load_image(key, fullpath)
	load_images.try_load_path(key, fullpath)
end


---------------------------------------------------------------
-- 语言

-- @brief 追加语言检索目录 (与 configure.lang_dirs 一致, 可直接 lfs 扫描)
-- @param [in]	dir[string]		[必须]检索目录
-- @return res
function res.append_lang_search_dir(dir)
	lang.append_path(dir)

	return res
end

-- @brief 加载语言信息 (须先 configure.lang_dirs)
-- @return 无
function res.load_language()
	lang.load_language()
end


-- @brief 获取 k 对应的翻译文字
-- @param [in]	k[string]	语言 key; eg. 'ok'
-- @return [string]	翻译文字
function res.str(k)
	return lang.str(k)
end


-- @brief 应用当前语言
-- @param [in]	s[string]	语言 key
-- @return 无
function res.apply_lang(s)
	lang.apply(s)
end


-- @brief 获取支持的语言列表
-- @return [string]	当前语言 key
-- @return [string]	当前语言本地名称
-- @return [table]	语言列表 { [{key]=describe}, ... }
function res.lang_supports()
	return lang.supports()
end


---------------------------------------------------------------
-- CSS

-- @brief 追加 CSS 检索目录 (与 configure.css_dirs 一致, 可直接 lfs 扫描)
-- @param [in]	dir[string]		[必须]检索目录
-- @return res
function res.append_css_search_dir(dir)
	css_loader.append_path(dir)

	return res
end


-- @brief 加载 CSS (须先 configure.css_dirs)
-- @param [in]	font_str[string]	[可选]字号档位, 取值: ['S', 'M', 'L']; eg. 'M'
-- @return 无
function res.load_css(font_str)
	css_loader.load_css(font_str)
end


-- @brief 生效全局 CSS (须在 load_css 之后)
-- @return 无
function res.apply_css()
	css_loader.apply()
end


-- @brief 合并全局 CSS 与来源 CSS
-- @param [in]	src[table]	[可选]来源 css
-- @return [table]	合并后的 css
function res.css(src)
	return css_loader.css(src)
end


-- @brief 按 class 名称获取 CSS 集合
-- @param [in]	class_name[string]	class 名称
-- @param [in]	css_src[table]		[可选]来源 css
-- @return [table]
function res.css_by_class(class_name, css_src)
	return css_loader.css_by_class(class_name, css_src)
end


-- @brief 按 type 名称获取 CSS 集合
-- @param [in]	type_name[string]	type 名称; eg. 'kbutton'
-- @param [in]	css_src[table]		[可选]来源 css
-- @return [table]
function res.css_by_type(type_name, css_src)
	return css_loader.css_by_type(type_name, css_src)
end


-- @brief 当前标准字号
-- @return [number]
function res.font_size()
	return css_loader.font_size()
end


-- @brief 当前字号档位字符串
-- @return [string]	取值: 'S' / 'M' / 'L'
function res.font_size_str()
	return css_loader.font_size_str()
end


res.FONT_SIZE_S = css_loader.FONT_SIZE_S
res.FONT_SIZE_M = css_loader.FONT_SIZE_M
res.FONT_SIZE_L = css_loader.FONT_SIZE_L


return res
