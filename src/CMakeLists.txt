# 核心库源文件
set(BACKTRADER_CORE_SOURCES
    base/MetaClass.cpp
    base/LineBuffer.cpp
    base/LineIterator.cpp
    base/LineSeries.cpp
    base/LineOperations.cpp
    core/LineRoot.cpp
    indicators/IndicatorBase.cpp
    indicators/SMA.cpp
    indicators/EMA.cpp
    data/DataFeed.cpp
)

# 创建核心库
add_library(backtrader_core ${BACKTRADER_CORE_SOURCES})

# 设置目标属性
set_target_properties(backtrader_core PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    CXX_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN YES
)

# 链接依赖
target_link_libraries(backtrader_core
    PUBLIC
        Threads::Threads
    PRIVATE
        $<$<BOOL:${OpenMP_CXX_FOUND}>:OpenMP::OpenMP_CXX>
)

# 编译定义
target_compile_definitions(backtrader_core
    PRIVATE
        BACKTRADER_EXPORTS
    PUBLIC
        $<$<CONFIG:Debug>:BACKTRADER_DEBUG>
)

# 包含目录
target_include_directories(backtrader_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
    PRIVATE
        ${CMAKE_SOURCE_DIR}/src
)

# 编译特性
target_compile_features(backtrader_core
    PUBLIC
        cxx_std_20
)

# 预编译头（如果支持）
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
    target_precompile_headers(backtrader_core
        PRIVATE
            <memory>
            <vector>
            <string>
            <algorithm>
            <cmath>
    )
endif()