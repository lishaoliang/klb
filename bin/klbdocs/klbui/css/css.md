## CSS样式

### 样式格式

* 原则1: 主体参考CSS3定义
* 原则2: 

```
表达式: [*.]*[:*]
eg. [私有子控件名.]主CSS样式[:状态]
```

* 通常normal状态省略

### 元素(窗口)状态

* 常规状态: normal
* 聚焦状态: focus
* 不使能状态: disable
* 选中状态: check
* 输入状态: input

```lua
{
    ['background-color'] = {255,220,220,220},      -- normal 常规状态
    ['background-color:focus'] = 0xFFA0A0A0,       -- focus 聚焦状态
    ['background-color:disable'] = '0xFFA0A0A0'    -- disable 不使能状态
    ['background-color:check'] = '0xFFA0A0A0'      -- check 选中状态
    ['background-color:input'] = '0xFFA0A0A0'      -- input 输入状态
}
```

### 私有子控件

* 对于含有私有子控件的添加前缀

```lua
{
    ['button.background-color'] = {255,220,220,220},      -- normal 常规状态
    ['button.background-color:focus'] = 0xFFA0A0A0,       -- focus 聚焦状态
    ['button.background-color:disable'] = '0xFFA0A0A0'    -- disable 不使能状态
    ['button.background-color:check'] = '0xFFA0A0A0'      -- check 选中状态
    ['button.background-color:input'] = '0xFFA0A0A0'      -- input 输入状态
}
```

### 边框模型
* 参考: https://www.w3school.com.cn/css/css_boxmodel.asp


* 模型图

```c
///  *---------------- margin(外边距) ------------------*
///  |  *------------- border(边框) -----------------*  |
///  |  |  *---------- padding(内边距) -----------*  |  |
///  |  |  |  *--------------------------------*  |  |  |
///  |  |  |  |                                |  |  |  |
///  |  |  |  |        element(元素/wnd)       |  |  |  |
///  |  |  |  |                                |  |  |  |
///  |  |  |  *--------------------------------*  |  |  |
///  |  |  *--------------------------------------*  |  |
///  |  *--------------------------------------------*  |
///  *--------------------------------------------------*
```

* 背景应用于由内容(元素wnd)和内边距、边框组成的区域

