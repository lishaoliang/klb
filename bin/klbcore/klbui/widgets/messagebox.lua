--[[
-- Copyright (c) 2023, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file  messagebox.lua
-- @brief 参考 messagebox
-- @note 
--]]
local klbui = require("klbcore.klbui")

local messagebox = {}

local jq = function () end

messagebox.css = {
	['type'] = {
		['kbutton'] = {
			['padding-top'] = 1,
			['padding-right'] = 2,
			['padding-bottom'] = 2,
			['padding-left'] = 10,
			
			['color:focus'] = {255, 10, 220, 220},
			['border-width:focus'] = {2, 2, 2, 2},
			['border-color:focus'] = {255,220,30,220},
		}
	},	
}

messagebox.dialog = {
	['path'] = '/messagebox',
	['type'] = 'kmessagebox',
	['pos'] = {200, 200, 480, 240},
	['title'] = 'messagebox',
	['name'] = 'messagebox1',
	
	['child'] = {
		{
			['path'] = '/messagebox/btn-ok',
			['type'] = 'kbutton',
			['pos'] = {10, 40 + 10, 140, 32},
			['title'] = 'Ok',
			['name'] = 'btn-ok',
		},
		
		{
			['path'] = '/messagebox/btn-cancel',
			['type'] = 'kbutton',
			['pos'] = {10, 40 + 50, 140, 32},
			['title'] = 'Cancel',
			['name'] = 'btn-cancel',
		},
	}
}


jq = klbui.select(messagebox.dialog)	-- 单选


messagebox.commonds = {
	['/messagebox/btn-ok'] = {
		['click'] = function ()
			klbui.messagebox_end()
		end
	},
	
	['/messagebox/btn-cancel'] = {
		['click'] = function ()
			klbui.messagebox_end()
		end
	}
}

return messagebox
