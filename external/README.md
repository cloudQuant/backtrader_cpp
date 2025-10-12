# External Dependencies

本目录包含项目的外部依赖。

## GoogleTest

GoogleTest 作为 Git Submodule 管理。

### 初始化方法

#### 方法 1: 自动初始化（推荐）

运行测试脚本会自动检查并初始化 GoogleTest：

```batch
cd F:\source_code\backtrader_cpp
.\run_tests_win.bat
```

#### 方法 2: 手动初始化

```bash
cd F:\source_code\backtrader_cpp
git submodule update --init --recursive external/googletest
```

#### 方法 3: 直接克隆

如果不想使用 submodule，可以直接克隆：

```bash
cd F:\source_code\backtrader_cpp\external
git clone -b v1.14.0 https://github.com/google/googletest.git
```

### 目录结构

```
external/
├── .gitkeep
├── README.md
└── googletest/        # Git Submodule (v1.14.0)
    ├── googlemock/
    ├── googletest/
    └── CMakeLists.txt
```

### 版本信息

- **GoogleTest 版本**: v1.14.0
- **仓库地址**: https://github.com/google/googletest.git
- **文档**: https://google.github.io/googletest/

## 优势

使用 Git Submodule 管理依赖的优势：

1. **版本固定**: 锁定特定版本，保证构建一致性
2. **离线编译**: 初始化后无需网络即可编译
3. **快速编译**: 避免每次都重新下载
4. **易于管理**: 可以轻松切换或更新版本

