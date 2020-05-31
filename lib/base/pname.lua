
-- from: klb/src/knet/knet.go

local pname = {}

pname.UNKOWN = 'UNKOWN'		-- unkown
pname.MNP = 'MNP'			-- mnp Э��
pname.MNPS = 'MNPS'			-- mnp Э��: TLS
pname.RTMP = 'RTMP'			-- rtmp Э��
pname.RTSP = 'RTSP'			-- rtsp Э��
pname.HTTP = 'HTTP'			-- http Э��
pname.HTTPS = 'HTTPS'		-- https Э��: TLS
pname.HTTPMNP = 'HTTP-MNP'	-- http mnp Э��
pname.HTTPFLV = 'HTTP-FLV'	-- http flv Э��
pname.WS = 'WS'				-- websocket Э��
pname.WSS = 'WSS'			-- websocket Э��: TLS
pname.WSMNP = 'WS-MNP'		-- websocket mnp Э��
pname.WSFLV = 'WS-FLV'		-- websocket flv Э��

return pname
