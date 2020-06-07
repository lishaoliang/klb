--[[
-- Copyright(c) 2020, LGPL All Rights Reserved
-- @brief lsm rtsp
-- @author 李绍良
--]]
local cjson = require("cjson.safe")
local kg_lsmfw = require("kg_lsmfw")

local printex = require("base.printex")
local rtsp_serve = require("rtsp.rtsp_serve")
local rtsp_cmd = require("rtsp.rtsp_cmd")


local lsm_rtsp = {}




local pack_sdp_tcp = function (base_path, sps, pps)
	local video_url = table.concat{base_path, '/video'}	-- '/video'

	local t = {}
	table.insert(t, 'v=0\r\n')
	table.insert(t, 'a=range:npt=0-\r\n')
	
	-- video
	table.insert(t, 'm=video 0 RTP/AVP/TCP 96\r\n')
	table.insert(t, 'a=rtpmap:96 H264/90000\r\n')
	
	-- 'a=fmtp:96 packetization-mode=1;sprop-parameter-sets=Z0IAKpY1QPAET8s3AQEBAg==,aM48gA==\r\n'
	if 0 < string.len(sps) and 0 < string.len(pps) then
		table.insert(t, string.format('a=fmtp:96 packetization-mode=1;sprop-parameter-sets=%s,%s\r\n', sps, pps))
	else
		table.insert(t, 'a=fmtp:96 packetization-mode=1\r\n')
	end	

	table.insert(t, string.format('a=control:%s\r\n', video_url))
	
	return table.concat(t)
end


lsm_rtsp.on_newconn = function (name, protocol, ok, err)
	--print('lsm_rtsp.on_newconn', name, protocol, ok, err)
	return 0
end

lsm_rtsp.on_receiver = function (name, protocol, sequence, uid, head, body)
	--print('lsm_rtsp.on_receiver', name, protocol, sequence, uid, head, body)
	
	local req = rtsp_serve.parse(head, body)
	--printex(name, protocol, req)

--rtsp_cmd.OPTIONS = 'OPTIONS'
--rtsp_cmd.DESCRIBE = 'DESCRIBE'
--rtsp_cmd.SETUP = 'SETUP'
--rtsp_cmd.TEARDOWN = 'TEARDOWN'
--rtsp_cmd.PLAY = 'PLAY'
--rtsp_cmd.PAUSE = 'PAUSE'
--rtsp_cmd.GET_PARAMETER = 'GET_PARAMETER'

	local res_head = ''
	local res_body = ''
	
	local cseq = req['cseq'] or 0
	local method = req['method'] or ''
	local base_path = req['url'] or '/111'
	local session = '123456'
	
	if rtsp_cmd.OPTIONS == method then
		res_head = rtsp_cmd.pack_res_options(cseq)
	elseif rtsp_cmd.DESCRIBE == method then
		local sdp = pack_sdp_tcp(base_path, '', '')
		res_head = rtsp_cmd.pack_res_describe(cseq, base_path, sdp)
		res_body = sdp
	elseif rtsp_cmd.SETUP == method then
		res_head = rtsp_cmd.pack_res_setup_tcp(cseq, session)
	elseif rtsp_cmd.PLAY == method then
		res_head = rtsp_cmd.pack_res_play(cseq, session)		
		kg_lsmfw.open_stream(name, protocol, 0, 1)
	elseif rtsp_cmd.PAUSE == method then
		res_head = rtsp_cmd.pack_res_pause(cseq, session)
	elseif rtsp_cmd.GET_PARAMETER == method then
		res_head = rtsp_cmd.pack_res_get_parameter(cseq, session)
	elseif rtsp_cmd.TEARDOWN == method then
		res_head = rtsp_cmd.pack_res_teardown(cseq, session)
	end

	kg_lsmfw.send_txt(name, protocol, sequence, uid, res_head, res_body)
	
	return 0
end


lsm_rtsp.on_disconnect = function (name, protocol, err)
	--print('lsm_rtsp.on_disconnect', name, protocol, err)
	
	return 0
end

return lsm_rtsp
