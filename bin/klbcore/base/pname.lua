
-- from: klb/src/knet/knet.go

local pname = {}

pname.UNKOWN = 'UNKOWN'		-- unkown
pname.MNP = 'MNP'			-- mnp 协议
pname.MNPS = 'MNPS'			-- mnp 协议: TLS
pname.RTMP = 'RTMP'			-- rtmp 协议
pname.RTSP = 'RTSP'			-- rtsp 协议
pname.HTTP = 'HTTP'			-- http 协议
pname.HTTPS = 'HTTPS'		-- https 协议: TLS
pname.HTTPMNP = 'HTTP-MNP'	-- http mnp 协议
pname.HTTPFLV = 'HTTP-FLV'	-- http flv 协议
pname.WS = 'WS'				-- websocket 协议
pname.WSS = 'WSS'			-- websocket 协议: TLS
pname.WSMNP = 'WS-MNP'		-- websocket mnp 协议
pname.WSFLV = 'WS-FLV'		-- websocket flv 协议

return pname
