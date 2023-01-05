## 静态显示框

* 注册类型: kstatic

### 概述

```c
// C实现文件: ./klb/src_c/klbgui/widgets/klbui_static.c
```

### 支持状态

* 普通: normal


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
