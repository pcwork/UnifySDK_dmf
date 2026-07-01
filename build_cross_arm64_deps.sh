#!/bin/bash
set -euo pipefail

# ========== CLI 参数解析 ==========
FORCE=false
DEBUG=false
PARALLEL=$(nproc)

print_usage() {
  cat <<EOF
Usage: $0 [--force] [--debug] [--jobs N]
  --force    : ignore .successful markers and rebuild everything
  --debug    : enable shell debug output (set -x)
  --jobs N   : set parallel build jobs (overrides auto-detected nproc)
EOF
}

# simple arg parse
while [ $# -gt 0 ]; do
  case "$1" in
    --force) FORCE=true; shift ;;
    --debug) DEBUG=true; shift ;;
    --jobs)
      shift
      if [[ $# -eq 0 ]]; then
        echo "ERROR: --jobs requires a numeric argument"
        print_usage
        exit 1
      fi
      PARALLEL="$1"
      shift
      ;;
    -h|--help) print_usage; exit 0 ;;
    *) echo "Unknown arg: $1"; print_usage; exit 1 ;;
  esac
done

# enable debug if requested
if [ "$DEBUG" = true ]; then
  set -x
fi

# ========== 全局配置 ==========
SYSROOT="/opt/arm64-sysroot"
CROSS_PREFIX="aarch64-linux-gnu"
BUILD_DIR="$HOME/arm64_build_cache"
ARCH="arm64"

# 库版本（稳定版）
ZLIB_VERSION="1.3.1"
OPENSSL_VERSION="3.0.15"
NCURSES_VERSION="6.4"
READLINE_VERSION="8.2"
LIBEDIT_VERSION="20240517-3.1"
SQLITE_VERSION="3450300"
YAML_CPP_VERSION="0.8.0"
BOOST_VERSION="1.83.0"
BOOST_UNDERLINE="1_83_0"
CJSON_VERSION="1.7.19"   # 可改为你想要的稳定版本
MOSQUITTO_VERSION="2.0.23"
PROTOBUF_VERSION="35.1"

echo "Build parallelism: ${PARALLEL}"
echo "Force rebuild: ${FORCE}"
echo "Debug: ${DEBUG}"

# ========== 前置检查 ==========
echo "==> 检查交叉编译器..."
if ! command -v ${CROSS_PREFIX}-gcc &>/dev/null; then
    echo "错误：未找到 ${CROSS_PREFIX}-gcc，请先修复交叉编译器"
    exit 1
fi
${CROSS_PREFIX}-gcc --version | head -1

echo "==> 检查宿主机构建工具..."
command -v protoc &>/dev/null || echo "提示：宿主机建议安装 protobuf-compiler，用于 proto 代码生成"

# 创建目录
sudo mkdir -p "$SYSROOT"
sudo chown "$USER:$USER" "$SYSROOT"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

