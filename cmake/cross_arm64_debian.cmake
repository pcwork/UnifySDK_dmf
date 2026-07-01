# 交叉编译目标系统
set(CMAKE_SYSTEM_NAME Linux)
#set(CMAKE_SYSTEM_PROCESSOR aarch64)
set(CMAKE_SYSTEM_VERSION 1)
#set(CONFIGURE_HOST aarch64-linux)
set(CMAKE_SYSTEM_PROCESSOR arm64)
set(CONFIGURE_HOST arm64-linux)
# this one not so much
set(CMAKE_SYSTEM_VERSION 1)

# 目标三元组
set(triple aarch64-linux-gnu)

# 交叉编译器选择：clang / gcc
if(CMAKE_CLANG)
  set(CMAKE_C_COMPILER clang)
  set(CMAKE_C_COMPILER_TARGET ${triple})
  set(CMAKE_CXX_COMPILER clang++)
  set(CMAKE_CXX_COMPILER_TARGET ${triple})
else()
  set(CMAKE_C_COMPILER ${triple}-gcc)
  set(CMAKE_CXX_COMPILER ${triple}-g++)
endif()

# Debian打包架构
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE arm64)
set(CPACK_FILE_NAME_EXTRA bullseye)

# ========== 新增：交叉编译核心路径隔离 ==========
# 第三方ARM库统一安装根目录（可自行修改路径）
set(ARM_SYSROOT "/opt/arm64-sysroot")

# CMake搜索根路径：只在这些目录下找库、头文件、配置包
set(CMAKE_FIND_ROOT_PATH ${ARM_SYSROOT})

# 查找规则：程序找宿主机，库/头文件只找目标架构
# 指定 sysroot（让编译器和链接器在 sysroot 中查找头/库）
set(CMAKE_SYSROOT /opt/arm64-sysroot)
set(CMAKE_FIND_ROOT_PATH ${CMAKE_SYSROOT})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)


# ========== 修正：pkg-config配置 ==========
# 使用宿主机pkg-config，限定只搜索目标架构的pc文件
set(PKG_CONFIG_EXECUTABLE "/usr/bin/pkg-config" CACHE PATH "pkg-config")
set(ENV{PKG_CONFIG_LIBDIR} "${ARM_SYSROOT}/usr/lib/aarch64-linux-gnu/pkgconfig:${ARM_SYSROOT}/usr/share/pkgconfig")
set(ENV{PKG_CONFIG_SYSROOT_DIR} "${ARM_SYSROOT}")

# 构建时宿主工具（doxygen/plantuml/python等）允许搜索宿主机系统路径
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# 针对 find_file 类的宿主工具，显式追加宿主机系统搜索路径
#set(CMAKE_SYSTEM_PREFIX_PATH "/usr/local;/usr;${CMAKE_FIND_ROOT_PATH}")