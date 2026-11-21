--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   init.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  klbonvif init.lua
-- @note   ONVIF 协议脚本封装; Discovery + 客户端/服务端 Device/Media 主路径
--   \n C `konvif.connect` / `konvif.listen` 为纯桩 (NOTSUP); 请用本模块 new_client / new_listen
--   \n 服务端现行不鉴权 (见 soap_serve)
--   \n 返回码: 直接用 klbcore.klbcode.base / net; 不本地再映射 E_*
-- @history 修改历史
--		[2026-09] 创建文件
--]]
local onvifclienter = require("klbcore.klbonvif.client.onvifclienter")
local onviflistener = require("klbcore.klbonvif.serve.onviflistener")
local onvifdiscoverer = require("klbcore.klbonvif.discovery.onvifdiscoverer")
local onvifdiscover_listen = require("klbcore.klbonvif.discovery.onvifdiscover_listen")


local klbonvif = {}


--------------------------------------------------------------------------------------------
-- Discovery


-- @brief 新建 WS-Discovery 搜索模块
-- @param [in]      cfg[table]		[可选] 默认配置
-- @return [table]	搜索模块
-- @note 使用: open / co_search(opts); 须在 kco 协程内
klbonvif.new_discover = function (cfg)
	return onvifdiscoverer.new(cfg)
end


-- @brief 新建 WS-Discovery 服务模块 (应答 Probe)
-- @param [in]      cfg[table]		须含 xaddr; 可选 types/scopes/endpoint
-- @return [table]	服务模块
-- @note 使用: open([port]) / co_once(); 须在 kco 协程内
klbonvif.new_discover_listen = function (cfg)
	return onvifdiscover_listen.new(cfg)
end


--------------------------------------------------------------------------------------------
-- 客户端


-- @brief 新建一个 ONVIF 客户端
-- @param [in]      cfg[table]		客户端配置
-- @return [table]	客户端模块
-- @note connect / call / co_get_* ; 须在 kco 协程内
klbonvif.new_client = function (cfg)
	return onvifclienter.new(cfg)
end


--------------------------------------------------------------------------------------------
-- 服务端


-- @brief 新建一个 ONVIF 监听模块
-- @param [in]      cfg[table]		监听模块配置
-- @return [table]	监听模块
-- @note open / co_accept -> session:co_once; 须在 kco 协程内
klbonvif.new_listen = function (cfg)
	return onviflistener.new(cfg)
end


return klbonvif
