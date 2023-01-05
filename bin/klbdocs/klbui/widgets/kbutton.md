## 按钮

* 注册类型: kbutton

### 概述

```c
// C实现文件: ./klb/src_c/klbgui/widgets/klbui_button.c
```

### 支持状态

* 普通: normal
* 聚焦: focus
* 不使能: disable


### CSS属性

#### 显隐

* 支持状态: normal
* 属性: visibility

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

#### 外边框

* 支持状态: normal
* 属性: margin

* 设置
```lua
{
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

#### 内边框

* 支持状态: normal
* 属性: padding

* 设置
```lua
{
    ['padding-top'] = 0,
    ['padding-right'] = 0,
    ['padding-bottom'] = 0,
    ['padding-left'] = 0,
}
```

* 获取
```lua
    0
```

#### 文本

##### 文本颜色

* 支持状态: normal, focus, disable
* 属性: color

* 设置
```lua
{
    ['color'] = {255,220,220,220},      -- normal 常规状态
    ['color:focus'] = 0xFFA0A0A0,       -- focus 聚焦状态
    ['color:disable'] = '0xFFA0A0A0'    -- disable 不使能状态
}
```

* 获取
```lua
    0xFFA0A0A0
```

##### 文本对齐

* 支持状态: normal, focus, disable
* 属性: text-align

* 设置
```lua
{
    ['text-align'] = 'center',
    ['text-align:focus'] = 'left',
    ['text-align:disable'] = 'right'
}
```

* 获取
```lua
    'center'        -- 中心对齐
    'left'          -- 左对齐
    'right'         -- 右对齐
```

#### 字体

##### 斜体

* 支持状态: normal, focus, disable
* 属性: font-style

* 设置
```lua
{
    ['font-style'] = 'normal',
    ['font-style:focus'] = 'normal',
    ['font-style:disable'] = 'italic',
}
```

##### 字体粗细

* 支持状态: normal, focus, disable
* 属性: font-weight

* 设置
```lua
{
    ['font-weight'] = 'normal',
    ['font-weight:focus'] = 'normal',
    ['font-weight:disable'] = 'bold',
}
```

##### 字体大小

* 支持状态: normal, focus, disable
* 属性: font-size

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

* 支持状态: normal, focus, disable
* 属性: background-color

* 设置
```lua
{
    ['background-color'] = {255,220,80,20},
    ['background-color:focus'] = {255,220,80,20},
    ['background-color:disable'] = {255,220,80,20},
}
```

* 获取
```lua
    0xFFA0A0A0
```

##### 背景图片

* 支持状态: normal, focus, disable
* 属性: background-image

* 设置
```lua
{
    ['background-image'] = 'aaa.bmp',
    ['background-image:focus'] = 'bbb.bmp',
    ['background-image:disable'] = 'ccc.bmp',
}
```

* 获取
```lua
    'aaa.bmp'
```

#### 边框

##### 边框类型

* 支持状态: normal, focus, disable
* 属性: border-style

* 设置
```lua
{
    ['border-style'] = 'none',
    ['border-style:focus'] = 'none',
    ['border-style:disable'] = 'none'
}
```

* 获取
```lua
    'none'
```

* 取值范围
```lua
{
    range = {
        "none",     -- 定义无边框
        "dotted",   -- 定义无边框
        "dashed",   -- 定义虚线边框
        "solid",    -- 定义实线边框
        "double",   -- 定义双边框
        "hidden",   -- 定义隐藏边框
        "inset",    -- 定义3D inset边框
        "outset",   -- 定义3D outset边框
    }
}
```

##### 边框宽度

* 支持状态: normal, focus, disable
* 属性: border-width

* 设置
```lua
{
    ['border-width'] = 2,
    ['border-width:focus'] = 2,
    ['border-width:disable'] = 2
}
```

* 获取
```lua
    2
```

##### 边框颜色

* 支持状态: normal, focus, disable
* 属性: border-color

* 设置
```lua
{
    ['border-color'] = {255,220,30,30},
    ['border-color:focus'] = {255,220,30,30},
    ['border-color:disable'] = {255,220,30,30},
}
```

* 获取
```lua
    0xFFA0A0A0
```

##### 圆角边框
* 支持状态: normal, focus, disable
* 属性: border-radius

* 设置
```lua
{
    ['border-radius'] = 1,
    ['border-radius:focus'] = 2,
    ['border-radius:disable'] = 3,
}
```

* 获取
```lua
    1
```

### 自定义属性

#### 标题

* 支持状态: normal
* 属性: title

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

* 支持状态: normal
* 属性: value

* 设置
```lua
{
    ['value'] = '按钮1'
}
```

* 获取
```lua
    '按钮1'
```
