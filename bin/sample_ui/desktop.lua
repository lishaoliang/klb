local klbui = require("klbcore.klbui")


local M = {}



local css = {
	['type'] = {
		['kbutton'] = {
			['border-color'] = {255,120,120,120},
			['border-color:focus'] = {255,180,180,180},
		},
	},
}


local dialog = {
	['type'] = 'kview',
	['pos'] = {0, 0, -1, -1},
	['name'] = 'desktop',
	
	['child'] = {
		{
			['type'] = 'kbutton',
			['pos'] = {100, 100, 120, 32},
			['name'] = 'btn1',
			
			['title'] = '按钮1',
		}
	}
}


local function OnLoad()
	print('Hello, klbui. desktop.OnLoad!')
end


local function OnUnload()
	print('Hello, klbui. desktop.OnUnload!')
end


local commands = {
	['desktop'] = {
		['onload'] = OnLoad,
		['onunload'] = OnUnload,
	},
	
	['btn1'] = {
		['click'] = function ()
			print('btn1, click!')
		end
	}
}


--------------------------------------------------------------------------------------
-- export

M.dialog = dialog
M.commands = commands
M.css = css

return M
