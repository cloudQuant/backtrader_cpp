# Windows DLL 文件详解

## 🎯 为什么有两个文件？

在 Windows 上使用 GCC/MinGW 编译动态库时，会生成两个文件：

```
libbacktrader_core.dll      # 动态链接库（运行时）
libbacktrader_core.dll.a    # 导入库（编译时）
```

这是 Windows 动态链接的设计特性，**不是 bug**。

## 📦 两个文件的详细说明

### 1. libbacktrader_core.dll（动态链接库）

**文件类型：** Windows 动态链接库（Dynamic Link Library）

**内容：**
- ✅ 实际的机器代码（编译后的函数实现）
- ✅ 数据段（全局变量、常量等）
- ✅ 资源（如果有的话）
- ✅ 导出表（哪些函数可以被外部调用）

**何时使用：**
- 🏃 **运行时**：程序执行时由操作系统加载

**类比：**
- 就像一本书的内容本身

**大小：** ~38 MB（包含所有代码）

### 2. libbacktrader_core.dll.a（导入库）

**文件类型：** 静态导入库（Import Library）

**内容：**
- ✅ 函数名称列表
- ✅ 函数在 DLL 中的位置信息
- ✅ 链接器需要的元数据
- ❌ **不包含**实际的函数实现代码

**何时使用：**
- 🔗 **编译/链接时**：告诉链接器去哪里找函数

**类比：**
- 就像一本书的目录和索引

**大小：** ~8 MB（只有符号表）

## 🔄 完整的工作流程示例

### 步骤 1: 编译你的程序

```cpp
// myprogram.cpp
#include "indicator.h"

int main() {
    auto sma = std::make_shared<backtrader::SMA>(data, 10);
    sma->calculate();
    return 0;
}
```

### 步骤 2: 链接时使用导入库

```batch
# 编译
g++ -c myprogram.cpp -IF:\source_code\backtrader_cpp\include

# 链接（使用 .dll.a）
g++ myprogram.o ^
    -LF:\source_code\backtrader_cpp ^
    -lbacktrader_core ^
    -o myprogram.exe

# 链接器实际读取的是: libbacktrader_core.dll.a
```

**链接器做的事情：**
```
1. 打开 libbacktrader_core.dll.a
2. 读取符号表：
   - SMA::SMA()
   - SMA::calculate()
   - ...
3. 在 myprogram.exe 中记录：
   "这些函数在 libbacktrader_core.dll 中，
    运行时去那里找"
4. 生成可执行文件（只有几 MB）
```

### 步骤 3: 运行时使用 DLL

```batch
# 运行程序
myprogram.exe
```

**操作系统做的事情：**
```
1. 加载 myprogram.exe
2. 读取依赖项：需要 libbacktrader_core.dll
3. 搜索 DLL：
   a. 当前目录
   b. System32
   c. PATH 环境变量
4. 找到并加载 libbacktrader_core.dll
5. 解析符号：
   - SMA::SMA() → 地址 0x12345678
   - SMA::calculate() → 地址 0x12345ABC
6. 更新 myprogram.exe 的函数调用地址
7. 程序开始执行
```

## 🆚 与 MSVC 的对比

### MinGW/GCC（你正在使用的）

```
编译时: libbacktrader_core.dll.a
运行时: libbacktrader_core.dll
```

### Microsoft Visual C++ (MSVC)

```
编译时: backtrader_core.lib
运行时: backtrader_core.dll
```

**区别：**
- MinGW: `.dll.a` 后缀（GNU 风格）
- MSVC: `.lib` 后缀（微软风格）
- 本质上是**同一个概念**

## 📂 文件放置建议

### 开发时

```
F:\source_code\backtrader_cpp\
├── libbacktrader_core.dll       ← 保留在这里
├── libbacktrader_core.dll.a     ← 保留在这里
└── libbacktrader_core.a         ← 静态库也在这里
```

### 编译你的程序时

```batch
# 方式 1: 指定路径
g++ myprogram.cpp ^
    -IF:\source_code\backtrader_cpp\include ^
    -LF:\source_code\backtrader_cpp ^
    -lbacktrader_core ^
    -o myprogram.exe

# 方式 2: 复制导入库到项目
copy F:\source_code\backtrader_cpp\libbacktrader_core.dll.a myproject\lib\
g++ myprogram.cpp -Imyproject\include -Lmyproject\lib -lbacktrader_core
```

### 运行你的程序时

```batch
# 方式 1: DLL 在同目录
myproject\
├── myprogram.exe
└── libbacktrader_core.dll  ← 必须在这里

# 方式 2: DLL 在 PATH
set PATH=%PATH%;F:\source_code\backtrader_cpp
myprogram.exe

# 方式 3: 复制 DLL 到系统目录（不推荐）
copy libbacktrader_core.dll C:\Windows\System32\
```

