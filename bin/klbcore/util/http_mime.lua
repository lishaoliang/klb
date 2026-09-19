--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   http_mime.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  按扩展名查 HTTP Content-Type
-- @note   模块导出单一函数 (非 table); 未知为 application/octet-stream
-- @history 修改历史
--  \n [2026] 创建文件
--]]
local stringex = require("klbcore.util.stringex")

local my_mime = {
	avi = 'video/x-msvideo',
	apk = 'application/vnd.android.package-archive',	
	asp = 'text/asp',
	awf = 'application/vnd.adobe.workflow',
	
	bmp = 'image/bmp',
	
	crt = 'application/x-x509-ca-cert',
	css = 'text/css',
	cml = 'text/xml',
	cmx = 'application/x-cmx',
	
	dbf = 'application/x-dbf',	
	dcd = 'text/xml',
	dcx = 'application/x-dcx',	
	doc = 'application/msword',
	drw = 'application/x-drw',
	dwg = 'application/x-dwg',
	dxf = 'application/x-dxf',
	dll = 'application/x-msdownload',
	dot = 'application/msword',
	dwf = 'application/x-dwf',
	
	emf = 'application/x-emf',	
	exe = 'application/x-msdownload',

	gif = 'image/gif',
	gl2 = 'application/x-gl2',

	hgl = 'application/x-hgl',
	html = 'text/html',
	htm = 'text/html',
	htx = 'text/html',

	icb = 'application/x-icb',
	ico = 'image/x-icon',
	img = 'application/x-img',
	iii = 'application/x-iphone',

	jpe = 'image/jpeg',	
	jpeg = 'image/jpeg',
	jpg = 'image/jpeg',
	js = 'text/javascript',	
	json = 'application/json',	-- https://tools.ietf.org/html/rfc4627
	jsp = 'text/html',
	jfif = 'image/jpeg',

	m2v = 'video/x-mpeg',
	m4e = 'video/mp4',
	mp1 = 'audio/mpeg',
	mp2v = 'video/mpeg',
	mp4 = 'video/mp4',
	mpeg = 'video/mpeg',
	mtx = 'text/xml',
	m1v = 'video/x-mpeg',
	mml = 'text/xml',
	mp2 = 'audio/mpeg',
	mp3 = 'audio/mpeg',
	mpa = 'video/x-mpg',
	mpe = 'video/x-mpeg',
	mpg = 'video/mpeg',
	md = 'text/markdown',

	pdf = 'application/pdf',
	png = 'image/png',
	plg = 'text/html',
	ppt = 'application/vnd.ms-powerpoint',
	
	rmvb = 'application/vnd.rn-realmedia-vbr',
	
	sdp = 'application/sdp',
	svg = 'image/svg+xml',
	stm = 'text/html',
	
	tif = 'image/tiff',
	tiff = 'image/tiff',
	tga = 'application/x-tga',
	torrent = 'application/x-bittorrent',
	txt = 'text/plain',
	
	uls = 'text/iuls',
	
	vml = 'text/xml',
	vsd = 'application/vnd.visio',
	vxml = 'text/xml',
	
	wav = 'audio/wav',
	wasm = 'application/wasm',

	xls = 'application/vnd.ms-excel',
	xml = 'text/xml',
	xsl = 'text/xml',
	xwd = 'application/x-xwd',

	aac = 'audio/aac',
	csv = 'text/csv',
	docx = 'application/vnd.openxmlformats-officedocument.wordprocessingml.document',
	flv = 'video/x-flv',
	gz = 'application/gzip',
	m3u8 = 'application/vnd.apple.mpegurl',
	m4a = 'audio/mp4',
	mjs = 'text/javascript',
	ogg = 'audio/ogg',
	otf = 'font/otf',
	pptx = 'application/vnd.openxmlformats-officedocument.presentationml.presentation',
	ts = 'video/mp2t',
	ttf = 'font/ttf',
	webm = 'video/webm',
	webp = 'image/webp',
	woff = 'font/woff',
	woff2 = 'font/woff2',
	xlsx = 'application/vnd.openxmlformats-officedocument.spreadsheetml.sheet',
	zip = 'application/zip',
	['7z'] = 'application/x-7z-compressed'
}

-- default = 'application/octet-stream'

-- @brief 由文件名扩展名查 MIME
-- @param [in]	filename[string]		文件名或路径; eg. 'index.html'
-- @return [string]		MIME 类型; 未知为 'application/octet-stream'
local http_mime = function (filename)
	local ext = string.match(filename, '[^.]*$')
	
	if nil ~= ext then
		ext = stringex.trim(ext)
		ext = string.lower(ext)
		
		local mime = my_mime[ext]
		if nil ~= mime then
			return mime
		end
	end
	
	return 'application/octet-stream' -- 二进制流,未知类型
end

return http_mime
