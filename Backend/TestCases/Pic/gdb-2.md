+++
title = "GDB 使用指南-2"
tags = [ "C/C++", "调试"]
categories = [ "工具"]
date = "2024-03-18T22:24:45+08:00"
+++
本文主要描述了GDB中断点调试和数据命令的使用方法

# 断点调试

## 设置断点

要设置断点时，使用`break`或`b`来设置断点

```
b main.cpp:5
```

## 条件断点

有时，我们只想在满足特定条件时暂停程序

```
b main.cpp:5 if i == 5
```

![](https://pic.imgdb.cn/item/65f84de09f345e8d03ec1444.png)

## 查看断点

如果需要查看所有断点所在的位置信息，使用`info breakpoints`

```
info breakpoints
```

## 清除断点

有些断点我们不再需要，可以直接清除

```
# 清理第5行的断点
clear main.cpp:5
```

## 观察点

观察点允许我们监视变量的值，并在其值发生变化时暂停程序。这对于跟踪变量的变化非常有用

```
watch variable_name
```

## 捕捉点

捕捉点是一种特殊类型的断点，它允许我们在发生特定事件，如抛出异常时暂停程序。

```
# 设置捕捉点
catch throw
```

![](https://pic.imgdb.cn/item/65f84de19f345e8d03ec15c8.png)

这将设置一个捕捉点，当程序抛出异常时，它将暂停。

# 数据类型

## 显示表达式值

在GDB中，可以通过`print`来评估表达式并显示其值

```
print variable_name
```

## 查看数据类型

当我们需要查看一个变量的类型，（例如类型推断的变量），需要使用`whatis`命令

```
whatis variable_name
```
![](https://pic.imgdb.cn/item/65f84de19f345e8d03ec1c4e.png)

## 打印变量值

`display`命令可以自动显示表达式的值，每次程序停止时都会显示

```
display x
```

每次程序停止时，都会显示变量x的值

![](https://pic.imgdb.cn/item/65f84de19f345e8d03ec185b.png)

## 修改变量值

在某些情况下，需要在调试过程中修改变量值。假设要将变量x的值设置为5，使用`set`命令

```
set variable x = 5
```

这将立即更改变量x的值，而不需要重新编译或重新启动程序。

![](https://pic.imgdb.cn/item/65f84de19f345e8d03ec1ad6.png)
