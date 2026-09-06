--[[
-- Copyright (c) 2026, GNU LESSER GENERAL PUBLIC LICENSE Version 3, 29 June 2007
-- @file   flexer.lua
-- @author 随风(https://gitee.com/klua/klb)
-- @brief  自动布局器
--	flex 自动布局设计: (参考C代码定义: klb\inc\klbgui\klb_wnd_flex.h; 参考C代码实现: klb\src_c\klbgui\extensions\klbuiex_flex.h/c)
--		1. L1 -> L2 -> L3 多层级布局; 
--		2. L1 层级: klb_wnd_pos_t.layout_algo = klb_wnd_flex_algo_e -> 布局引擎
--		3. L2 层级: tag( PX/FLOW )
--		4. L3 层级: payload( px/flow )
--
--	初始由 parser 解析窗口时, 完成 flex 布局引擎 设置; 
--
--	解析高阶 pos 字符串; 参考web解析
--		eg. 格式1: 'flow-row : 10, 10px, auto, 100px'
--		eg. 格式2: 'flow-row { 10, 10px, auto, 100px }'
--		eg. 格式3: '10, 10px, auto, 100px'  默认: 'flow-column'
--		eg. 格式4: '{ 10, 10px, auto, 100px }'  默认: 'flow-column'
--		eg. 格式5: 'fixed; inset 0; z-index 999'
--		eg. 格式6: 'fixed; flow-row : 10, 10px, auto, 100px; z-index 999'
--
--	pos 公式 = '段1 ; 段2 ; 段3 ; ...'
--		段 = { fixed|absolute|static, inset..., top|left|width|height..., z-index N, flow-row : x,y,w,h }
--		上面段的示例 仅参考, 表示参考格式
--
--	pos 段解析: NewPosCtx -> SplitPosSegments(';') ->  ParsePosSegment ->  MergePosCtx
--
-- @history 修改历史
--   \n 2026 0.1 创建文件
--]]

local flexer = {}
local E = {}


-- KLB_WND_STYLE_FLEX 样式标记
-- 参见: klb\inc\klbgui\klb_wnd_style.h
local CONST_flex_style = 0x8000

-- klb_wnd_flex_algo_e : L1 层级: 自动布局引擎
-- 参见: klb\inc\klbgui\klb_wnd_flex.h
local CONST_flex_algo_none = 0 -- 无布局引擎
local CONST_flex_algo_flow_column = 1 -- 纵向布局引擎
local CONST_flex_algo_flow_row = 2 -- 横向布局引擎
local CONST_flex_algo_flow_form = 3 -- 表单布局引擎
local CONST_flex_algo_flow_grid = 4 -- 网格布局引擎

-- klb_wnd_flex_tag_e : L2 层级(tag): 分量类型
-- 参见: klb\inc\klbgui\klb_wnd_flex.h
local CONST_flex_tag_shift = 24
local CONST_flex_payload_mask = 0x00FFFFFF

-- klb_wnd_flex_tag_e : L2 层级(tag): 分量类型
-- 参见: klb\inc\klbgui\klb_wnd_flex.h
local CONST_flex_tag_px = 1
local CONST_flex_tag_flow = 2

-- klb_wnd_flex_flow_e : L3 层级(payload): 流式/弹性策略
-- 参见: klb\inc\klbgui\klb_wnd_flex.h
local CONST_flex_flow_auto = 1
local CONST_flex_flow_grow = 2
local CONST_flex_flow_fill = 3
local CONST_flex_flow_stretch = 4
local CONST_flex_flow_inherit = 5

-- flex 布局引擎字符串关键字
local CONST_flex_algo_str = {
	['flow-row'] = CONST_flex_algo_flow_row,
	['flow-column'] = CONST_flex_algo_flow_column,
	['flow-form'] = CONST_flex_algo_flow_form,
	['flow-grid'] = CONST_flex_algo_flow_grid,
}

-- flex flow 字符串关键字
local CONST_flex_flow_str = {
	['auto'] = CONST_flex_flow_auto,
	['grow'] = CONST_flex_flow_grow,
	['fill'] = CONST_flex_flow_fill,
	['stretch'] = CONST_flex_flow_stretch,
	['inherit'] = CONST_flex_flow_inherit,
}

--------------------------------------------------------------------------------------
-- 未启用flex

-- 自动对齐区域(x, y, w, h)
-- -1: 将需要自动对齐父窗口的宽高
local function AutoRect(parent_path, x, y, w, h)
	local auto_x, auto_y, auto_w, auto_h = x, y, w, h
	
	-- do. 1. 不需要处理
	if 0 <= auto_x and 0 <= auto_y and 0 <= auto_w and 0 <= auto_h then
		return auto_x, auto_y, auto_w, auto_h
	end
	
	-- 屏幕宽/高
	local sceen_w, sceen_h = kgui.wh()
	
	-- 父窗口宽/高
	local parent_rect = {}
	if parent_path then
		parent_rect = kgui.wndpos(parent_path, false)
	end
	
	local parent_w = parent_rect['w'] or sceen_w
	local parent_h = parent_rect['h'] or sceen_h
	
	-- do. 2 自动宽
	if 0 <= x and auto_w < 0 then
		auto_w = parent_w - x
		
		if auto_w < 0 then
			auto_w = 0
		end
	end
	
	-- do. 3. 自动高
	if 0 <= y and auto_h < 0 then
		auto_h = parent_h - y
		
		if auto_h < 0 then
			auto_h = 0
		end
	end
	
	-- do. 4. 自动x, 向右对齐
	if x < 0 and auto_w >= 0 then
		auto_x = parent_w - auto_w
		
		if auto_x < 0 then
			auto_x = 0
		end
	end
	
	-- do. 5. 自动y, 向下对齐
	if y < 0 and auto_h >= 0 then
		auto_y = parent_h - auto_h
		
		if auto_y < 0 then
			auto_y = 0
		end
	end
	
	return auto_x, auto_y, auto_w, auto_h
end

--------------------------------------------------------------------------------------
-- 启用flex

-- 编码 flex px 分量 (KLB_WND_FLEX_TAG_PX)
local function EncodeFlexPx(px)
	local payload = px % CONST_flex_payload_mask

	if px < 0 then
		payload = (0x1000000 + payload) % 0x1000000
	end

	return CONST_flex_tag_px * (2 ^ CONST_flex_tag_shift) + payload
end


-- 编码 flex flow 分量 (KLB_WND_FLEX_TAG_FLOW)
local function EncodeFlexFlow(flow)
	return CONST_flex_tag_flow * (2 ^ CONST_flex_tag_shift) + (flow % CONST_flex_payload_mask)
end


-- 去除前后空白
local function TrimString(s)
	return (string.gsub(s, '^%s*(.-)%s*$', '%1'))
end

-- 判定 pos 是否未设置 (未填 'pos' 或空表)
local function IsPosUnset(pos)
	if nil == pos then
		return true
	end

	if 'table' ~= type(pos) then
		return false
	end

	if nil == pos[1] and nil == pos[2] and nil == pos[3] and nil == pos[4] then
		return true
	end

	return false
end


-- 判定 pos 是否为纯数值坐标表 {x, y, w, h}
local function IsNumericPos(pos)
	if 'table' ~= type(pos) then
		return false
	end

	local x, y, w, h = pos[1], pos[2], pos[3], pos[4]

	if 'number' == type(x) and 'number' == type(y) and 'number' == type(w) and 'number' == type(h) then
		return true
	end

	return false
end


-- 判定 pos 是否启用 flex 自动布局
-- 未设置 pos -> false (原坐标路径)
-- pos 为 string -> 由 ParsePosDeclString 判定 (此处返回 false)
-- 仅当 pos = {x, y, w, h} 且四项均为数值时返回 false; 否则为 flex
local function IsFlexPos(pos)
	if IsPosUnset(pos) then
		return false
	end

	if 'string' == type(pos) then
		return false
	end

	if IsNumericPos(pos) then
		return false
	end

	return true
end


-- 解析 flex 单分量字面量 (eg. '10px', 'auto', '-1')
local function ParsePosDimToken(token)
	local s = TrimString(token)

	if '' == s then
		return 0
	end

	if CONST_flex_flow_str[s] then
		return s
	end

	local px = string.match(s, '^(%-?%d+)px$')

	if px then
		return tonumber(px)
	end

	local n = tonumber(s)

	if n then
		return n
	end

	return s
end


-- 解析高阶 pos 字符串维度列表 (逗号分隔; 分隔符两侧允许空格)
local function ParsePosDims(dims_str)
	local dims = {}
	local i = 0

	for token in string.gmatch(dims_str, '%s*([^,]+)') do
		i = i + 1
		dims[i] = ParsePosDimToken(token)
	end

	return dims[1] or 0, dims[2] or 0, dims[3] or 0, dims[4] or 0
end


-- 去掉维度串外层花括号 (允许 '{ ... }' 两侧及内部首尾空格)
local function StripPosDimsBrace(dims_str)
	local s = TrimString(dims_str)
	local inner = string.match(s, '^%s*{%s*(.-)%s*}%s*$')

	if inner then
		return TrimString(inner)
	end

	return s
end


-- flex 定位模式关键字
local CONST_position_str = {
	['fixed'] = true,
	['absolute'] = true,
	['static'] = true,
}

-- flex 盒模型边关键字 -> ctx 字段
local CONST_box_key_str = {
	['top'] = 'top',
	['right'] = 'right',
	['bottom'] = 'bottom',
	['left'] = 'left',
	['width'] = 'width',
	['height'] = 'height',
	['w'] = 'width',
	['h'] = 'height',
	['x'] = 'left',
	['y'] = 'top',
}


-- 解析高阶 pos 单段 flex 维度 (格式1~4)
local function ParsePosFlexSegment(seg)
	local s = TrimString(seg)
	local algo_key = 'flow-column'
	local dims_str = s

	-- 格式1: 'flow-row : 10, 10px, auto, 100px'
	local algo_colon, dims_colon = string.match(s, '^%s*(%S+)%s*:%s*(.+)%s*$')

	if algo_colon and CONST_flex_algo_str[algo_colon] then
		algo_key = TrimString(algo_colon)
		dims_str = StripPosDimsBrace(dims_colon)
	else
		-- 格式2: 'flow-row { 10, 10px, auto, 100px }'
		local algo_brace, dims_brace = string.match(s, '^%s*(%S+)%s*{%s*(.-)%s*}%s*$')

		if algo_brace and CONST_flex_algo_str[algo_brace] then
			algo_key = TrimString(algo_brace)
			dims_str = TrimString(dims_brace)
		else
			-- 格式4: '{ 10, 10px, auto, 100px }'
			local dims_only_brace = string.match(s, '^%s*{%s*(.-)%s*}%s*$')

			if dims_only_brace then
				algo_key = 'flow-column'
				dims_str = TrimString(dims_only_brace)
			else
				-- 格式3: '10, 10px, auto, 100px'
				algo_key = 'flow-column'
				dims_str = s
			end
		end
	end

	local layout_algo = CONST_flex_algo_str[algo_key] or CONST_flex_algo_flow_column
	local x, y, w, h = ParsePosDims(dims_str)

	return layout_algo, x, y, w, h
end


-- 建立 pos 声明解析上下文
local function NewPosCtx()
	return {
		position = nil,
		top = nil,
		right = nil,
		bottom = nil,
		left = nil,
		width = nil,
		height = nil,
		inset = nil,
		flex_algo = nil,
		flex_x = nil,
		flex_y = nil,
		flex_w = nil,
		flex_h = nil,
		has_flex = false,
		z_index = nil,
	}
end


-- 主分隔符 ';' 分段 (段两侧 Trim; 空段丢弃)
local function SplitPosSegments(pos_str)
	local segments = {}
	local i = 0

	for seg in string.gmatch(pos_str, '([^;]+)') do
		seg = TrimString(seg)

		if '' ~= seg then
			i = i + 1
			segments[i] = seg
		end
	end

	return segments
end


-- 解析 inset 值列表 (1/2/4 值; 默认 px)
local function ParseInsetValues(s)
	local vals = {}
	local n = 0

	for token in string.gmatch(s, '%S+') do
		n = n + 1
		vals[n] = ParsePosDimToken(token)
	end

	if 1 == n then
		return { vals[1], vals[1], vals[1], vals[1] }
	end

	if 2 == n then
		return { vals[1], vals[2], vals[1], vals[2] }
	end

	return { vals[1] or 0, vals[2] or 0, vals[3] or 0, vals[4] or 0 }
end


-- 解析维度值为 number (盒模型段用)
local function ParsePosDimNumber(v)
	if 'number' == type(v) then
		return v
	end

	if 'string' == type(v) then
		local n = tonumber(v)

		if n then
			return n
		end
	end

	return 0
end


-- 解析 z-index 段
local function ParsePosSegmentZIndex(s, ctx)
	local z_val = string.match(s, '^%s*z%-index%s*:%s*(.+)%s*$')

	if not z_val then
		z_val = string.match(s, '^%s*z%-index%s+(.+)%s*$')
	end

	if not z_val then
		z_val = string.match(s, '^%s*z%s*:%s*(.+)%s*$')
	end

	if not z_val then
		z_val = string.match(s, '^%s*z%s+(.+)%s*$')
	end

	if z_val then
		ctx.z_index = tonumber(TrimString(z_val))
		return true
	end

	return false
end


-- 判定段是否为 flex 维度段
local function IsFlexSegment(seg)
	local s = TrimString(seg)
	local head, sep = string.match(s, '^%s*(%S+)%s*(.)')

	if head and CONST_flex_algo_str[head] and (':' == sep or '{' == sep) then
		return true
	end

	if string.match(s, '^%s*{') then
		return true
	end

	if string.match(s, '^%s*[%d%-]') then
		return true
	end

	local flow_head = string.match(s, '^%s*(%S+)')

	if flow_head and CONST_flex_flow_str[flow_head] then
		return true
	end

	return false
end


-- 解析单段: position / inset / box / z-index / flex
local function ParsePosSegment(seg, ctx)
	local s = TrimString(seg)

	-- z-index
	if ParsePosSegmentZIndex(s, ctx) then
		return
	end

	-- position: fixed / absolute / static
	local pos_mode = string.match(s, '^%s*(fixed|absolute|static)%s*$')

	if pos_mode then
		ctx.position = pos_mode
		return
	end

	local pos_key, pos_val = string.match(s, '^%s*position%s*:%s*(%S+)%s*$')

	if pos_key then
		pos_val = TrimString(pos_val)

		if CONST_position_str[pos_val] then
			ctx.position = pos_val
			return
		end
	end

	-- inset
	local inset_val = string.match(s, '^%s*inset%s*:%s*(.+)%s*$')

	if not inset_val then
		inset_val = string.match(s, '^%s*inset%s+(.+)%s*$')
	end

	if inset_val then
		ctx.inset = ParseInsetValues(inset_val)
		return
	end

	-- flex 维度段 (格式1~4; 优先于盒模型段)
	if IsFlexSegment(s) then
		local layout_algo, x, y, w, h = ParsePosFlexSegment(s)

		ctx.has_flex = true
		ctx.flex_algo = layout_algo
		ctx.flex_x = x
		ctx.flex_y = y
		ctx.flex_w = w
		ctx.flex_h = h
		return
	end

	-- 盒模型: top/left/width/height...
	local box_key, box_val = string.match(s, '^%s*(%a+)%s*:%s*(.+)%s*$')

	if not box_key then
		box_key, box_val = string.match(s, '^%s*([%a%-]+)%s+(.+)%s*$')
	end

	if box_key then
		box_key = TrimString(box_key)
		local ctx_key = CONST_box_key_str[box_key]

		if ctx_key then
			ctx[ctx_key] = ParsePosDimNumber(ParsePosDimToken(box_val))
		end
	end
end


-- 由盒模型 ctx 构建 x,y,w,h (未走 flex 时)
local function BuildBoxRect(ctx, parent_path)
	if ctx.inset then
		local t = ParsePosDimNumber(ctx.inset[1])
		local r = ParsePosDimNumber(ctx.inset[2])
		local b = ParsePosDimNumber(ctx.inset[3])
		local l = ParsePosDimNumber(ctx.inset[4])

		if 0 == t and 0 == r and 0 == b and 0 == l then
			return 0, 0, -1, -1
		end

		local parent_w = 0
		local parent_h = 0

		if parent_path then
			local parent_rect = kgui.wndpos(parent_path, false)

			parent_w = parent_rect['w'] or 0
			parent_h = parent_rect['h'] or 0
		else
			parent_w, parent_h = kgui.wh()
		end

		local x = l
		local y = t
		local w = parent_w - l - r
		local h = parent_h - t - b

		if w < 0 then
			w = 0
		end

		if h < 0 then
			h = 0
		end

		return x, y, w, h
	end

	local x = ctx.left
	if nil == x then
		x = 0
	end

	local y = ctx.top
	if nil == y then
		y = 0
	end

	local w = ctx.width
	if nil == w then
		w = -1
	end

	local h = ctx.height
	if nil == h then
		h = -1
	end

	return x, y, w, h
end


-- 解析 flex 单分量
local function ParseFlexDim(v, is_size_dim)
	if 'string' == type(v) then
		local flow = CONST_flex_flow_str[v]

		if flow then
			return EncodeFlexFlow(flow)
		end

		local px = string.match(v, '^(%-?%d+)px$')

		if px then
			return EncodeFlexPx(tonumber(px))
		end

		local n = tonumber(v)

		if n then
			return EncodeFlexPx(n)
		end
	end

	if 'number' == type(v) then
		if is_size_dim and -1 == v then
			return EncodeFlexFlow(CONST_flex_flow_auto)
		end

		return EncodeFlexPx(v)
	end

	return EncodeFlexPx(0)
end


-- flex 自动布局: 编码 rect_in_flex
local function EncodeFlexRect(x, y, w, h)
	-- eg. 1. w = 'auto' / h = -1 -> KLB_WND_FLEX_TAG_FLOW & KLB_WND_FLEX_FLOW_AUTO
	-- eg. 2. w = 'grow', 'fill', 'stretch', 'inherit'
	-- eg. 3. x/y/w/h = 10 / '10px' -> KLB_WND_FLEX_TAG_PX
	local x1 = ParseFlexDim(x, false)
	local y1 = ParseFlexDim(y, false)
	local w1 = ParseFlexDim(w, true)
	local h1 = ParseFlexDim(h, true)

	-- KLB_WND_FLEX_TAG_PX : 使用低24位作为坐标值(带符号)
	-- KLB_WND_FLEX_TAG_FLOW : 参与流式/弹性策略
	return x1, y1, w1, h1
end


-- 合并 ctx -> is_flex, 坐标, style, layout_algo, z_index
local function MergePosCtx(ctx, parent_path)
	if ctx.has_flex then
		local layout_algo = ctx.flex_algo or CONST_flex_algo_flow_column
		local x1, y1, w1, h1 = EncodeFlexRect(ctx.flex_x, ctx.flex_y, ctx.flex_w, ctx.flex_h)

		return true, x1, y1, w1, h1, CONST_flex_style, layout_algo, ctx.z_index
	end

	local ref_parent = parent_path

	if 'fixed' == ctx.position then
		ref_parent = nil
	end

	local x, y, w, h = BuildBoxRect(ctx, ref_parent)
	local x1, y1, w1, h1 = AutoRect(ref_parent, x, y, w, h)

	return false, x1, y1, w1, h1, 0, CONST_flex_algo_none, ctx.z_index
end


-- 解析高阶 pos 声明串: ';' 分段 → 逐段 → 合并
local function ParsePosDeclString(pos_str, parent_path)
	local ctx = NewPosCtx()

	for _, seg in ipairs(SplitPosSegments(TrimString(pos_str))) do
		ParsePosSegment(seg, ctx)
	end

	return MergePosCtx(ctx, parent_path)
end

-- @brief 自动布局 引擎: 根据父窗口路径, 计算子窗口位置/大小
-- 仅当 pos = {x, y, w, h};  且 x/y/w/h 全部为数值, 才认为 未启用flex, 使用 指定坐标的方式; (wh = -1 时, 表示自动计算宽高)
-- 未填 'pos' 时, 走原坐标路径, 坐标交由 append/C 默认
-- 否则 使用 flex 自动布局
flexer.parse_wnd_layout = function(wnd, parent_path)
	-- pos/z-index 参数 在 flexer 解析

	-- 解析 z-index (字段优先; pos 串内 z-index 段可补充)
	local z_index = wnd['z-index'] or nil -- z 索引(int16_t) 或 未设置(nil); C 默认0

	-- 解析 pos (未填时为 nil, 不默认 {})
	local pos = wnd['pos']

	-- 高阶 pos 声明串: ';' 分段解析
	if 'string' == type(pos) then
		local is_flex, x1, y1, w1, h1, style, layout_algo, z_ctx = ParsePosDeclString(pos, parent_path)

		if nil == z_index then
			z_index = z_ctx
		end

		return	is_flex,			-- 是否启用 flex 自动布局
				x1, y1, w1, h1,		-- 位置/大小
				style,				-- 窗口样式
				layout_algo,		-- L1 层级: 自动布局引擎
				z_index				-- z 索引(int16_t)
	end

	-- 判定是否启用 flex 自动布局
	local is_flex = IsFlexPos(pos)

	local layout_algo = CONST_flex_algo_none
	local x1, y1, w1, h1, style

	if is_flex then
		-- 使用 flex 自动布局 (table 格式)
		style = CONST_flex_style
		layout_algo = CONST_flex_algo_flow_column
		x1, y1, w1, h1 = EncodeFlexRect(pos[1], pos[2], pos[3], pos[4])
	else
		-- 使用标准指定坐标的方式
		style = 0
		layout_algo = CONST_flex_algo_none

		if IsPosUnset(pos) then
			-- 未设置 pos: 原坐标路径, 交由 append/C 默认
			x1, y1, w1, h1 = nil, nil, nil, nil
		else
			-- wh = -1 时由 AutoRect 自动计算宽高
			x1, y1, w1, h1 = AutoRect(parent_path, pos[1], pos[2], pos[3], pos[4])
		end
	end

	return	is_flex,			-- 是否启用 flex 自动布局
			x1, y1, w1, h1,		-- 位置/大小
			style,				-- 窗口样式
			layout_algo,		-- L1 层级: 自动布局引擎
			z_index				-- z 索引(int16_t)
end

return flexer
