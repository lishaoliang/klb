

安装
  go get github.com/lishaoliang/klb


遵循命名规则, 从命名大致推断代码位置及功能

C,CGO命名规则
  C函数格式: "xxx_xxx_xxx"
  在Go语言中调用的cgo函数格式: "xxx_go_xxx_xxx_cgo"
  Go语言导出给C调用的函数格式: "xxx_go_xxx_xxx"

CGO,Lua库命名规则
  打开某个库: "klua_go_open_xxx"
  在Go语言中调用的cgo函数格式: "klua_go_open_xxx_cgo"
  库中出函数: "klua_go_xxx_xxx"

C,Lua库命名规则
  打开某个库: "klua_open_xxx"
  库中函数: "klua_xxx_xxx"

Lua命名规则
  Lua标准库
  Lua第三方C开源扩展库
  C实现的k系列扩展库: "k*"
  Go实现的kx系列扩展库: "kx_*"
  函数: "xxx_xxx_xxx"

Go导出Lua库命名规则
  通用库: "kg_*"
  Arm库: "ka_*"
  x86,x86_64库: "kx_*"


模块使用关系
                App
      Ctx       Ctx       Ctx       ...
  Env Env Env      
* App可以在main开启多个Ctx, 每个Ctx可使用不同的加载库
* 每个Ctx可以开启多个Env(Lua环境),开启的Env继承Ctx加载库等属性
* Env之间可以相互通信,即通过kg_thread.post发送消息
