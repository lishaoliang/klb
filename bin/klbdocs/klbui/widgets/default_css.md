## 默认全局CSS

* 无注册类型

### 概述

```c
// C实现文件: ./klb/src_c/klbgui/extensions/klbuiex_default.c
```

### 支持状态

* 常规状态: normal
* 聚焦状态: focus
* 不使能状态: disable
* 选中状态: check
* 输入状态: input

### 参考使用

* 设置
```lua
local klbui = require("klbcore.klbui")

-- 设置多个全局CSS属性
klbui.default_css({
    ['color'] = {255,80,80,80},
    ['color:focus'] = {255,220,220,220},
    ['color:disable'] = {255,20,20,20},
})
```

* 获取
```lua
local klbui = require("klbcore.klbui")

-- 获取单个全局CSS属性
local color = klbui.default_css('color')
print('color:', color)
```

### CSS属性

#### 外边距

* 属性: margin
* 无状态

* 设置
```lua
{
    ['margin'] = {0, 0, 0, 0},
    ['margin-top'] = 0,
    ['margin-right'] = 0,
    ['margin-bottom'] = 0,
    ['margin-left'] = 0,
}
```

* 获取
```lua
    0
```

#### 内边距

* 属性: padding
* 无状态

* 设置
```lua
{
    ['padding'] = {1, 1, 1, 1},
    ['padding-top'] = 1,
    ['padding-right'] = 1,
    ['padding-bottom'] = 1,
    ['padding-left'] = 1,
}
```

* 获取
```lua
    0
```

#### 文本

##### 文本颜色

* 属性: color
* 支持状态: normal, focus, disable, check, input

* 设置
```lua
{
    ['color'] = {255,220,220,220},          -- normal 常规状态
    ['color:focus'] = 0xFFA0A0A0,           -- focus 聚焦状态
    ['color:disable'] = '0xFFA0A0A0',       -- disable 不使能状态
    ['color:check'] = {255,220,220,220},    -- check 选中状态
    ['color:input'] = {255,220,220,220},    -- 输入状态
}
```

* 获取
```lua
    0xFFA0A0A0
```

##### 文本对齐

* 属性: text-align
* 支持状态: normal, focus, disable, check, input

* 设置
```lua
{
    ['text-align'] = 'center',
    ['text-align:focus'] = 'left',
    ['text-align:disable'] = 'right',
    ['text-align:check'] = 'left',
    ['text-align:input'] = 'left'
}
```

* 获取
```lua
    'center'        -- 中心对齐
    'left'          -- 左对齐
    'right'         -- 右对齐
```

#### 字体

##### 字体大小

* 属性: font-size
* 支持状态: normal, focus, disable, check, input

* 设置
```lua
{
    ['font-size'] = 24,
    ['font-size:focus'] = 28,
    ['font-size:disable'] = 20,
    ['font-size:check'] = 20,
    ['font-size:input'] = 20,
}
```

#### 背景

##### 背景色

* 属性: background-color
* 支持状态: normal, focus, disable, check, input

* 设置
```lua
{
    ['background-color'] = {255,220,80,20},
    ['background-color:focus'] = 0xFFA0A0A0,
    ['background-color:disable'] = '0xFFA0A0A0',
    ['background-color:check'] = {255,220,80,20},
    ['background-color:input'] = {255,220,80,20},
}
```

* 获取
```lua
    0xFFA0A0A0
```

##### 背景图片

* 属性: background-image
* 支持状态: normal, focus, disable, check, input

* 设置
```lua
{
    ['background-image'] = '',
    ['background-image:focus'] = 'bbb.bmp',
    ['background-image:disable'] = 'ccc.bmp',
    ['background-image:check'] = 'eee.bmp',
    ['background-image:input'] = 'fff.bmp',
}
```

* 获取
```lua
    'bbb.bmp'
```

#### 边框

##### 边框宽度

* 属性: border-width
* 支持状态: normal, focus, disable, check, input

* 设置
```lua
{
    ['border-width'] = 1,
    ['border-width'] = {1, 1, 1, 1},

    ['border-width:focus'] = 1,
    ['border-width:disable'] = 1,
    ['border-width:check'] = 1,
    ['border-width:input'] = 1,
}
```

* 获取
```lua
    1
```

##### 边框颜色

* 属性: border-color
* 支持状态: normal, focus, disable, check, input

* 设置
```lua
{
    ['border-color'] = {255,220,30,30},
    ['border-color'] = 0xFFA0A0A0,
    ['border-color'] = '0xFFA0A0A0',

    ['border-color:focus'] = {255,220,30,30},
    ['border-color:disable'] = {255,220,30,30},
    ['border-color:check'] = {255,220,30,30},
    ['border-color:input'] = {255,220,30,30},
}
```

* 获取
```lua
    0xFFA0A0A0
```
