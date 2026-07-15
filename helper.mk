#!/usr/bin/make -f
# -*- makefile -*-
# ex: set tabstop=4 noexpandtab:
# -*- coding: utf-8 -*

default: help all/default
	@echo "$@: TODO: Support more than $^ by default"
	@date -u

SELF?=${CURDIR}/helper.mk

project?=unifysdk

# mirror for debootstrap
mirror_url?=https://mirrors.tuna.tsinghua.edu.cn/debian
debootstrap_keyring_dir?=/tmp/${project}-debootstrap-keyrings
debootstrap_keyring?=${debootstrap_keyring_dir}/${debian_codename}.gpg

supported_debootstrap_keyring_codenames:=bullseye bookworm trixie
debootstrap_keyring_option=$(if $(filter ${debian_codename},${supported_debootstrap_keyring_codenames}),--keyring="${debootstrap_keyring}",)

# GitHub download proxy
GITHUB_PROXY?=https://ghproxy.net/

# Allow overloading from env if needed
# VERBOSE?=1
BUILD_DEV_GUI?=OFF
BUILD_IMAGE_PROVIDER?=ON

cmake_options?=-B ${build_dir}

CMAKE_GENERATOR?=Ninja
export CMAKE_GENERATOR

build_dir?=build
sudo?=sudo

debian_codename?=bookworm

# Cross-compilation target architecture
#   arm64  -> aarch64-linux-gnu   (crossbuild-essential-arm64)
#   armhf  -> arm-linux-gnueabihf  (crossbuild-essential-armhf)
#   amd64  -> native x86-64 build (no cross toolchain needed)
target_arch?=amd64

# Derive GNU triplet and dpkg arch from target_arch
triple_arm64=aarch64-linux-gnu
triple_armhf=arm-linux-gnueabihf
triple_amd64=x86_64-linux-gnu
dpkg_arch_arm64=arm64
dpkg_arch_armhf=armhf
dpkg_arch_amd64=amd64
triple=$(if $(filter arm64,$(target_arch)),$(triple_arm64),$(if $(filter armhf,$(target_arch)),$(triple_armhf),$(triple_amd64)))
dpkg_arch=$(if $(filter arm64,$(target_arch)),$(dpkg_arch_arm64),$(if $(filter armhf,$(target_arch)),$(dpkg_arch_armhf),$(dpkg_arch_amd64)))

# CMake toolchain file for cross-compilation (empty for native amd64)
toolchain_file=$(if $(filter amd64,$(target_arch)),,cmake/$(target_arch)_debian.cmake)

# When cross-compiling, inject toolchain file into cmake_options
ifneq ($(target_arch),amd64)
cmake_options+=-DCMAKE_TOOLCHAIN_FILE=$(toolchain_file)
endif

packages?=cmake ninja-build build-essential python3-full ruby clang
packages+=git-lfs unp time file
packages+=nlohmann-json3-dev
# TODO: remove for offline build
packages+=curl wget python3-pip
packages+=time

# Extra for components, make it optional
packages+=python3-jinja2
packages+=yarnpkg

rust_url?=https://sh.rustup.rs
# Rust toolchain version to install (docker/Dockerfile L60: ENV RUST_VERSION)
RUST_VERSION?=stable
# Rust and Cargo home directories (docker/Dockerfile L62-63)
RUSTUP_HOME?=${HOME}/.rustup
CARGO_HOME?=${HOME}/.cargo
export RUSTUP_HOME
export CARGO_HOME
export PATH := ${CARGO_HOME}/bin:${PATH}
RUSTUP_DIST_SERVER?="https://mirrors.tuna.tsinghua.edu.cn/rustup"
RUSTUP_UPDATE_ROOT?="https://mirrors.tuna.tsinghua.edu.cn/rustup/rustup"

# Allow overloading from env if needed
ifdef VERBOSE
CMAKE_VERBOSE_MAKEFILE?=${VERBOSE}
cmake_options+=-DCMAKE_VERBOSE_MAKEFILE=${CMAKE_VERBOSE_MAKEFILE}
endif

