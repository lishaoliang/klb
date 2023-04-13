## 静态显示框

* 注册类型: kstatic

### 概述

```c
// C实现文件: ./klb/src_c/klbgui/widgets/klbui_static.c
```

### 支持状态

* 普通: normal


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
* 支持状态: normal

* 设置
```lua
{
    ['color'] = {255,220,220,220},          -- normal 常规状态
    ['color'] = 0xFFA0A0A0,                 -- normal 常规状态
    ['color'] = '0xFFA0A0A0',               -- normal 常规状态
}
```

* 获取
```lua
    0xFFA0A0A0
```

##### 文本对齐

* 属性: text-align
* 支持状态: normal
* 设置
```lua
{
    ['text-align'] = 'left',
    ['text-align'] = 'center',
    ['text-align'] = 'right',
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
* 支持状态: normal

* 设置
```lua
{
    ['font-size'] = 24,
}
```

#### 背景

##### 背景色

* 属性: background-color
* 支持状态: normal

* 设置
```lua
{
    ['background-color'] = {255,220,80,20},
    ['background-color'] = 0xFFA0A0A0,
    ['background-color'] = '0xFFA0A0A0',
}
```

* 获取
```lua
    0xFFA0A0A0
```

##### 背景图片

* 属性: background-image
* 支持状态: normal

* 设置
```lua
{
    ['background-image'] = '',
    ['background-image'] = 'bbb.bmp',
}
```

* 获取
```lua
    'bbb.bmp'
```

#### 边框

##### 边框宽度

* 属性: border-width
* 支持状态: normal

* 设置
```lua
{
    ['border-width'] = 1,
    ['border-width'] = {1, 1, 1, 1},
}
```

* 获取
```lua
    1
```

##### 边框颜色

* 属性: border-color
* 支持状态: normal

* 设置
```lua
{
    ['border-color'] = {255,220,30,30},
    ['border-color'] = 0xFFA0A0A0,
    ['border-color'] = '0xFFA0A0A0',
}
```

* 获取
```lua
    0xFFA0A0A0
```
