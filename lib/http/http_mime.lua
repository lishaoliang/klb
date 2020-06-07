local stringex = require("base.stringex")

local my_mime = {
	avi = 'video/avi',
	apk = 'application/vnd.android.package-archive',	
	asp = 'text/asp',
	awf = 'application/vnd.adobe.workflow',
	
	bmp = 'application/x-bmp',
	
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
	m4e = 'video/mpeg4',
	mp1 = 'audio/mp1',
	mp2v = 'video/mpeg',
	mp4 = 'video/mpeg4',
	mpeg = 'video/mpg',
	mtx = 'text/xml',
	m1v = 'video/x-mpeg',
	mml = 'text/xml',
	mp2 = 'audio/mp2',
	mp3 = 'audio/mp3',
	mpa = 'video/x-mpg',
	mpe = 'video/x-mpeg',
	mpg = 'video/mpg',
	md = 'text/x-markdown',

	pdf = 'application/pdf',
	png = 'image/png',
	plg = 'text/html',
	ppt = 'application/x-ppt',
	
	rmvb = 'application/vnd.rn-realmedia-vbr',
	
	sdp = 'application/sdp',
	svg = 'text/xml',
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

	xls = 'application/x-xls',
	xml = 'text/xml',
	xsl = 'text/xml',
	xwd = 'application/x-xwd'
}

-- default = 'application/octet-stream'

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
