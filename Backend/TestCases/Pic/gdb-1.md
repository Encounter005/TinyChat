+++
title = "GDB 使用指南-1"
tags = [ "C/C++", "调试"]
categories = [ "工具"]
date = "2024-03-18T21:48:08+08:00"
+++
本文主要描述了GDB的基础命令使用

# GDB 介绍

GDB，全称GNU调试器（GNU Debugger），是一个强大的Unix系统下的源代码级调试工具。它可以帮助程序员查看程序在执行过程中的内部状态，从而更好地理解程序的运行机制。GDB主要用于调试C和C++语言编写的程序。它的存在，使得我们能够更深入地了解程序的运行过程，找出并修复程序中的错误。

# 基本命令

## 启动GDB

假如我们有一个编译好的C++程序，名字是`a.out`，（记得编译的时候要带`-g`表示有调试信息），接下来我们需要启动GDB

```
gdb a.out
```

## 查看源码

在GDB中，我们可以通过`list`来查看源码信息

```
(gdb) list
```

[![Ogk3e6.png](https://ooo.0x0.ooo/2024/03/18/Ogk3e6.png)](https://img.tg/image/Ogk3e6)

## 设置断点

断点是调试的核心，允许在程序特定的位置暂停执行，可以通过`break`来设置断点，通常只需要`b + 文件名: + 行号`即可

```
(gdb) b main.cpp:5
```

[![OgkXsD.png](https://ooo.0x0.ooo/2024/03/18/OgkXsD.png)](https://img.tg/image/OgkXsD)

## 查看断点

在设置了多个断点之后，我们可能需要查看所有的断点信息，来确保没有遗漏或者打错位置的断点，可以使用`info breakpoints`来查看所有的断点

```
(gdb) info breakpoints
```

[![OgkIoF.png](https://ooo.0x0.ooo/2024/03/18/OgkIoF.png)](https://img.tg/image/OgkIoF)

## 运行代码

接下来可以开始执行程序了，使用`run`来让程序跑起来

```
(gdb) run
```

[![OgkCGP.png](https://ooo.0x0.ooo/2024/03/18/OgkCGP.png)](https://img.tg/image/OgkCGP)

当程序遇到断点时，它会暂停，等待我们的进一步指令。

## 显示变量值

在调试过程中，如果需要查看变量，使用`print + variable_name`

```
(gdb) print b
```

## 观察变量

在某些情况下，我们可能想要知道一个变量何时被修改。GDB提供了`watch`命令，允许我们观察变量的变化。

```
(gdb) watch variable_name
```

每当variable_name的值发生变化时，程序会暂停执行。
[![OgxOhl.png](https://ooo.0x0.ooo/2024/03/18/OgxOhl.png)](https://img.tg/image/OgxOhl)

## 单步运行

如果需要一行一行执行代码，可以使用`step`，一般用`s`即可

```
(gdb) s
```

[![Ogkw3b.png](https://ooo.0x0.ooo/2024/03/18/Ogkw3b.png)](https://img.tg/image/Ogkw3b)

## 继续执行

如果设置了断点停止了程序，这时想让程序直接执行到下一个断点时，使用`continue`

```
(gdb) continue
```

[![Ogk8yI.png](https://ooo.0x0.ooo/2024/03/18/Ogk8yI.png)](https://img.tg/image/Ogk8yI)