## ⚠️ 常见错误

### 错误 1: 链接时找不到库

```
undefined reference to `backtrader::SMA::calculate()'
```

**原因：** 链接时没有指定 `.dll.a` 文件

**解决：**
```batch
g++ myprogram.o -L. -lbacktrader_core
```

### 错误 2: 运行时找不到 DLL

```
无法启动此程序，因为计算机中丢失 libbacktrader_core.dll
```

**原因：** 运行时找不到 `.dll` 文件

**解决：**
```batch
# 方法 1
copy libbacktrader_core.dll 到可执行文件目录

# 方法 2
set PATH=%PATH%;DLL所在目录
```

### 错误 3: 混淆两个文件的用途

❌ **错误做法：**
```batch
# 链接时使用 .dll（错误！）
g++ myprogram.o libbacktrader_core.dll -o myprogram.exe

# 运行时复制 .dll.a（多余！）
copy libbacktrader_core.dll.a myprogram目录\
```

✅ **正确做法：**
```batch
# 链接时使用 .dll.a
g++ myprogram.o -L. -lbacktrader_core -o myprogram.exe

# 运行时提供 .dll
copy libbacktrader_core.dll myprogram目录\
```

## 🎓 深入理解

### 为什么要分成两个文件？

这是 Windows 操作系统的设计决定，有几个原因：

1. **安全性**
   - DLL 可以被多个程序共享
   - 导入库防止直接操作 DLL

2. **灵活性**
   - 可以更新 DLL 而不重新编译程序
   - 导入库提供版本兼容性

3. **性能**
   - 链接时只需要读取小的导入库
   - 运行时才加载大的 DLL

4. **内存效率**
   - 多个程序可以共享同一个 DLL
   - 节省内存空间

### 与 Linux/macOS 的对比

**Linux (.so)：**
```bash
libbacktrader_core.so      # 动态库（一个文件就够了）
```

**macOS (.dylib)：**
```bash
libbacktrader_core.dylib   # 动态库（一个文件就够了）
```

**Windows (.dll)：**
```batch
libbacktrader_core.dll     # 动态库
libbacktrader_core.dll.a   # 导入库（额外需要）
```

**为什么 Windows 需要两个文件？**
- Linux/macOS: 动态库本身就包含符号表
- Windows: 将符号表分离到导入库中（历史设计）

## 📝 最佳实践

### 1. 开发环境

```
项目目录/
├── include/                    # 头文件
├── lib/
│   ├── libbacktrader_core.dll.a  # 链接时需要
│   └── libbacktrader_core.a      # 静态库（可选）
└── bin/
    ├── myprogram.exe
    └── libbacktrader_core.dll      # 运行时需要
```

### 2. 发布程序

**包含在安装包中：**
- ✅ `myprogram.exe`
- ✅ `libbacktrader_core.dll`
- ❌ `libbacktrader_core.dll.a`（用户不需要）

### 3. 版本控制（Git）

**应该提交：**
```gitignore
# 不提交编译产物
*.dll
*.dll.a
*.a
*.exe
```

**应该保留：**
- 源代码
- 构建脚本（build.bat）
- CMakeLists.txt

## 🔍 检查工具

### 查看 DLL 导出的符号

```batch
# 使用 objdump
objdump -p libbacktrader_core.dll | findstr "Export"

# 使用 nm
nm -D libbacktrader_core.dll
```

### 查看导入库的符号

```batch
nm libbacktrader_core.dll.a
```

### 查看可执行文件依赖的 DLL

```batch
# 使用 objdump
objdump -p myprogram.exe | findstr "DLL Name"

# 输出示例：
# DLL Name: libbacktrader_core.dll
# DLL Name: libstdc++-6.dll
# DLL Name: libgcc_s_seh-1.dll
```

## 📖 总结

| 文件 | 用途 | 何时需要 | 包含内容 | 大小 |
|------|------|---------|---------|------|
| `.dll` | 动态库 | 运行时 | 实际代码 | 大 (~38MB) |
| `.dll.a` | 导入库 | 编译时 | 符号表 | 小 (~8MB) |

**记住：**
- 🔗 编译/链接时 → 使用 `.dll.a`
- 🏃 运行时 → 需要 `.dll`
- 📦 发布时 → 只需包含 `.dll`

---

**延伸阅读：**
- [Microsoft: DLLs in Visual C++](https://docs.microsoft.com/en-us/cpp/build/dlls-in-visual-cpp)
- [GCC: Building a shared library](https://gcc.gnu.org/wiki/Visibility)
- [MinGW-w64 Wiki](https://www.mingw-w64.org/documentation/)

