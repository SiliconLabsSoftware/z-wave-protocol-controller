#! /usr/bin/env bash
# -*- mode: Bash; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
# vim:shiftwidth=4:softtabstop=4:tabstop=4:
# SPDX-License-Identifier: LicenseRef-MSLA
# SPDX-FileCopyrightText: Silicon Laboratories Inc. https://www.silabs.com

set -e
set -x

cat<<EOF
Usage:

ARCH=arm64 ./scripts/build-rootfs.sh
EOF

project="z-wave-protocol-controller"
debian_suite="bookworm"

debian_arch=$(dpkg --print-architecture)

# Can be overloaded from env eg: ARCH=arm64"
target_debian_arch=${ARCH:="${debian_arch}"}

debian_mirror_url="http://deb.debian.org/debian"
sudo="sudo"
machine="${project}-${debian_suite}-${target_debian_arch}-rootfs"
rootfs_dir="/var/tmp/var/lib/machines/${machine}"
MAKE="/usr/bin/make"
CURDIR="$PWD"
chroot="systemd-nspawn"
packages="debootstrap \
  debian-archive-keyring \
  systemd-container \
  time \
"
case $target_debian_arch in
    amd64)
        qemu_system="qemu-system-${CMAKE_SYSTEM_PROCESSOR}"
        export CMAKE_SYSTEM_PROCESSOR="x86_64"
        export CARGO_TARGET_TRIPLE="${CMAKE_SYSTEM_PROCESSOR}-unknown-linux-gnu"
        qemu_package="qemu-system-x86"
        ;;

    arm64)
        qemu_arch="aarch64"
        qemu_system="qemu-system-${qemu_arch}"
        export CMAKE_SYSTEM_PROCESSOR="${qemu_arch}"
        export CARGO_TARGET_TRIPLE="${CMAKE_SYSTEM_PROCESSOR}-unknown-linux-gnu"
        qemu_package="qemu-system-arm"
        ;;

    armhf)
        debian_arch="armhf"
        qemu_arch="arm"
        qemu_system="qemu-system-${qemu_arch}"
        export CMAKE_SYSTEM_PROCESSOR="armv7l"
        export CARGO_TARGET_TRIPLE="armv7-unknown-linux-gnueabihf"
        qemu_package="qemu-system-arm"

        # Workaround: https://github.com/armbian/build/issues/5330
        binfmt_file="/var/lib/binfmts/qemu-${qemu_arch}"
        [ -e "$binfmt_file" ] || cat<<EOF \
                | { sudo mkdir -p /var/lib/binfmts && sudo tee "$binfmt_file" ; }
package qemu-user-static
interpreter /usr/libexec/qemu-binfmt/arm-binfmt-P
magic \x7f\x45\x4c\x46\x01\x01\x01\x00\x00\x00\x00\x00\x00\x00\x00\x00\x02\x00\x28\x00
offset 0
mask \xff\xff\xff\xff\xff\xff\xff\x00\xff\xff\xff\xff\xff\xff\xff\xff\xfe\xff\xff\xff
credentials yes
fix_binary no
preserve yes
EOF
        ;;
    *)
        CMAKE_SYSTEM_PROCESSOR="$ARCH"
        CARGO_TARGET_TRIPLE="${CMAKE_SYSTEM_PROCESSOR}-unknown-linux-gnu"
        qemu_arch="${ARCH}"
        qemu_system="qemu-system-${qemu_arch}"
        qemu_package="${qemu_system}"
        echo "error: Not supported yet"

        exit 1
        ;;
esac
qemu_file="/usr/bin/${qemu_system}"
binfmt_file="/var/lib/binfmts/qemu-${qemu_arch}"

${sudo} apt-get update
${sudo} apt install -y ${packages}

if [ "${debian_target_arch}" != "${debian_arch}" ] ; then
    echo "log: ${ARCH}: Support foreign arch: ${qemu_arch}"
    ${sudo} apt-get update
    packages="binfmt-support qemu-user-static ${qemu_package}"
    ${sudo} apt install -y ${packages}

    if [ -e "${qemu_file}" ]; then
        ${sudo} update-binfmts --enable "qemu-${qemu_arch}" \
            || find /usr/libexec/qemu-binfmt/
    fi
fi

if [ ! -d "${rootfs_dir}" ] ; then
    ${sudo} mkdir -pv "${rootfs_dir}"
    time ${sudo} debootstrap \
         --arch="${target_debian_arch}" \
	       "${debian_suite}" "${rootfs_dir}" "${debian_mirror_url}"
    ${sudo} chmod -v u+rX "${rootfs_dir}"
fi

### Environement to pass

[ "" = "$UNIFYSDK_GIT_REPOSITORY" ] \
  || env_vars="$env_vars UNIFYSDK_GIT_REPOSITORY=${UNIFYSDK_GIT_REPOSITORY}"
[ "" = "$UNIFYSDK_GIT_TAG" ] \
  || env_vars="$env_vars UNIFYSDK_GIT_TAG=${UNIFYSDK_GIT_TAG}"
export UNIFYSDK_GIT_TAG


# TODO: https://github.com/rust-lang/cargo/issues/8719#issuecomment-1516492970
env_vars="$env_vars CARGO_REGISTRIES_CRATES_IO_PROTOCOL=sparse"

# TODO: https://github.com/rust-lang/cargo/issues/10583
env_vars="$env_vars CARGO_NET_GIT_FETCH_WITH_CLI=true"

env_vars_options=""
for i in $env_vars; do
    env_vars_options="$env_vars_options --setenv=$i"
done

### Workarounds/Optimizations

cargo_dir="/tmp/$USER/${machine}/${HOME}/.cargo"
${sudo} mkdir -pv  "${cargo_dir}"

case ${chroot} in
    "systemd-nspawn")
        rootfs_shell="${sudo} systemd-nspawn \
 --directory="${rootfs_dir}" \
 --machine="${machine}" \
 --bind="${CURDIR}:${CURDIR}" \
 --bind="${cargo_dir}:/root/.cargo" \
 $env_vars_options
"
        if [ -e "${qemu_file}" ] ; then
            rootfs_shell="$rootfs_shell --bind ${qemu_file}"
        fi
        ;;
    *)
        rootfs_shell="${sudo} chroot ${rootfs_dir}"
        l="dev dev/pts sys proc"
        for t in $l ; do
            $sudo mkdir -p "${rootfs_dir}/$t"
            $sudo mount --bind "/$t" "${rootfs_dir}/$t"
        done
    ;;
esac

${rootfs_shell} \
    apt-get install -y -- make sudo util-linux

${rootfs_shell}	\
    ${MAKE} \
    --directory="${CURDIR}" \
    --file="${CURDIR}/helper.mk" \
    USER="${USER}" \
    ${env_vars} \
    -- \
    help setup default \
    target_debian_arch="${target_debian_arch}" \
    CMAKE_SYSTEM_PROCESSOR="${CMAKE_SYSTEM_PROCESSOR}" \
    CARGO_TARGET_TRIPLE="${CARGO_TARGET_TRIPLE}" \
    # EoL

echo "sudo du -hs -- '/var/tmp/var/lib/machines/${machine}' # can be removed now"
