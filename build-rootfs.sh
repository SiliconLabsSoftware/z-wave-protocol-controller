#! /usr/bin/env bash
# -*- mode: Bash; tab-width: 2; indent-tabs-mode: nil; coding: utf-8 -*-
# vim:shiftwidth=4:softtabstop=4:tabstop=4:
# SPDX-License-Identifier: LicenseRef-MSLA
# SPDX-FileCopyrightText: Silicon Laboratories Inc. https://www.silabs.com

set -e
set -x

project="unify-zpc"
debian_suite="bookworm"

target_debian_arch=${ARCH:="$(dpkg --print-architecture)"}
#target_debian_arch="armhf"

case $target_debian_arch in
    arm64)
        qemu_arch="aarch64"
        qemu_system="qemu-system-aarch64"
        qemu_machine="raspi3b"
        cmake_options="
  -DCMAKE_SYSTEM_PROCESSOR=$ARCH \
	-DCARGO_TARGET_TRIPLE=aarch64-unknown-linux-gnu \
"
        ;;

    armhf)
        qemu_arch="arm"
        qemu_system="qemu-system-arm"
        cmake_options="
  -DCMAKE_SYSTEM_PROCESSOR=armhf \
	-DCARGO_TARGET_TRIPLE=armv7-unknown-linux-gnueabihf \
"
        ;;

    amd64)
        qemu_arch="x86_64"
        qemu_system="qemu-system-${qemu_arch}"
        cmake_options="
  -DCMAKE_SYSTEM_PROCESSOR=${qemu_arch}
	-DCARGO_TARGET_TRIPLE=${qemu_arch}-unknown-linux-gnu \
  "
        ;;
    i386)
        qemu_arch="${target_debian_arch}"
        qemu_system="qemu-system-${qemu_arch}"
        cmake_options="
  -DCMAKE_SYSTEM_PROCESSOR=${qemu_arch}
	-DCARGO_TARGET_TRIPLE=i386-unknown-linux-gnu \
  "
        ;;

    *)
        echo "error: Not supported yet"
        exit 1
        ;;
esac

qemu_file="/usr/bin/${qemu_system}"

debian_mirror_url="http://deb.debian.org/debian"
sudo="sudo"
machine="${project}-${debian_suite}-${target_debian_arch}"
rootfs_dir="/var/tmp/var/lib/machines/${machine}"
MAKE=/usr/bin/make
CURDIR="$PWD"
chroot="systemd-nspawn"

${sudo} apt-get update
${sudo} apt install -y \
        binfmt-support \
        debian-archive-keyring \
        debootstrap \
        qemu-system-arm \
        qemu-user-static \
        systemd-container \
        time

case ${target_debian_arch} in
    i386)
        echo TODO \
        sudo update-binfmts --install i386 /usr/bin/qemu-i386-static \
             --magic '\x7fELF\x01\x01\x01\x03\x00\x00\x00\x00\x00\x00\x00\x00\x03\x00\x03\x00\x01\x00\x00\x00' \
             --mask '\xff\xff\xff\xff\xff\xff\xff\xfc\xff\xff\xff\xff\xff\xff\xff\xff\xf8\xff\xff\xff\xff\xff\xff\xff'
        ;;
    *)
        ${sudo} update-binfmts --enable qemu-${qemu_arch}
        ;;
esac

if [ ! -d "${rootfs_dir}" ] ; then
    ${sudo} mkdir -pv "${rootfs_dir}"
    time ${sudo} debootstrap \
         --arch="${target_debian_arch}" \
	       "${debian_suite}" "${rootfs_dir}" "${debian_mirror_url}"
    ${sudo} chmod -v u+rX "${rootfs_dir}"
fi


# TODO: https://github.com/rust-lang/cargo/issues/8719#issuecomment-1516492970
env_vars="$env_vars CARGO_REGISTRIES_CRATES_IO_PROTOCOL=sparse"

# TODO: https://github.com/rust-lang/cargo/issues/10583
env_vars="$env_vars CARGO_NET_GIT_FETCH_WITH_CLI=true"


cargo_dir="/tmp/$USER/${machine}/${HOME}/.cargo"
${sudo} mkdir -pv  "${cargo_dir}"

case ${chroot} in
    "systemd-nspawn")
        rootfs_shell="${sudo} systemd-nspawn \
 --bind "${qemu_file}" \
 --directory="${rootfs_dir}" \
 --machine="${machine}" \
 --bind="${CURDIR}:${CURDIR}" \
 --bind="${cargo_dir}:/root/.cargo" \
"
        ;;
    *)
        rootfs_shell="${sudo} chroot ${rootfs_dir}"
        l="dev dev/pts sys proc"
        for t in $l ; do $sudo mkdir -p "${rootfs_dir}/$t" && $sudo mount --bind "/$t" "${rootfs_dir}/$t" ; done
    ;;
esac

${rootfs_shell} \
    apt-get install -y -- make sudo util-linux
if [ "$1" = "" ] ; then
${rootfs_shell}	\
    ${MAKE} \
    --directory="${CURDIR}" \
    --file="${CURDIR}/helper.mk" \
    USER="${USER}" \
    ${env_vars} \
    -- \
    help setup default \
    target_debian_arch="${target_debian_arch}" \
    cmake_options="${cmake_options}"

find "${CURDIR}/" -iname "*.deb"

else
${rootfs_shell}	\
    ${MAKE} \
    --directory="${CURDIR}" \
    --file="${CURDIR}/helper.mk" \
    USER="${USER}" \
    ${env_vars} \
    -- \
    help $@ \
    target_debian_arch="${target_debian_arch}"
fi



sudo du -hs "/var/tmp/var/lib/machines/${machine}"
