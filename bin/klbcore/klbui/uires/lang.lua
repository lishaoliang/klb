--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   lang.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  语言, language; 语言一般由 *.xls 文件自动生成 *.lua, 放入 lang 目录, 由 load_language 扫描加载
-- @note   启动前须 res.configure({ lang_dirs = {...} })
-- @history 修改历史
--  \n [2026] 创建文件
--]]
local lfs = require("lfs")
local pathex = require("klbcore.util.pathex")
local tableex = require("klbcore.util.tableex")


local lang = {}


local lang_dirs = {}		-- 语言包检索目录 (可直接 lfs 扫描的路径)

local CurLang = {}			-- 当前生效语言
local CurLangStr = ''		-- 当前生效语言key
local CurLangName = ''		-- 当前生效语言 本地名称

local CurLangMap = {}		-- 检索得到语言资源表(仅索引信息)
local CurDefaultLang = {}	-- 默认语言


-- @brief 获取k对应的翻译文字
-- @param [in]      k[string]	语言key; eg. 'ok'
-- @return [string] 翻译文字
local function Str(k)
	local s = CurLang[k]

	-- 1. 当前语言中寻找
	if 'string' == type(s) then
		-- 翻译中 为 空字符串 或 'xxxx' 也将使用英文替代显示
		if '' ~= s and 'xxxx' ~= s then
			return s
		end
	end

	-- 2. 在默认语言中寻找
	s = CurDefaultLang[k]
	if 'string' == type(s) then
		return s
	end

	-- 3. 都没有, 则显示 'xxxx'
	return 'xxxx'
end


-- @brief 获取k对应的翻译文字
-- @param [in]      k[string]	语言key; eg. 'ok'
-- @return [string] 翻译文字
function lang.str(k)
	return Str(k)
end


-- @brief 从路径中读取语言
local function TryLoadlanguage(path)
	local success, result = pcall(dofile, path)		-- 保护模式运行
	if not success then
		return nil, nil, nil
	end

	if 'table' ~= type(result) then
		return nil, nil, nil
	end

	local key = result[1]
	local describe = result[2]

	if 'string' == type(key) and 'string' == type(describe) then
		return result, key, describe
	end

	return nil, nil, nil
end


-- @brief 应用当前语言
function lang.apply(s)
	local key, describe, path = '', '', ''

	-- 从索引中得到文件路径
	for i = 1, #CurLangMap do
		if s == CurLangMap[i].key then
			key = CurLangMap[i].key
			describe = CurLangMap[i].describe
			path = CurLangMap[i].path
			break								-- 找到可用的
		elseif 1 == i then
			-- 默认使用第一个
			key = CurLangMap[i].key
			describe = CurLangMap[i].describe
			path = CurLangMap[i].path
		end
	end

	-- 加载语言
	if '' ~= path then
		local t = TryLoadlanguage(path)
		if t then
			CurLang = t
			CurLangStr = key
			CurLangName = describe
		end
	end
end


-- @brief 获取支持的语言列表
function lang.supports()
	local list = {}

	for i = 1, #CurLangMap do
		table.insert(list, {[CurLangMap[i].key] = CurLangMap[i].describe})
	end

	return CurLangStr, CurLangName, list
end


-- @brief 在指定路径中寻找 '*.lua' 语言文件, 并建立索引
-- @param [in]	path[string]		[必须]路径: eg. './'
local function FindLangFile(path)
	for file in lfs.dir(path) do
		if file ~= '.' and file ~= '..' then
			local fullpath = table.concat({path, '/', file})	-- path .. '/' .. file

			local attr = lfs.attributes(fullpath)
			if attr.mode == 'file' then				-- 文件
				if pathex.ext(fullpath) == 'lua' then
					-- 文件命名格式形如: '*.lua'
					local _, key, describe = TryLoadlanguage(fullpath)
					if key and describe then
						-- 插入加载记录
						table.insert(CurLangMap, {
							['key'] = key,
							['describe'] = describe,
							['path'] = fullpath,
						})
					end
				end
			end
		end
	end
end


-- @brief 加载语言信息
-- @return 无
function lang.load_language()
	CurLangMap = {}
	CurDefaultLang = {}

	-- 依次从 configure 指定的路径列表中读取语言定义文件
	for i = 1, #lang_dirs do
		FindLangFile(lang_dirs[i])
	end

	-- 加载默认语言
	if 0 < #CurLangMap then
		-- Fixed Bug. [2026] 在 arm 下, 读取的文件列表为乱序的, 这里需要 排序!!!
		-- 将语言文件 按文件名路径 排序
		table.sort(CurLangMap, function (a, b)
			return a.path < b.path
		end)

		-- 尝试加载默认语言
		CurDefaultLang = TryLoadlanguage(CurLangMap[1].path)

		-- 应用默认语言
		lang.apply(CurDefaultLang[1])
	end
end


-- @brief 配置语言包检索目录 (可直接扫描的路径)
-- @param [in] opts[table]	键: lang_dirs
-- @return lang
function lang.configure(opts)
	if 'table' ~= type(opts) then
		return lang
	end

	if 'table' == type(opts.lang_dirs) then
		lang_dirs = tableex.copy(opts.lang_dirs)
		for i = 1, #lang_dirs do
			lang_dirs[i] = pathex.trim(lang_dirs[i])
		end
	end

	return lang
end


-- @brief 追加语言包检索目录 (可直接扫描的路径)
-- @param [in] path[string]	[必须]路径
-- @return lang
function lang.append_path(path)
	if 'string' ~= type(path) then
		return lang
	end

	lang_dirs[#lang_dirs + 1] = pathex.trim(path)

	return lang
end


return lang
