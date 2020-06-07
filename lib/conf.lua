-- Copyright(c) 2020, LGPLV3
-- https://github.com/lishaoliang/klb

local conf = {
	port = 8080,
	
	tls_port = 8443,
	tls_cert = '/usr/local/lib/klb1.0/tls/server.pem',
	tls_key = '/usr/local/lib/klb1.0/tls/server.key',
	
	path_html = '/usr/local/lib/klb1.0/html',	
	path_media = '/usr/local/lib/klb1.0/media'
}


local test = false
if test then
	conf = {
		port = 8080,					-- 非TLS监听端口
		
		tls_port = 8443,				-- TLS监听端口
		tls_cert = './tls/server.pem',	-- TLS openssl秘钥
		tls_key = './tls/server.key',	-- TLS openssl秘钥
		
		path_html = './html',			-- 提供静态网页路径
		path_media = './media'			-- 测试使用的模拟数据: 暂时只支持flv(H264)格式
	}
end


return conf
