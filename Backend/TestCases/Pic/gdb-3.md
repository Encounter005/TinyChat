+++
title = "GDB使用指南-3"
tags = [ "C/C++", "调试",]
categories = [ "工具",]
date = "2024-03-20T22:46:38+08:00"
+++
# 调试运行环境

## 设置运行参数

```c++
#include <iostream>

int main (int argc, char *argv[]) {

    std::cout << "The arguments' count is: " << argc << std::endl;
    for(int i = 0; i < argc; i++) {
        std::cout << argv[i] << " ";
    }
    std::cout << std::endl;

    return 0;
}
```

在GDB中，可以通过`set args`命令来设置运行参数

```
(gdb) set args 参数1 参数2...
```

## 改变工作目录

有时候我们需要在GDB跳转到别的文件目录中

```
(gdb) cd /path/to/directory
```

## 设置输入输出

假设我们有一个`input.txt`和一个`output.txt`作为程序的输入和输出文件，我们想让gdb在启动程序的时候进行重定向

```
(gdb) run < input.txt > output.txt
```

如果想保存整个程序的日志文件，我们需要`set logging`命令

```
(gdb) set logging file 文件名
(gdb) set logging on
```

## 线程调试

```c++
#include <iostream>
#include <thread>
#include <chrono>
int main() {

    std::thread t1( []() {
        std::cout << "this is thread 1" << std::endl;
        std::this_thread::sleep_for(
            std::chrono::duration( std::chrono::seconds( 10 ) ) );
    } );
    std::thread t2( []() {
        std::cout << "this is thread 2" << std::endl;
        std::this_thread::sleep_for(
            std::chrono::duration( std::chrono::seconds( 10 ) ) );
    } );
    t1.detach(), t2.detach();

    return 0;
}
```

在线程调试中，如果要查看线程信息可以使用`info thread`

```
(gdb) info thread
```

如果需要切换到特定的线程里面

```
(gdb) thread 线程号
```

其他关于线程的命令

- `break location thread thread-id`: 在指定线程上设置断点，仅当该特定线程执行时，它才会停止。
- `set scheduler-locking`: 控制在调试过程中其他线程的行为。可以设置以下模式：

  off：不锁定，所有线程都可以运行。
  on：锁定，只有当前线程可以运行。
  step：只有一个线程在单步执行时才锁定其他线程。

- `thread apply all command`: 对所有线程执行指定的命令。
- `set follow-fork-mode`: 设置GDB在fork系统调用时如何跟踪进程。可以设置为 parent 或 child，分别表示跟踪父进程或子进程。
- `set detach-on-fork`: 控制GDB在fork后是否保持调试父进程和子进程。
- `catch thread`: 在任何线程创建或退出时设置断点。
- `thread select`: 切换到已经停止的线程（例如，通过断点或者异常）。

## 检查堆栈

```c++
#include <iostream>

void test(int i) {
    if(i < 0) {
        return;
    }
    test(i - 1);
    std::cout << i << std::endl;
}

int main() {

    test(10);

    return 0;
}

```

在调试过程中，堆栈是经常需要查看的

```
(gdb) info stack
```

![stack](https://pic.imgdb.cn/item/65fae7ba9f345e8d03ce1ec5.png)

# 跳转命令

在GDB中，我们可以使用jump命令（或简写为j）来实现跳转执行。例如，如果我们想从当前位置直接跳到第10行执行，可以使用以下命令：

```
j 10
```

这样程序就会直接跳转到第10行开始执行，跳过中间所有的代码

## 跳转的限制

虽然跳转命令很强大，但它也有一些限制。例如，我们不能跳到一个没有被加载的函数或模块中，也不能跳到一个已经执行完毕的函数或模块中。
此外，频繁地使用跳转命令可能会导致程序状态的不一致，因此在使用时需要格外小心。

## 跳转应用的场景

1. **绕过错误或崩溃**：如果你在调试过程中遇到了一个会导致程序崩溃的代码块，并且希望跳过这部分代码继续调试程序的其他部分，可以使用jump命令。
2. **重复执行代码**：你可能想反复执行某段代码以观察问题，jump能够让你回到这段代码的起点。
3. **测试代码路径**：在多个分支或执行路径的代码中，你可能想强制执行某个特定的路径，即使它实际上在当前的程序状态下不会被执行。
4. **跳过执行时间很长的函数**：在调试时，你可能不想等待一个耗时的函数完成。你可以跳过这个函数的调用，直接跳到其后的代码执行。

# 信号命令

信号是一种通知机制，用于告知进程某些事件已经发生。经常被用于处理异常情况（例如程序错误、外部中断等），在GDB中我们可以通过信号命令来处理、模拟这些信号

## 生成和处理信号

在GDB中，我们可以使用signal命令来发送信号到正在调试的程序。例如，要发送一个SIGINT信号，我们可以使用以下命令：

```
(gdb) signal SIGINT
```

这是模拟用户按下`Ctrl+C`的情况

## 查看和设置信号

要查看当前程序如何处理各种信号，可以使用`info signals`

```
(gdb) info signals
```

这将显示所有信号及其当前的处理方式。

如果我们想改变某个信号的处理方式，可以使用`handle`命令。例如，要让程序在接收到SIGINT信号时停止并打印消息，我们可以使用：

```
(gdb) handle SIGINT stop print
```

这样，每当程序接收到`SIGINT`信号时，它都会停止执行并在GDB中打印消息

# 运行SHELL命令

要想运行shell命令，我们只需要在命令前加上shell关键字即可

```
(gdb) shell ...
```

# 调试core文件

在软件开发过程中，程序可能会出现崩溃。为了更好地理解和解决这些崩溃，我们经常需要调试程序的core文件。core文件是程序崩溃时生成的，它包含了程序崩溃时的内存快照，帮助我们定位问题。

## 生成core文件

先放一份源文件，方便调试

```c++
#include <iostream>
#include <numeric>

int main() {
    int a[10];
    std::iota(a , a + 10, 0);

    std::cout << a[100001] << std::endl;
    return 0;
}
```

当程序崩溃时，系统通常会生成一个core文件，要确保core文件被生成，需要设置`ulimit`

```
ulimit -c unlimited
```

## 使用GDB查看core文件

要使用GDB查看core文件，我们需要两个文件：崩溃的程序的可执行文件和core文件。使用以下命令启动GDB：

```
gdb <executable-file> <core-file>
```

这里可以使用coredumpctl来调用gdb

```
# 启动最新的core文件
coredumpctl debug
```

![core](https://pic.imgdb.cn/item/65faf78d9f345e8d0337efe7.png)

## 实时观察进程Crash信息

有时，我们可能希望实时观察进程的崩溃信息，而不是等待程序崩溃后再查看core文件。为此，我们可以使用strace工具跟踪系统调用和信号。

```
strace -o output.txt <executable-file>
```

这将在output.txt文件中记录所有的系统调用和信号，帮助我们实时观察进程的行为。