# 如果 --force，删除所有成功标记
if [ "$FORCE" = true ]; then
  echo "==> --force: removing all .successful markers in ${BUILD_DIR}"
  rm -f "${BUILD_DIR}"/*_*.successful || true
fi

# 导出编译环境
export CC=${CROSS_PREFIX}-gcc
export CXX=${CROSS_PREFIX}-g++
export AR=${CROSS_PREFIX}-ar
export RANLIB=${CROSS_PREFIX}-ranlib
export CFLAGS="-fPIC -O2"
export CXXFLAGS="-fPIC -O2"
export PKG_CONFIG_SYSROOT_DIR="$SYSROOT"
export PKG_CONFIG_LIBDIR="$SYSROOT/lib/pkgconfig"

# helper: cd with check
cd_or_fail() {
  target="$1"
  cd "$target" || { echo "ERROR: failed to cd into $target"; exit 1; }
}

# helper: success file handling
success_file() {
  # args: pkgname version
  printf "%s/%s_%s.successful" "$BUILD_DIR" "$1" "$2"
}

check_and_skip() {
  # args: pkgname version
  sf=$(success_file "$1" "$2")
  if [ -f "$sf" ]; then
    echo "==> [SKIP] $1 $2 already built (marker: $sf)"
    return 0
  fi
  return 1
}

mark_success() {
  # args: pkgname version
  sf=$(success_file "$1" "$2")
  touch "$sf"
  echo "==> [OK] Marked $1 $2 successful ($sf)"
}

# Usage:
#   install_deb_to_sysroot PKG VER
# Requires global variables:
#   SYSROOT  - path to target sysroot (e.g. /opt/arm64-sysroot)
#   ARCH     - target architecture (e.g. arm64)
# Optional helper functions in caller script:
#   cd_or_fail DIR
#   mark_success PKG VER
install_deb_to_sysroot() {
  local PKG="$1"
  local VER="$2"
  local TMPDIR
  TMPDIR="$(mktemp -d)" || { echo "ERROR: mktemp failed"; return 1; }

#  echo ">>> Add ${PKG} (${VER}) for ${ARCH} into ${SYSROOT}"

  # Method 1: apt-get download and extract
  if ! check_and_skip "$PKG" "$VER"; then
    if command -v apt-get >/dev/null 2>&1; then
      echo ">>> Trying apt-get download ${PKG}:${ARCH}=${VER}"
      pushd "${TMPDIR}" >/dev/null || { echo "ERROR: pushd failed"; rm -rf "${TMPDIR}"; return 1; }

      # allow multiarch downloads (no install)
      dpkg --add-architecture "${ARCH}" 2>/dev/null || true
      sudo apt-get update -qq || true

      if sudo apt-get download "${PKG}:${ARCH}=${VER}" >/dev/null 2>&1; then
        echo ">>> Download succeeded, extracting .deb into ${SYSROOT}"
        for deb in *.deb; do
          [ -f "$deb" ] || continue
          dpkg-deb -x "$deb" "${TMPDIR}/pkg" || { echo "ERROR: dpkg-deb failed on $deb"; popd >/dev/null; rm -rf "${TMPDIR}"; return 1; }
          rsync -a "${TMPDIR}/pkg/" "${SYSROOT}/" || { echo "ERROR: rsync failed"; popd >/dev/null; rm -rf "${TMPDIR}"; return 1; }
          rm -rf "${TMPDIR}/pkg"
        done
        popd >/dev/null
        rm -rf "${TMPDIR}"
        echo ">>> ${PKG} installed into ${SYSROOT} via apt .deb"
        # call optional helpers if present
        if type cd_or_fail >/dev/null 2>&1; then cd_or_fail "$BUILD_DIR"; fi
        if type mark_success >/dev/null 2>&1; then mark_success "$PKG" "$VER"; fi
        return 0
      else
        echo ">>> apt-get download failed for ${PKG}:${ARCH}, will fallback to source build"
        popd >/dev/null
      fi
    fi
    # If we reach here, apt download failed or apt not available
    echo ">>> apt-get not available or download failed for ${PKG}:${ARCH}"
    rm -rf "${TMPDIR}"
    return 2
  fi

  return 0
}

# ========== 1. zlib 基础压缩库 ==========
PKG="zlib"
VER="${ZLIB_VERSION}"
if ! check_and_skip "$PKG" "$VER"; then
  echo "==> [1/9] 编译 zlib ${ZLIB_VERSION}"
  ZLIB_DIR="zlib-${ZLIB_VERSION}"
  if [ ! -d "$ZLIB_DIR" ]; then
      wget -c --tries=3 --timeout=30 "https://github.com/madler/zlib/releases/download/v${ZLIB_VERSION}/zlib-${ZLIB_VERSION}.tar.gz"
      tar xf "zlib-${ZLIB_VERSION}.tar.gz"
  fi
  cd_or_fail "$ZLIB_DIR"
  if ./configure --prefix="$SYSROOT" --static --cross-prefix=${CROSS_PREFIX}-; then
    :
  else
    ./configure --prefix="$SYSROOT" --static
  fi
  make -j${PARALLEL}
  make install
  cd_or_fail "$BUILD_DIR"
  mark_success "$PKG" "$VER"
fi

# ========== 2. OpenSSL ==========
PKG="openssl"
VER="${OPENSSL_VERSION}"
if ! check_and_skip "$PKG" "$VER"; then
  echo "==> [2/9] 编译 OpenSSL ${OPENSSL_VERSION}"
  OPENSSL_DIR="openssl-${OPENSSL_VERSION}"
  if [ ! -d "$OPENSSL_DIR" ]; then
      wget -c --tries=3 --timeout=30 "https://www.openssl.org/source/openssl-${OPENSSL_VERSION}.tar.gz"
      tar xf "openssl-${OPENSSL_VERSION}.tar.gz"
  fi
  cd_or_fail "$OPENSSL_DIR"
  ./Configure linux-aarch64 \
      --prefix="$SYSROOT" \
      --openssldir="$SYSROOT/ssl" \
      no-shared no-tests \
      -fPIC

  if ! make -j${PARALLEL} build_libs; then
      echo "ERROR: OpenSSL build_libs failed in $(pwd)"
      exit 1
  fi
  make install_dev
  cd_or_fail "$BUILD_DIR"
  mark_success "$PKG" "$VER"
fi

# ========== 3. ncurses (readline 依赖) ==========
#PKG="ncurses"
#VER="${NCURSES_VERSION}"
#if ! check_and_skip "$PKG" "$VER"; then
#  echo "==> [3/9] 编译 ncurses ${NCURSES_VERSION}"
#  NCURSES_DIR="ncurses-${NCURSES_VERSION}"
#  if [ ! -d "$NCURSES_DIR" ]; then
#      wget -c --tries=3 --timeout=30 "https://ftp.gnu.org/gnu/ncurses/ncurses-${NCURSES_VERSION}.tar.gz"
#      tar xf "ncurses-${NCURSES_VERSION}.tar.gz"
#  fi
#  cd_or_fail "$NCURSES_DIR"
#  ./configure \
#      --host=${CROSS_PREFIX} \
#      --prefix="$SYSROOT" \
#      --enable-static --disable-shared \
#      --without-manpages --without-progs --without-tests \
#      --with-termlib
#  make -j${PARALLEL}
#  make install
#  cd_or_fail "$BUILD_DIR"
#  mark_success "$PKG" "$VER"
#fi
PKG="libncurses-dev"
VER="6.2+20201114-2+deb11u2"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"

## --- 方法 1：apt 下载并解包（推荐） ---
#if command -v apt-get >/dev/null 2>&1; then
#  echo ">>> Trying apt-get download ${PKG}:${ARCH}"
#  pushd "${TMPDIR}" >/dev/null || exit 1
#
#  # 确保允许 multiarch 下载 arm64 包（不会安装到宿主）
#  dpkg --add-architecture ${ARCH} 2>/dev/null || true
#  sudo apt-get update -qq || true
#
#  if sudo apt-get download "${PKG}:${ARCH}" >/dev/null 2>&1; then
#    echo ">>> Download succeeded, extracting .deb into ${SYSROOT}"
#    for deb in ${PKG}_*.deb *.deb; do
#      [ -f "$deb" ] || continue
#      dpkg-deb -x "$deb" "${TMPDIR}/pkg"
#      rsync -a "${TMPDIR}/pkg/" "${SYSROOT}/"
#      rm -rf "${TMPDIR}/pkg"
#    done
#    popd >/dev/null
#    rm -rf "${TMPDIR}"
#    echo ">>> ${PKG} installed into ${SYSROOT} via apt .deb"
#    # 也建议确保 libtinfo / libbsd / libedit2 存在，若缺失可 apt-get download 它们
#    cd_or_fail "$BUILD_DIR"
#    mark_success "$PKG" "$VER"
#  else
#    echo ">>> apt-get download failed for ${PKG}:${ARCH}, will fallback to source build"
#    popd >/dev/null
#  fi
#fi

install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}
PKG="libncursesw6"
VER="6.2+20201114-2+deb11u2"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

PKG="libncurses6"
VER="6.2+20201114-2+deb11u2"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

PKG="libtinfo6"
VER="6.2+20201114-2+deb11u2"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

PKG="libc6"
VER="2.31-13+deb11u14"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

PKG="libc6-dev"
VER="2.31-13+deb11u14"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

PKG="libcrypt-dev"
VER="1:4.4.18-4"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

PKG="libcrypt1"
VER="1:4.4.18-4"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

PKG="libnsl-dev"
VER="1.3.0-2"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}


PKG="libnsl2"
VER="1.3.0-2"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

PKG="libtirpc-dev"
VER="1.3.1-1+deb11u1"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

PKG="libtirpc3"
VER="1.3.1-1+deb11u1"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

PKG="libgcc-s1"
VER="10.2.1-6"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

# ========== 4. readline ==========
PKG="readline"
VER="${READLINE_VERSION}"
if ! check_and_skip "$PKG" "$VER"; then
  echo "==> [4/9] 编译 readline ${READLINE_VERSION}"
  READLINE_DIR="readline-${READLINE_VERSION}"
  if [ ! -d "$READLINE_DIR" ]; then
      wget -c --tries=3 --timeout=30 "https://ftp.gnu.org/gnu/readline/readline-${READLINE_VERSION}.tar.gz"
      tar xf "readline-${READLINE_VERSION}.tar.gz"
  fi
  cd_or_fail "$READLINE_DIR"
  ./configure \
      --host=${CROSS_PREFIX} \
      --prefix="$SYSROOT" \
      --enable-static --disable-shared \
      --with-curses="$SYSROOT"
  make -j${PARALLEL}
  make install
  cd_or_fail "$BUILD_DIR"
  mark_success "$PKG" "$VER"
fi

# ========== 5. libedit ==========
#PKG="libedit"
#VER="${LIBEDIT_VERSION}"
#if ! check_and_skip "$PKG" "$VER"; then
#  echo "==> [5/9] 编译 libedit ${LIBEDIT_VERSION}"
#  LIBEDIT_DIR="libedit-${LIBEDIT_VERSION}"
#  if [ ! -d "$LIBEDIT_DIR" ]; then
#      wget -c --tries=3 --timeout=30 "https://thrysoee.dk/editline/libedit-${LIBEDIT_VERSION}.tar.gz"
#      tar xf "libedit-${LIBEDIT_VERSION}.tar.gz"
#  fi
#  cd_or_fail "$LIBEDIT_DIR"
#
#  export CPPFLAGS="-I${SYSROOT}/include -I${SYSROOT}/include/ncurses"
#  export LDFLAGS="-L${SYSROOT}/lib"
#  ./configure \
#      --host=${CROSS_PREFIX} \
#      --prefix="$SYSROOT" \
#      --enable-static --disable-shared \
#      CPPFLAGS="${CPPFLAGS}" LDFLAGS="${LDFLAGS}" LIBS="-lncurses"
#
#  make -j${PARALLEL}
#  make install
#  cd_or_fail "$BUILD_DIR"
#  mark_success "$PKG" "$VER"
#fi

# --- BEGIN: install libedit-dev into arm64 sysroot ---
# 说明：优先尝试 apt 下载 libedit-dev:arm64 并解包到 SYSROOT，
# 回退方案：源码交叉编译并安装到 SYSROOT。

PKG="libedit-dev"
VER="3.1-20191231-2+b1"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

PKG="libedit2"
VER="3.1-20191231-2+b1"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

PKG="libbsd-dev"
VER="0.11.3-1+deb11u1"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

PKG="libbsd0"
VER="0.11.3-1+deb11u1"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

PKG="libmd0"
VER="1.0.3-3"
#echo ">>> Add ${PKG} for ${ARCH} into ${SYSROOT}"
install_deb_to_sysroot ${PKG} ${VER} || {
  echo "apt download ${PKG}failed, fallback to source build or handle error"
}

# ========== 6. SQLite3 ==========
PKG="sqlite3"
VER="${SQLITE_VERSION}"
if ! check_and_skip "$PKG" "$VER"; then
  echo "==> [6/9] 编译 SQLite3"
  SQLITE_DIR="sqlite-autoconf-${SQLITE_VERSION}"
  if [ ! -d "$SQLITE_DIR" ]; then
      wget -c --tries=3 --timeout=30 "https://www.sqlite.org/2024/sqlite-autoconf-${SQLITE_VERSION}.tar.gz"
      tar xf "sqlite-autoconf-${SQLITE_VERSION}.tar.gz"
  fi
  cd_or_fail "$SQLITE_DIR"
  ./configure \
      --host=${CROSS_PREFIX} \
      --prefix="$SYSROOT" \
      --enable-static --disable-shared \
      --disable-readline --disable-shell
  make -j${PARALLEL}
  make install
  cd_or_fail "$BUILD_DIR"
  mark_success "$PKG" "$VER"
fi

# ========== 7. yaml-cpp ==========
PKG="yaml-cpp"
VER="${YAML_CPP_VERSION}"
if ! check_and_skip "$PKG" "$VER"; then
  echo "==> [7/9] 编译 yaml-cpp ${YAML_CPP_VERSION}"
  YAML_DIR="yaml-cpp-${YAML_CPP_VERSION}"
  if [ ! -d "$YAML_DIR" ]; then
      wget -c --tries=3 --timeout=30 "https://github.com/jbeder/yaml-cpp/archive/refs/tags/${YAML_CPP_VERSION}.tar.gz" -O "yaml-cpp-${YAML_CPP_VERSION}.tar.gz"
      tar xf "yaml-cpp-${YAML_CPP_VERSION}.tar.gz"
  fi
  cd_or_fail "$YAML_DIR"
  rm -rf build_arm64 && mkdir build_arm64 && cd build_arm64
  cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER=${CROSS_PREFIX}-gcc \
      -DCMAKE_CXX_COMPILER=${CROSS_PREFIX}-g++ \
      -DCMAKE_INSTALL_PREFIX="$SYSROOT" \
      -DYAML_BUILD_SHARED_LIBS=OFF \
      -DYAML_CPP_BUILD_TESTS=OFF \
      -DYAML_CPP_BUILD_TOOLS=OFF \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  make -j${PARALLEL}
  make install
  cd_or_fail "$BUILD_DIR"
  mark_success "$PKG" "$VER"
fi

# ========== 8. Boost 全组件 ==========
PKG="boost"
VER="${BOOST_VERSION}"
if ! check_and_skip "$PKG" "$VER"; then
  echo "==> [8/9] 编译 Boost ${BOOST_VERSION}"
  BOOST_DIR="${PKG}-${VER}"
  if [ ! -d "$BOOST_DIR" ]; then
      wget -c --tries=3 --timeout=30 "https://github.com/boostorg/boost/releases/download/${PKG}-${VER}/${PKG}-${VER}.tar.xz"
      tar xf "${PKG}-${VER}.tar.xz"
  fi
  cd_or_fail "$BOOST_DIR"
  cat > user-config.jam << EOF
using gcc : arm64 : ${CROSS_PREFIX}-g++ ;
EOF

  ./bootstrap.sh --prefix="$SYSROOT" --without-libraries=python,mpi,graph_parallel

  ./b2 \
      --user-config=user-config.jam \
      toolset=gcc-arm64 \
      target-os=linux \
      architecture=arm \
      address-model=64 \
      link=static,shared \
      runtime-link=shared \
      threading=multi \
      cxxflags="-fPIC" \
      --with-filesystem \
      --with-log \
      --with-program_options \
      --with-system \
      --with-thread \
      --with-chrono \
      --with-regex \
      --with-date_time \
      --prefix="$SYSROOT" \
      install

  cd_or_fail "$BUILD_DIR"
  mark_success "$PKG" "$VER"
fi

# ========== 安装 cJSON  ==========
PKG="cJSON"
VER="${CJSON_VERSION}"
CJSON_DIR="${PKG}-${VER}"
if ! check_and_skip "$PKG" "$VER"; then
	if [ ! -d "$CJSON_DIR" ]; then
	  wget -c --tries=3 --timeout=30 "https://github.com/DaveGamble/cJSON/archive/refs/tags/v${VER}.tar.gz" -O "${PKG}-${VER}.tar.gz"
	  tar xf "${PKG}-${VER}.tar.gz"
	fi
	cd_or_fail "$CJSON_DIR"
	mkdir -p build && cd build
	cmake .. \
	  -DCMAKE_BUILD_TYPE=Release \
	  -DCMAKE_INSTALL_PREFIX="$SYSROOT" \
	  -DBUILD_SHARED_LIBS=OFF \
	  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
	  -DCMAKE_C_COMPILER=${CROSS_PREFIX}-gcc
	make -j${PARALLEL}
	make install
	cd_or_fail "$BUILD_DIR"

	# Ensure header path matches mosquitto expectation (<cjson/cJSON.h>)
	if [ -f "${SYSROOT}/include/cJSON.h" ] && [ ! -f "${SYSROOT}/include/cjson/cJSON.h" ]; then
	  mkdir -p "${SYSROOT}/include/cjson"
	  cp "${SYSROOT}/include/cJSON.h" "${SYSROOT}/include/cjson/cJSON.h"
	  echo "Created ${SYSROOT}/include/cjson/cJSON.h for mosquitto"
	fi

	mark_success "$PKG" "$VER"
fi

# ========== 9. mosquitto 库 ==========
PKG="mosquitto"
VER="${MOSQUITTO_VERSION}"
if ! check_and_skip "$PKG" "$VER"; then
  echo "==> [9/9] 编译 mosquitto ${MOSQUITTO_VERSION}"
  MOSQ_DIR="mosquitto-${MOSQUITTO_VERSION}"
  if [ ! -d "$MOSQ_DIR" ]; then
      wget -c --tries=3 --timeout=30 "https://mosquitto.org/files/source/mosquitto-${MOSQUITTO_VERSION}.tar.gz"
      tar xf "mosquitto-${MOSQUITTO_VERSION}.tar.gz"
  fi
  cd_or_fail "$MOSQ_DIR"

  export CPPFLAGS="-I${SYSROOT}/include"
  export CFLAGS="-fPIC ${CPPFLAGS}"
  export LDFLAGS="-L${SYSROOT}/lib"
#  export LIBS="-ltinfo -lncurses"

  make -j${PARALLEL} \
      CC=${CROSS_PREFIX}-gcc \
      CXX=${CROSS_PREFIX}-g++ \
      AR=${CROSS_PREFIX}-ar \
      CPPFLAGS="${CPPFLAGS}" \
      CFLAGS="${CFLAGS}" \
      LDFLAGS="${LDFLAGS}" \
      prefix="/" \
	  DESTDIR="${SYSROOT}" \
      install
  cd_or_fail "$BUILD_DIR"
  mark_success "$PKG" "$VER"
fi

# ===== 插入 nlohmann_json 安装段 =====
PKG="nlohmann_json"
VER="3.12.0"
if ! check_and_skip "$PKG" "$VER"; then
  echo "==> 编译并安装 nlohmann_json ${VER}"
  PKG_DIR="nlohmann_json-${VER}"
  TAR1="v${VER}.tar.gz"
  TAR_URL1="https://github.com/nlohmann/json/archive/refs/tags/${TAR1}"
  TAR2="json.tar.xz"
  TAR_URL2="https://github.com/nlohmann/json/releases/download/v${VER}/${TAR2}"

  # 下载优先尝试 release tarball，失败回退到 git clone
  if [ ! -d "$PKG_DIR" ]; then
    echo "Downloading nlohmann_json ${VER}..."
    if ! (wget -c --tries=3 --timeout=30 "$TAR_URL2" -O "${TAR2}" || wget -c --tries=3 --timeout=30 "$TAR_URL1" -O "${TAR1}"); then
      echo "Release tarball not found, falling back to git clone"
      git clone --branch "v${VER}" https://github.com/nlohmann/json.git "$PKG_DIR"
    else
      # 解压下载的 tarball（优先两种可能的文件名）
      if [ -f "${TAR2}" ]; then
        tar xf "${TAR2}"
        # upstream tarball extracts to json-${VER}
        PKG_DIR="json"
      else
        tar xf "${TAR1}"
        # git archive extracts to json-${VER} or json-<tag>; normalize
        PKG_DIR="json"
      fi
    fi
  fi

  cd_or_fail "$PKG_DIR"

  # nlohmann_json 是 header-only。把头文件安装到 SYSROOT include 下。
  # 支持两种源树布局：single header in include/nlohmann or include/json.hpp
  mkdir -p "${SYSROOT}/include/nlohmann"
  if [ -f "single_include/nlohmann/json.hpp" ]; then
    cp -a single_include/nlohmann/json.hpp "${SYSROOT}/include/nlohmann/json.hpp"
    echo "Installed ${SYSROOT}/include/nlohmann/json.hpp"
  elif [ -f "include/nlohmann/json.hpp" ]; then
    cp -a include/nlohmann/json.hpp "${SYSROOT}/include/nlohmann/json.hpp"
    echo "Installed ${SYSROOT}/include/nlohmann/json.hpp"
  else
    # fallback: install whole single_include directory if present
    if [ -d "single_include/nlohmann" ]; then
      cp -a single_include/nlohmann "${SYSROOT}/include/"
      echo "Installed ${SYSROOT}/include/nlohmann/ (from single_include)"
    else
      echo "ERROR: cannot find json.hpp in source tree"
      cd_or_fail "$BUILD_DIR"
      exit 1
    fi
  fi

  # Optionally install CMake config files so find_package can work for native builds
  if [ -d "cmake" ]; then
    mkdir -p "${SYSROOT}/lib/cmake/nlohmann_json"
    cp -a cmake/* "${SYSROOT}/lib/cmake/nlohmann_json/" || true
  fi

  cd_or_fail "$BUILD_DIR"
  mark_success "$PKG" "$VER"
fi
# ===== end nlohmann_json =====

# ========== 补充：Protobuf 库 ==========
PKG="protobuf"
VER="${PROTOBUF_VERSION}"
if ! check_and_skip "$PKG" "$VER"; then
  echo "==> 补充：编译 Protobuf ${PROTOBUF_VERSION} 库"
  PROTO_DIR="${PKG}-${VER}"
  if [ ! -d "$PROTO_DIR" ]; then
      wget -c --tries=3 --timeout=30 "https://github.com/protocolbuffers/protobuf/releases/download/v${VER}/${PKG}-${VER}.tar.gz"
      tar xf "${PKG}-${VER}.tar.gz"
  fi
  cd_or_fail "$PROTO_DIR"
  rm -rf build_arm64 && mkdir build_arm64 && cd build_arm64
  cmake .. \
      -DCMAKE_BUILD_TYPE=Release \
      -DCMAKE_C_COMPILER=${CROSS_PREFIX}-gcc \
      -DCMAKE_CXX_COMPILER=${CROSS_PREFIX}-g++ \
      -DCMAKE_INSTALL_PREFIX="$SYSROOT" \
      -Dprotobuf_BUILD_SHARED_LIBS=OFF \
      -Dprotobuf_BUILD_TESTS=OFF \
      -Dprotobuf_BUILD_PROTOC_BINARIES=OFF \
      -DCMAKE_POSITION_INDEPENDENT_CODE=ON
  make -j${PARALLEL}
  make install
  cd_or_fail "$BUILD_DIR"
  mark_success "$PKG" "$VER"
fi

# ========== 最终验证 ==========
echo ""
echo "======================================"
echo "全部依赖编译完成，安装路径: $SYSROOT"
echo "核心库验证："
ls -la "$SYSROOT/lib/libboost_log.a" 2>/dev/null && echo "✅ Boost log"
ls -la "$SYSROOT/lib/libssl.a" 2>/dev/null && echo "✅ OpenSSL"
ls -la "$SYSROOT/lib/libsqlite3.a" 2>/dev/null && echo "✅ SQLite3"
ls -la "$SYSROOT/lib/libyaml-cpp.a" 2>/dev/null && echo "✅ yaml-cpp"
ls -la "$SYSROOT/lib/libcjson.a" 2>/dev/null && echo "✅ cJSON"
ls -la "$SYSROOT/lib/libmosquitto.a" 2>/dev/null && echo "✅ mosquitto"
ls -la "$SYSROOT/lib/libprotobuf.a" 2>/dev/null && echo "✅ Protobuf"
ls -la "$SYSROOT/lib/libreadline.a" 2>/dev/null && echo "✅ readline"
ls -la "$SYSROOT/lib/libedit.a" 2>/dev/null && echo "✅ libedit"
echo "======================================"
