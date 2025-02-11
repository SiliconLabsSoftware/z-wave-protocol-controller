#!/usr/bin/make -f
# -*- makefile -*-
# ex: set tabstop=4 noexpandtab:
# -*- coding: utf-8 -*

default: help all/default
	@echo "$@: TODO: Support more than $^ by default"
	@date -u

SELF?=${CURDIR}/helper.mk

project?=z-wave-protocol-controller
# Temporary workaround for:
# https://gitlab.kitware.com/cmake/cmake/-/issues/22813#note_1620373
project_test_dir?=applications
project_docs_api_target?=zpc_doxygen
version?=$(shell git describe --tags || echo "0")

# Allow overloading from env if needed
# VERBOSE?=1
BUILD_DEV_GUI?=OFF

cmake_options?=-B ${build_dir}

CMAKE_GENERATOR?=Ninja
export CMAKE_GENERATOR

build_dir?=build
sudo?=sudo

debian_codename?=bookworm

packages?=cmake ninja-build build-essential python3-full ruby clang
packages+=git-lfs unp time file
packages+=nlohmann-json3-dev
# TODO: remove for offline build
packages+=curl wget python3-pip
packages+=time

# For docs
packages+=graphviz
export cmake_options+=-DDOXYGEN_HAVE_DOT=YES

packages+=python3-breathe python3-myst-parser \
  python3-sphinx-markdown-tables python3-sphinx-rtd-theme \
  python3-linkify-it

# TODO: https://bugs.debian.org/1004136#python-sphinxcontrib.plantuml
# packages+=python3-sphinxcontrib.plantuml

docs_dist_dir?=${build_dir}/dist

# Extra for components, make it optional
packages+=python3-jinja2
packages+=yarnpkg

rust_url?=https://sh.rustup.rs
RUST_VERSION?=1.71.0
export PATH := ${HOME}/.cargo/bin:${PATH}


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
	@grep -o '^[^ ]*:' ${SELF} \
		| grep -v '\$$' | grep -v '^#' | grep -v '^\.' \
		| grep -v '=' | grep -v '%'
	@echo ""
	@echo "# Environment:"
	@echo "# PATH=${PATH}"
	@echo ""

setup/debian: ${CURDIR}/docker/target_dependencies.apt ${CURDIR}/docker/host_dependencies.apt
	cat /etc/debian_version
	-${sudo} apt update
	${sudo} apt install -y $(shell sort $^ | sed -e 's|//.*||g' )
	${sudo} apt install -y ${packages}
	@echo "$@: TODO: Support debian stable rustc=1.63 https://tracker.debian.org/pkg/rustc"

setup/rust:
	@echo "$@: TODO: Support https://tracker.debian.org/pkg/rustup"
	curl --insecure  --proto '=https' --tlsv1.2 -sSf  ${rust_url} | bash -s -- -y --default-toolchain ${RUST_VERSION}
	cat $${HOME}/.cargo/env
	@echo '$@: info: You might like to add ". $${HOME}/.cargo/env" to "$${HOME}/.bashrc"'
	-which rustc
	rustc --version
	cargo --version
	rustc --print target-list
	@echo "$@: TODO: https://github.com/kornelski/cargo-deb/issues/159"
	cargo install --version 1.44.0 --locked cargo-deb
	@echo "$@: TODO: Support stable version from https://releases.rs/ or older"

setup/python/pip/%:
	pip3 --version || echo "warning: Please install pip"
	pip3 install "${@F}" \
		|| pip3 install --break-system-packages "${@F}"

setup/python: setup/python/pip/pybars3 setup/python/pip/sphinxcontrib.plantuml
	python3 --version
	@echo "$@: TODO: https://bugs.debian.org/1094297#pybars3"
	@echo "$@: TODO: https://bugs.debian.org/1004136#python-sphinxcontrib.plantuml"

# Relate-to: https://gitlab.kitware.com/cmake/cmake/-/issues/22813#note_1620373
cmake_version?=3.29.3
cmake_sha256?=f1a1672648eb72c0f7945b347e0537ebf640468e5ddd74f3d1def714e190e0cf
cmake_filename?=cmake-${cmake_version}-linux-x86_64.sh
cmake_url?=https://github.com/Kitware/CMake/releases/download/v${cmake_version}/${cmake_filename}

setup/cmake:
	@echo "$@: TODO: remove for debian-13+ , currently supporting ${debian_codename}"
	time curl -O -L ${cmake_url}
	sha256sum ${cmake_filename} \
		| grep "${cmake_sha256}"
	${sudo} ${SHELL} "${cmake_filename}" \
		--prefix=/usr/local \
		--skip-license
	rm -v "${cmake_filename}"
	cmake --version

setup-cmake: setup/cmake


