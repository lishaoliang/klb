## 组合框

* 注册类型: kcombo

### 概述

```c
// C实现文件: ./klb/src_c/klbgui/widgets/klbui_combo.c
```

### 支持状态

* 普通: normal
* 聚焦: focus
* 不使能: disable


### CSS属性

#### 显隐

* 属性: visibility
* 无状态

* 显示
```lua
{
    ['visibility'] = 'visible',
    ['visibility'] = true,
}
```

* 隐藏
```lua
{
    ['visibility'] = 'hidden',
    ['visibility'] = false,
}
```

* 获取
```lua
    'visible'   -- 显示
    'hidden'    -- 隐藏
```

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
    1
```

#### 文本

##### 文本颜色

* 属性: color
* 支持状态: normal, focus, disable

* 设置
```lua
{
    ['color'] = {255,220,220,220},          -- normal 常规状态
    ['color:focus'] = 0xFFA0A0A0,           -- focus 聚焦状态
    ['color:disable'] = '0xFFA0A0A0',       -- disable 不使能状态
}
```

* 获取
```lua
    0xFFA0A0A0
```

##### 文本对齐

* 属性: text-align
* 支持状态: normal, focus, disable

* 设置
```lua
{
    ['text-align'] = 'center',
    ['text-align:focus'] = 'left',
    ['text-align:disable'] = 'right',
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
* 支持状态: normal, focus, disable

* 设置
```lua
{
    ['font-size'] = 24,
    ['font-size:focus'] = 28,
    ['font-size:disable'] = 20,
}
```


#### 背景

##### 背景色

* 属性: background-color
* 支持状态: normal, focus, disable

* 设置
```lua
{
    ['background-color'] = {255,220,80,20},
    ['background-color:focus'] = 0xFFA0A0A0,
    ['background-color:disable'] = '0xFFA0A0A0',
}
```

#### 边框

##### 边框宽度

* 属性: border-width
* 支持状态: normal, focus, disable

* 设置
```lua
{
    ['border-width'] = 1,
    ['border-width'] = {1, 1, 1, 1},

    ['border-width:focus'] = 1,
    ['border-width:disable'] = 1,
}
```

* 获取
```lua
    1
```

##### 边框颜色

* 属性: border-color
* 支持状态: normal, focus, disable

* 设置
```lua
{
    ['border-color'] = {255,220,30,30},
    ['border-color'] = 0xFFA0A0A0,
    ['border-color'] = '0xFFA0A0A0',

    ['border-color:focus'] = {255,220,30,30},
    ['border-color:disable'] = {255,220,30,30},
}
```

* 获取
```lua
    0xFFA0A0A0
```

#### 子控件 - 右侧图形按钮

##### 右侧按钮-前景色

* 属性: color
* 支持状态: normal, focus, disable

* 设置
```lua
{
    ['button_right.color'] = {255,220,220,220},      -- normal 常规状态
    ['button_right.color:focus'] = 0xFFA0A0A0,       -- focus 聚焦状态
    ['button_right.color:disable'] = '0xFFA0A0A0'    -- disable 不使能状态
}
```

* 获取
```lua
    0xFFA0A0A0
```

#### 子控件 - 弹出菜单

##### 弹出菜单-内边距

* 属性: padding
* 无状态

* 设置
```lua
{
    ['menu.padding'] = {1, 1, 1, 1},
    ['menu.padding-top'] = 1,
    ['menu.padding-right'] = 1,
    ['menu.padding-bottom'] = 1,
    ['menu.padding-left'] = 1,
}
```

* 获取
```lua
    0
```

##### 弹出菜单-背景色

* 属性: background-color
* 支持状态: normal

* 设置
```lua
{
    ['menu.background-color'] = {255,220,80,20},
    ['menu.background-color'] = 0xFFA0A0A0,
    ['menu.background-color'] = '0xFFA0A0A0',
}
```

* 获取
```lua
    0xFFA0A0A0
```

##### 弹出菜单-边框宽度

* 属性: border-width
* 支持状态: normal

* 设置
```lua
{
    ['menu.border-width'] = 1,
    ['menu.border-width'] = {1, 1, 1, 1},
}
```

* 获取
```lua
    1
```

##### 弹出菜单-边框颜色

* 属性: border-color
* 支持状态: normal

* 设置
```lua
{
    ['menu.border-color'] = {255,220,30,30},
    ['menu.border-color'] = 0xFFA0A0A0,
    ['menu.border-color'] = '0xFFA0A0A0',
}
```

* 获取
```lua
    0xFFA0A0A0
```

#### 子控件 - 弹出菜单子项

##### 弹出菜单子项-文本颜色

* 属性: color
* 支持状态: normal, focus

* 设置
```lua
{
    ['menu_item.color'] = {255,220,220,220},          -- normal 常规状态
    ['menu_item.color:focus'] = 0xFFA0A0A0,           -- focus 聚焦状态
}
```

* 获取
```lua
    0xFFA0A0A0
```

##### 弹出菜单子项-文本对齐

* 属性: text-align
* 支持状态: normal, focus

* 设置
```lua
{
    ['menu_item.text-align'] = 'center',
    ['menu_item.text-align:focus'] = 'left',
}
```

* 获取
```lua
    'center'        -- 中心对齐
    'left'          -- 左对齐
    'right'         -- 右对齐
```

##### 弹出菜单子项-边框宽度

* 属性: border-width
* 支持状态: normal, focus

* 设置
```lua
{
    ['menu_item.border-width'] = 1,
    ['menu_item.border-width'] = {1, 1, 1, 1},

    ['menu_item.border-width:focus'] = 1,
}
```

* 获取
```lua
    1
```

##### 弹出菜单子项-边框颜色

* 属性: border-color
* 支持状态: normal, focus

* 设置
```lua
{
    ['menu_item.border-color'] = {255,220,30,30},
    ['menu_item.border-color'] = 0xFFA0A0A0,
    ['menu_item.border-color'] = '0xFFA0A0A0',

    ['menu_item.border-color:focus'] = {255,220,30,30},
}
```

* 获取
```lua
    0xFFA0A0A0
```

### 自定义属性

#### 标题

* 属性: title
* 无状态

* 设置
```lua
{
    ['title'] = '按钮1'
}
```

* 获取
```lua
    '按钮1'
```

#### 值

* 属性: value
* 无状态

* 设置
```lua
{
    ['value'] = 'aaa'
}
```

* 获取
```lua
    'aaa'
```

#### 添加选项

* 方法: append
* 无状态

* 弹出菜单选项次序, 会按照append次序

* 设置
```lua
{
    {
        ['1'] = '测试1',
        ['2'] = '测试2',
        ['3'] = '测试3',
    }
}
```

* 示例
```lua
{
    ['type'] = 'kcombo',
    ['pos'] = {10 + 240, 64 + 50 + 40, 140, 32},
    ['name'] = 'combo23',
    ['append'] = {
        {['1'] = '测试1'},
        {['2'] = '测试2'},
        {['3'] = '测试3'},
        {['4'] = '测试4'},
    },
}
```

* 无获取


#### 清空选项

* 方法: clear
* 无状态

* 示例
```lua
local jq = klbui.select(dialog)     -- 单选
jq('combo23').clear()
```

* 无获取