ifdef BUILD_DEV_GUI
cmake_options+=-DBUILD_DEV_GUI=${BUILD_DEV_GUI}
ifeq (${BUILD_DEV_GUI}, ON)
packages+=nodejs
endif
endif

ifdef BUILD_IMAGE_PROVIDER
cmake_options+=-DBUILD_IMAGE_PROVIDER=${BUILD_IMAGE_PROVIDER}
endif

# Allow to bypass env detection, to support more build systems
ifdef CMAKE_SYSTEM_PROCESSOR
cmake_options+=-DCMAKE_SYSTEM_PROCESSOR="${CMAKE_SYSTEM_PROCESSOR}"
export CMAKE_SYSTEM_PROCESSOR
else
# CMAKE_SYSTEM_PROCESSOR?=$(shell uname -m)
endif

ifdef CARGO_TARGET_TRIPLE
cmake_options+=-DCARGO_TARGET_TRIPLE="${CARGO_TARGET_TRIPLE}"
export CMAKE_TARGET_TRIPLE
endif


help: README.md
	@cat $<
	@echo ""
	@echo "# Available rules at your own risk:"
	@grep -o '^[^ ]*:' ${SELF} | grep -v '\$$' | grep -v '^#' | grep -v '^\.'
	@echo ""
	@echo "# Environment:"
	@echo "# PATH=${PATH}"
	@echo ""