plantuml_url?=https://github.com/plantuml/plantuml/releases/download/v1.2022.0/plantuml-1.2022.0.jar
plantuml_filename?=$(shell basename -- "${plantuml_url}")
plantuml_sha256?=f1070c42b20e6a38015e52c10821a9db13bedca6b5d5bc6a6192fcab6e612691
plantuml_dir?=/usr/local/share/plantuml
PLANTUML_JAR_PATH?=${plantuml_dir}/${plantuml_filename}
export PLANTUML_JAR_PATH

${PLANTUML_JAR_PATH}:
	@echo "# $@: TODO: Please help on:"
	@echo "# $@: https://bugs.debian.org/1004135#2025"
	curl -L ${plantuml_url} -O
	sha256sum ${plantuml_filename} | grep "${plantuml_sha256}"
	${sudo} install -d ${plantuml_dir}
	${sudo} install ${plantuml_filename} ${plantuml_dir}/
	rm -v ${plantuml_filename}
	@echo "# %@: Please adapt env to:"
	@echo "# export PLANTUML_JAR_PATH=${plantuml_dir}/${plantuml_filename}"

setup/plantuml: ${PLANTUML_JAR_PATH}
	file -E $<

setup/debian/bookworm: setup/debian setup/rust setup/python setup/plantuml
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

git/prepare: git/modules/prepare
	@echo "# ${project}: warning: $@: Skipping $^ as not needed for project"

configure: ${build_dir}/CMakeCache.txt
	file -E $<

configure/clean:
	rm -rf ${build_dir}/CMake*

reconfigure: configure/clean configure
	@date -u

${build_dir}/CMakeCache.txt: CMakeLists.txt
	cmake ${cmake_options}

all: ${build_dir}/CMakeCache.txt
	cmake --build ${<D} \
		|| cat ${build_dir}/CMakeFiles/CMakeOutput.log
	cmake --build ${<D}
.PHONY: all

${build_dir}/%: all
	file -E "$@"

test: ${build_dir}
	ctest --test-dir ${<}/${project_test_dir}

check: test

dist/cmake: ${build_dir}
	cmake --build $< --target package
	cmake --build $< --target package_archive

dist/deb: ${build_dir}
	cmake --build $< --target package
	install -d $</$@
	cp -av ${<}/*.deb $</$@

dist: dist/cmake

distclean:
	rm -rf ${build_dir}

prepare: git/prepare
	git --version
	cmake --version

all/default: configure prepare all test dist
	@date -u


### @rootfs is faster than docker for env check

rootfs_dir?=/var/tmp/var/lib/machines/${project}

rootfs_shell?=${sudo} systemd-nspawn  \
		--machine="${project}" \
		--directory="${rootfs_dir}"
${rootfs_dir}:
	@mkdir -pv ${@D}
	time ${sudo} debootstrap --include="systemd,dbus" "${debian_codename}" "${rootfs_dir}"
	@${sudo} chmod -v u+rX "${rootfs_dir}"

clean/rootfs:
	-${sudo} mv -fv -- "${rootfs_dir}" "${rootfs_dir}._$(shell date -u +%s).bak"

rootfs/%: ${rootfs_dir}
	${sudo} file -E -- "${rootfs_dir}" \
		|| ${SELF} "${rootfs_dir}"
	${rootfs_shell} apt-get update
	${rootfs_shell} apt-get install -- make sudo
	${rootfs_shell}	\
		--bind="${CURDIR}:${CURDIR}" \
		${MAKE} \
			--directory="${CURDIR}" \
			--file="${CURDIR}/helper.mk" \
			HOME="${HOME}" \
			USER="${USER}" \
			-- "${@F}"

check/rootfs: prepare rootfs/check
	echo "# TODO only touched files"
	@echo "# ${project}: log: $@: done: $^"

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

docs: ./scripts/build/build_documentation.py doc ${PLANTUML_JAR_PATH} configure
	@echo "# export PLANTUML_JAR_PATH=${plantuml_dir}/${plantuml_filename}"
	@echo "$@: PLANTUML_JAR_PATH=${PLANTUML_JAR_PATH}"
	$< --output-dir $@
	touch $@/.nojekyll

zpc/docs/api: docs
	cmake --build build --target  zpc_doxygen
	install -d docs/doxygen_zpc
	cp -rfa build/zpc_doxygen_zpc/html/* docs/doxygen_zpc/

docs/api: zpc/docs/api

docs/dist: ${docs_dist_dir}/${project}-docs-${version}.zip
	file -E "$<"
	@du -hsc "$<"

${docs_dist_dir}/${project}-docs-${version}.zip: docs docs/api
	ln -fs docs "${project}-docs-${version}"
	install -d ${@D}
	zip -r9 "$@" "${project}-docs-${version}/" \
	  --exclude "*/_sources/*"
	rm "${project}-docs-${version}"
