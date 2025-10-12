# 更新日志

## [2024-10-12] 构建系统优化

### ✨ 新功能

#### 1. 支持动态库和静态库构建
- **静态库**: `libbacktrader.a` (~75 MB)
- **动态库**: `libbacktrader.dll` (~38 MB) + `libbacktrader.dll.a` (~8 MB)
- **灵活构建**: 可选择编译静态库、动态库或同时编译

#### 2. 完整的警告系统
- ✅ 移除所有警告抑制（`-Wno-*`）
- ✅ 启用完整警告级别：`-Wall -Wextra`
- ✅ 所有警告记录到日志文件
- ✅ 便于识别和修复代码问题

#### 3. 改进的构建脚本

**build.bat 选项：**
```batch
build.bat           # 编译静态库
build.bat --shared  # 编译动态库
build.bat --both    # 同时编译两种库
build.bat --clean   # 清理重建
```

#### 4. 详细的日志系统
- 每次构建生成带时间戳的日志文件
- 记录所有编译输出和警告
- 便于调试和问题追踪

### 🔧 技术改进

#### CMakeLists.txt 更新
```cmake
# 添加库类型选项
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)

# 启用完整警告
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# 移除警告抑制
# 之前: -Wno-overloaded-virtual -Wno-unused-parameter ...
# 现在: 无抑制
```

#### 构建目录结构
```
build/
├── static/
│   └── libbacktrader.a
├── shared/
│   ├── libbacktrader.dll
│   └── libbacktrader.dll.a
└── build_*.log
```

### 📊 当前警告统计

编译后发现的主要警告类型：
1. **`-Wunused-parameter`**: 未使用的函数参数
2. **`-Woverloaded-virtual`**: 虚函数重载隐藏问题
3. **`-Wunused-variable`**: 未使用的变量
4. **`-Wsign-compare`**: 有符号/无符号比较
5. **`-Wreorder`**: 成员初始化顺序

### 🎯 下一步计划

1. **修复所有编译警告**
   - 优先级: unused-parameter → overloaded-virtual → 其他
   - 预计需要修改的文件: `lineroot.h`, `indicator.h` 等

2. **代码质量改进**
   - 使用静态分析工具
   - 添加更多单元测试
   - 改进文档

3. **性能优化**
   - Release 构建优化
   - 内存使用优化

### 📚 文档更新

新增文档：
- **BUILD_GUIDE.md**: 完整的构建指南
- **BUILD_AND_TEST_GUIDE.md**: 构建和测试流程
- **CHANGELOG.md**: 更新日志（本文件）

### 🐛 已知问题

1. 存在大量编译警告（约数百条）
   - 这是预期的，因为之前所有警告都被抑制了
   - 需要逐步修复

2. 动态库运行时路径问题
   - Windows 需要 DLL 在 PATH 或可执行文件同目录
   - 已在文档中提供解决方案

### ⚙️ 环境要求

- **CMake**: 3.16+
- **编译器**: GCC 8.0+ (MinGW-w64 on Windows)
- **Make**: mingw32-make or make
- **操作系统**: Windows 10/11

### 📖 使用示例

#### 编译静态库
```batch
F:\source_code\backtrader_cpp> build.bat
[SUCCESS] 核心库构建完成
[INFO] 构建产物:
  √ 静态库: libbacktrader.a
```

#### 编译动态库
```batch
F:\source_code\backtrader_cpp> build.bat --shared
[SUCCESS] 核心库构建完成
[INFO] 构建产物:
  √ 动态库: libbacktrader.dll
  √ 导入库: libbacktrader.dll.a
```

#### 同时编译两种库
```batch
F:\source_code\backtrader_cpp> build.bat --both
[SUCCESS] 核心库构建完成
[INFO] 构建产物:
  √ 静态库: libbacktrader.a
  √ 动态库: libbacktrader.dll
  √ 导入库: libbacktrader.dll.a
```

#### 查看编译警告
```batch
F:\source_code\backtrader_cpp> findstr /i "warning" build\build_*.log
F:/source_code/backtrader_cpp/include/lineroot.h:70:33: warning: unused parameter 'value' [-Wunused-parameter]
F:/source_code/backtrader_cpp/include/lineroot.h:71:33: warning: unused parameter 'size' [-Wunused-parameter]
...
```

### 🙏 贡献

欢迎贡献代码来修复警告和改进代码质量！

优先修复的警告：
1. unused-parameter（最容易修复）
2. overloaded-virtual（需要仔细设计）
3. 其他类型警告

---

**版本**: 1.0.0  
**日期**: 2024-10-12  
**作者**: Backtrader C++ Team