# setup/debian: Base Debian package setup with architecture-aware cross-compilation support
# Follows docker/Dockerfile L16-32 pattern:
#   1. Install host dependencies (no arch suffix)
#   2. Install extra packages
#   3. Add target dpkg architecture + install target deps with :arch suffix + crossbuild-essential (cross only)
#   4. Clean apt cache
setup/debian: ${CURDIR}/docker/target_dependencies.apt ${CURDIR}/docker/host_dependencies.apt
	cat /etc/debian_version
	-${sudo} apt-get update
	# Install host dependencies (no architecture suffix needed)
	${sudo} apt-get install -y $(shell awk '!/^\s*#/{split($$0,a,"//"); print a[1]}' ${CURDIR}/docker/host_dependencies.apt)

	# Install extra packages
	${sudo} apt-get install -y ${packages}
	# Install target dependencies: native (no suffix) vs cross (:<dpkg_arch> suffix + crossbuild-essential)
	@if [ "$(target_arch)" = "amd64" ]; then \
		${sudo} apt-get install -y $(shell awk '!/^\s*#/{split($$0,a,"//"); print a[1]}' ${CURDIR}/docker/target_dependencies.apt); \
	else \
		${sudo} dpkg --add-architecture $(dpkg_arch); \
		${sudo} apt-get update; \
		${sudo} apt-get install -y $(shell awk '!/^\s*#/{split($$0,a,"//"); print a[1]}' ${CURDIR}/docker/target_dependencies.apt | while read pkg; do echo "$${pkg}:$(dpkg_arch)"; done); \
		${sudo} apt-get install -y crossbuild-essential-$(dpkg_arch); \
	fi
	# Clean apt cache (docker/Dockerfile L32)
	${sudo} rm -rf /var/lib/apt/lists/*
	@echo "$@: TODO: Support debian stable rustc=1.96 https://tracker.debian.org/pkg/rustc"


# setup/cross: Verify cross-compilation toolchain is functional
# The actual installation (dpkg --add-arch, target deps with :arch suffix,
# crossbuild-essential) is now handled by setup/debian.
# This target only verifies the cross-compiler can be invoked.
setup/cross:
	@echo "=== setup/cross: verifying target_arch=$(target_arch) dpkg_arch=$(dpkg_arch) triple=$(triple) ==="
ifeq ($(target_arch),amd64)
	@echo "=== setup/cross: amd64 is native, nothing to verify ==="
else
	# Verify cross-compiler is functional
	@which $(triple)-gcc && $(triple)-gcc --version | head -1
	@which $(triple)-g++ && $(triple)-g++ --version | head -1
	@which $(triple)-pkg-config || echo "warning: $(triple)-pkg-config not found (may need pkg-config package)"
endif
	@echo "=== setup/cross: done ==="

# Map target_arch to Rust target triple (vendor field differs from GNU triplet)
rust_target_arm64=aarch64-unknown-linux-gnu
rust_target_armhf=armv7-unknown-linux-gnueabihf
rust_target_amd64=x86_64-unknown-linux-gnu
rust_target=$(if $(filter arm64,$(target_arch)),$(rust_target_arm64),$(if $(filter armhf,$(target_arch)),$(rust_target_armhf),$(rust_target_amd64)))


setup/cargo-mirror:
	@echo "=== Injecting cargo mirror config into nspawn container ==="
	mkdir -p /root/.cargo
	# 覆盖写入头部
	echo "[source.crates-io]" > /root/.cargo/config.toml
	echo "replace-with = \"tuna\"" >> /root/.cargo/config.toml
	echo "" >> /root/.cargo/config.toml
	echo "[source.tuna]" >> /root/.cargo/config.toml
	echo "registry = \"sparse+https://mirrors.tuna.tsinghua.edu.cn/crates.io-index/\"" >> /root/.cargo/config.toml
	echo "" >> /root/.cargo/config.toml
	echo "[net]" >> /root/.cargo/config.toml
	echo "git-fetch-with-cli = true" >> /root/.cargo/config.toml
	echo "retry = 3" >> /root/.cargo/config.toml
	echo "" >> /root/.cargo/config.toml
	echo "[http]" >> /root/.cargo/config.toml
	echo "multiplexing = true" >> /root/.cargo/config.toml
	echo "timeout = 300" >> /root/.cargo/config.toml
	# 校验文件
	# 校验文件是否写入成功
	@echo "=== cargo mirror config injected ==="


# Refer to docker/Dockerfile lines 64-76 for the Rust installation pattern
# Key fixes:
#   1. Export RUSTUP_HOME / CARGO_HOME to isolate from stale config (docker/Dockerfile L62-63)
#   2. Delete stale settings.toml to avoid 404 errors from outdated default_toolchain
#   3. Build RUST_TRIPLES with host + target triples (docker/Dockerfile L67-70)
#   4. Use stable toolchain via --default-toolchain to avoid version mismatch (docker/Dockerfile L70)
setup/rust:
	@echo "$@: Installing Rust for target_arch=$(target_arch) rust_target=$(rust_target)"
	@echo "$@: TODO: Support https://tracker.debian.org/pkg/rustup"
	# Delete stale settings.toml to prevent 404 errors from outdated default_toolchain
	# (docker has no pre-existing config; host machines may have stale settings)
	@rm -f ${RUSTUP_HOME}/settings.toml ${RUSTUP_HOME}/settings.toml.bak
	curl --insecure --proto '=https' --tlsv1.2 -sSf ${rust_url} --output /tmp/sh.rustup.rs \
		&& chmod +x /tmp/sh.rustup.rs
	# Build RUST_TRIPLES: host triple + target triple (docker/Dockerfile L67-70)
	@case "$$(uname -m)" in \
		x86_64)  HOST_TRIPLE="x86_64-unknown-linux-gnu" ;; \
		aarch64) HOST_TRIPLE="aarch64-unknown-linux-gnu" ;; \
		arm*)    HOST_TRIPLE="armv7-unknown-linux-gnueabihf" ;; \
		*)       HOST_TRIPLE="$$(uname -m)-unknown-linux-gnu" ;; \
	esac; \
	RUSTUP_UPDATE_ROOT=${RUSTUP_UPDATE_ROOT} \
    RUSTUP_DIST_SERVER=${RUSTUP_DIST_SERVER} \
	/tmp/sh.rustup.rs -y --default-toolchain ${RUST_VERSION}
	rm -f /tmp/sh.rustup.rs
	# Fix permissions to ensure toolchain is accessible (docker/Dockerfile L74-75)
	chmod -R a+rw $${RUSTUP_HOME} $${CARGO_HOME} 2>/dev/null || true
	find $${RUSTUP_HOME} $${CARGO_HOME} -type d -exec chmod a+x {} \; 2>/dev/null || true
	cat $${CARGO_HOME}/env
	. $${CARGO_HOME}/env
	@echo '$@: info: You might like to add ". $${CARGO_HOME}/env" to "$${HOME}/.bashrc"'
	-which rustc
	rustc --version
	cargo --version
	rustc --print target-list
	rustup default stable
    # Add cross target
ifeq ($(target_arch),arm64)
	${CARGO_HOME}/bin/rustup target add aarch64-unknown-linux-gnu
endif
ifeq ($(target_arch),armhf)
	${CARGO_HOME}/bin/rustup target add armv7-unknown-linux-gnueabihf
endif
	@echo "$@: TODO: https://github.com/kornelski/cargo-deb/issues/159"
	cargo install --version 0.1.15 --locked cargo2junit
	cargo install --version 3.7.0 --locked cargo-deb
	@echo "$@: TODO: Support stable version from https://releases.rs/ or older"
	@echo "$@: Installed Rust targets (rustup target list --installed):"

setup/python:
	python3 --version
	@echo "$@: TODO: https://bugs.debian.org/1094297"
	pip3 --version || echo "warning: Please install pip"
	pip3 install --upgrade pip
	# Code coverage and code review tools (refer to docker/Dockerfile L40-42)
	pip3 install gcovr==5.0 diff-cover==9.1.0
	# Template engine and its dependencies
	pip3 install pybars3==0.9.7 PyMeta3==0.5.1 xmltodict==0.12.0
	# Documentation generation (Sphinx) and related plugins
	pip3 install sphinx==5.1.0 breathe==4.34.0 myst-parser==0.18.0 \
		linkify-it-py==2.0.0 sphinxcontrib-plantuml==0.24 \
		sphinx-markdown-tables==0.0.17 sphinx-rtd-theme==1.0.0

cmake_url?=https://github.com/Kitware/CMake/releases/download/v3.21.6/cmake-3.21.6-Linux-x86_64.sh
cmake_filename?=$(shell basename -- "${cmake_url}")
cmake_sha256?=d460a33c42f248388a8f2249659ad2f5eab6854bebaf4f57c1df49ded404e593

# _cmake: Download and install CMake 3.21.6 to /usr/local (runs wherever invoked — host or container)
# Only needed for bullseye (debian-11); bookworm (debian-12) ships a modern CMake.
_cmake:
	@echo "$@: TODO: remove for debian-12+"
	curl -L ${cmake_url} -o /tmp/${cmake_filename}
	sha256sum  /tmp/${cmake_filename} \
		| grep "${cmake_sha256}"
	${SHELL} "/tmp/${cmake_filename}" \
		--prefix=/usr/local \
		--skip-license
	rm -v "/tmp/${cmake_filename}"
	cmake --version

# setup/plantuml: Download PlantUML jar for Doxygen documentation (refer to docker/Dockerfile L87-91)
setup/plantuml:
	@echo "$@: Downloading PlantUML..."
	curl -L ${GITHUB_PROXY}https://github.com/plantuml/plantuml/releases/download/v1.2022.0/plantuml-1.2022.0.jar --output /tmp/plantuml.jar
	mv /tmp/plantuml.jar /opt/plantuml.jar
	echo f1070c42b20e6a38015e52c10821a9db13bedca6b5d5bc6a6192fcab6e612691 /opt/plantuml.jar > /tmp/plantuml.jar.sha256
	sha256sum -c /tmp/plantuml.jar.sha256
	rm /tmp/plantuml.jar.sha256
	@echo "export PLANTUML_JAR_PATH=/opt/plantuml.jar"

# setup/zap: Fetch and install ZAP (ZCL Advanced Platform) for ZCL cluster generation (refer to docker/Dockerfile L94-99)
setup/zap:
	@echo "$@: Installing ZAP..."
	${sudo} apt-get update
	wget -O /tmp/zap.deb https://github.com/project-chip/zap/releases/download/v2025.01.15/zap-linux-x64.deb
	${sudo} apt-get install -y --no-install-recommends /tmp/zap.deb
	rm -f /tmp/zap.deb
	@echo "$@: done"

# setup/slc_cli: Unpack Silicon Labs Configurator CLI (refer to docker/Dockerfile L102-106)
setup/slc_cli:
	@echo "$@: Installing SLC CLI..."
	unzip -o uic-resources/linux/slc_cli_linux.zip -d /opt
	chmod +x /opt/slc_cli/slc
	@echo 'export PATH="/opt/slc_cli:$${PATH}"'

# setup/clang: Install Clang 12 toolchain via llvm.sh (optional, refer to docker/Dockerfile L108-113)
setup/clang:
	@echo "$@: Installing Clang 12 toolchain..."
	curl -sL https://apt.llvm.org/llvm.sh --output /tmp/llvm.sh
	chmod +x /tmp/llvm.sh
	${sudo} /tmp/llvm.sh 12
	rm -f /tmp/llvm.sh
	@echo "$@: done"

# setup/yarn: Install yarn package manager globally via npm (refer to docker/Dockerfile L116)
setup/yarn: setup/nodejs
	@echo "$@: Installing yarn..."
	npm install yarn -g --force
	@echo "$@: done"

setup/nodejs:
	@echo "$@: Installing Node.js 24..."
	curl -fsSL https://deb.nodesource.com/setup_24.x | sudo -E bash -
	sudo apt-get install -y nodejs
	node -v

# setup/qemu: Create dummy ld.so.cache for QEMU cross-compilation (refer to docker/Dockerfile L47-57)
setup/qemu:
	@echo "$@: Setting up QEMU ld prefix..."
	${sudo} mkdir -p /dummyroot/etc
	${sudo} touch /dummyroot/etc/ld.so.cache
	@echo 'export QEMU_LD_PREFIX=/dummyroot'

# setup/mosquitto: Fetch and build mosquitto for target arch (refer to docker/Dockerfile L44-45)
setup/mosquitto:
	@echo "$@: Building mosquitto for target_arch=$(target_arch)..."
	bash fetch_build_mosquitto.sh "$(target_arch)"
	@echo "$@: done"

setup/debian/bullseye: setup/debian setup/rust setup/python setup/plantuml setup/zap setup/nodejs setup/yarn
	date -u

setup/debian/bookworm: setup/debian setup/rust setup/python setup/plantuml setup/zap setup/yarn
	date -u

setup: setup/debian/${debian_codename}
	date -u


git/lfs/prepare:
	[ ! -r .git/lfs/objects ] \
	  || { git lfs version || echo "$@: warning: Please install git-lfs" \
	  && git lfs status --porcelain || git lfs install \
	  && time git lfs pull \
	  && git lfs update || git lfs update --force \
	  && git lfs status --porcelain \
	  && git lfs ls-files \
	  ; }

git/modules/prepare:
	[ ! -r .git/modules ] || git submodule update --init --recursive

git/prepare: git/modules/prepare git/lfs/prepare

configure: ${build_dir}/CMakeCache.txt
	file -E $<

${build_dir}/CMakeCache.txt: CMakeLists.txt
	cmake ${cmake_options}

build: ${build_dir}/CMakeCache.txt
	cmake --build ${<D} \
		|| cat ${build_dir}/CMakeFiles/CMakeOutput.log
	cmake --build ${<D}
.PHONY: build

${build_dir}/%: build
	file -E "$@"

test: ${build_dir}
	ctest --test-dir ${<}

check: test

dist: ${build_dir}
	cmake --build $< --target package
	install -d $</$@
	cp -av ${<}/*.deb $</$@

distclean:
	rm -rf ${build_dir}

prepare: git/prepare

all/default: configure prepare build test dist
	@date -u


### @rootfs is faster than docker for env check
# Usage examples:
#
# === Host machine (WSL / native Linux) ===
#
#   Full bullseye setup (bookworm uses system CMake; CMake 3.21.6 is auto-installed only inside the container):
#     make -f helper.mk setup debian_codename=bullseye target_arch=amd64
#
#   Quick start:
#     make -f helper.mk setup             # setups everything for current OS version
#     make -f helper.mk configure         # runs cmake configure
#     make -f helper.mk build             # builds the project
#     make -f helper.mk test              # runs tests
#
#   Cross-compile arm64 on host:
#     make -f helper.mk setup target_arch=arm64 debian_codename=bullseye
#     make -f helper.mk configure target_arch=arm64 build_dir=build-arm64
#
# === Individual setup targets (host or rootfs) ===
#
#     make -f helper.mk setup/debian         # apt install base packages
#     make -f helper.mk setup/cross          # install crossbuild-essential for target_arch
#     make -f helper.mk setup/rust           # install Rust + cross target + cargo tools
#     make -f helper.mk setup/python         # install pip packages (gcovr, sphinx, etc.)
#     make -f helper.mk setup/plantuml       # download plantuml.jar to /opt
#     make -f helper.mk setup/zap            # install ZAP .deb
#     make -f helper.mk setup/slc_cli        # unpack SLC CLI to /opt/slc_cli
#     make -f helper.mk setup/clang          # install Clang 12 (optional)
#     make -f helper.mk setup/yarn           # install yarn via npm
#
# === rootfs container (isolated Debian environment, faster than Docker) ===
#
#   Native amd64 in bullseye container:
#     make -f helper.mk rootfs/setup-cross debian_codename=bullseye target_arch=amd64
#     make -f helper.mk rootfs/configure debian_codename=bullseye target_arch=amd64 build_dir=build
#
#   Cross-compile arm64 in bullseye container:
#     make -f helper.mk rootfs/setup-cross debian_codename=bullseye target_arch=arm64
#     make -f helper.mk rootfs/configure debian_codename=bullseye target_arch=arm64 build_dir=build-arm64
#
#   Cross-compile armhf in bullseye container:
#     make -f helper.mk rootfs/setup-cross debian_codename=bullseye target_arch=armhf
#     make -f helper.mk rootfs/configure debian_codename=bullseye target_arch=armhf build_dir=build-armhf
#
#   Use bookworm (default) container:
#     make -f helper.mk rootfs/setup-cross target_arch=arm64
#     make -f helper.mk rootfs/configure target_arch=arm64 build_dir=build-arm64
#
#   Full integration test (clean rootfs -> setup -> build -> test):
#     make -f helper.mk test/rootfs debian_codename=bullseye target_arch=arm64
#
#   CMake management (bullseye only — bookworm ships a modern CMake):
#     # rootfs/setup-cross auto-installs CMake 3.21.6 inside bullseye container.
#     # Use rootfs/cmake standalone to enter the container and upgrade/reinstall:
#     make -f helper.mk rootfs/cmake debian_codename=bullseye target_arch=arm64
#
# === Docker workflow (for CI validation) ===
#
#     make -f helper.mk prepare/docker
#     make -f helper.mk test/docker

rootfs_dir?=/var/tmp/var/lib/machines/${project}

rootfs_shell?=${sudo} systemd-nspawn  \
		--machine="${project}" \
		--directory="${rootfs_dir}"
${rootfs_dir}:
	@mkdir -pv ${@D}
	@if [ -n '${debootstrap_keyring_option}' ]; then \
		mkdir -pv '${debootstrap_keyring_dir}'; \
		python3 '${CURDIR}/scripts/debootstrap_keyring.py' \
			--codename '${debian_codename}' \
			--output '${debootstrap_keyring}'; \
	fi
	time ${sudo} debootstrap ${debootstrap_keyring_option} --include="systemd,dbus" "${debian_codename}" "${rootfs_dir}" "${mirror_url}"
	@${sudo} chmod -v u+rX "${rootfs_dir}"

clean/rootfs:
	-${sudo} mv -fv -- "${rootfs_dir}" "${rootfs_dir}._$(shell date -u +%s).bak"

rootfs/%: ${rootfs_dir}
	${sudo} file -E -- "${rootfs_dir}" \
		|| ${SELF} "${rootfs_dir}"
	${rootfs_shell} apt-get update
	${rootfs_shell} apt-get install -- make sudo
	# Fix hostname resolution to prevent "sudo: unable to resolve host" errors
	@if ! grep -q "${project}" "${rootfs_dir}/etc/hosts" 2>/dev/null; then \
		echo "127.0.1.1 ${project}" | ${sudo} tee -a "${rootfs_dir}/etc/hosts" >/dev/null; \
	fi
	${rootfs_shell}	\
		--bind="${CURDIR}:${CURDIR}" \
		${MAKE} \
			--directory="${CURDIR}" \
			--file="${CURDIR}/helper.mk" \
			HOME="/root" \
			RUSTUP_HOME="/root/.rustup" \
			CARGO_HOME="/root/.cargo" \
			target_arch="${target_arch}" \
			build_dir="${build_dir}" \
			cmake_options="${cmake_options}" \
			-- "${@F}"

check/rootfs: prepare rootfs/check
	echo "# TODO only touched files"
	@echo "# ${project}: log: $@: done: $^"

# rootfs/cmake: Enter the bullseye container and install CMake 3.21.6 inside it
rootfs/cmake: ${rootfs_dir}
	${sudo} file -E -- "${rootfs_dir}" \
		|| ${SELF} "${rootfs_dir}"
	${rootfs_shell} apt-get update
	${rootfs_shell} apt-get install -- make sudo curl
	# Fix hostname resolution to prevent "sudo: unable to resolve host" errors
	@if ! grep -q "${project}" "${rootfs_dir}/etc/hosts" 2>/dev/null; then \
		echo "127.0.1.1 ${project}" | ${sudo} tee -a "${rootfs_dir}/etc/hosts" >/dev/null; \
	fi
	${rootfs_shell}	\
		--bind="${CURDIR}:${CURDIR}" \
		${MAKE} \
			--directory="${CURDIR}" \
			--file="${CURDIR}/helper.mk" \
			HOME="/root" \
			RUSTUP_HOME="/root/.rustup" \
			CARGO_HOME="/root/.cargo" \
			target_arch="${target_arch}" \
			build_dir="${build_dir}" \
			cmake_options="${cmake_options}" \
			-- _cmake

# rootfs/setup-cross: Create rootfs, run setup (includes cross toolchain via setup/debian) + verify
rootfs/setup-cross: ${rootfs_dir}
	${sudo} file -E -- "${rootfs_dir}" \
		|| ${SELF} "${rootfs_dir}"
	${rootfs_shell} apt-get update
	${rootfs_shell} apt-get install -- make sudo
	# Fix hostname resolution to prevent "sudo: unable to resolve host" errors
	@if ! grep -q "${project}" "${rootfs_dir}/etc/hosts" 2>/dev/null; then \
		echo "127.0.1.1 ${project}" | ${sudo} tee -a "${rootfs_dir}/etc/hosts" >/dev/null; \
	fi
	${rootfs_shell}	\
		--bind="${CURDIR}:${CURDIR}" \
		${MAKE} \
			--directory="${CURDIR}" \
			--file="${CURDIR}/helper.mk" \
			HOME="/root" \
			RUSTUP_HOME="/root/.rustup" \
			CARGO_HOME="/root/.cargo" \
			target_arch="${target_arch}" \
			build_dir="${build_dir}" \
			cmake_options="${cmake_options}" \
			-- setup setup/cargo-mirror setup/cross setup/rust _cmake

test/rootfs: clean/rootfs rootfs/setup rootfs/distclean check/rootfs
	@echo "# ${project}: log: $@: done: $^"

### @Docker: is only for validation no need to rely on it

prepare/docker: Dockerfile prepare
	time docker build \
		--tag="${project}" \
		--file="$<" .
	@echo "# ${project}: log: $@: done: $^"

docker_workdir?=/usr/local/opt/${project}

docker/%: Dockerfile
	time docker run "${project}:latest" -C "${docker_workdir}" "${@F}"

test/docker: distclean prepare/docker docker/help docker/test
	@echo "# ${project}: log: $@: done: $^"
