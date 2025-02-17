# z-wave-protocol-controller

## About

Z-Wave-Protocol-Controller is a Linux application to control Z-Wave
networks with a Silicon Labs NCP, It has grown as part of Unify SDK
project and is now maintained as an external project.

## News

- [NEWS.md](NEWS.md) lists important information regarding zpc "split release".
- [applications/zpc/release_notes.md](applications/zpc/release_notes.md)

## Documentation

Please refer to online documentation at:

https://siliconlabssoftware.github.io/z-wave-protocol-controller

Or relevant sources pages, to get started head to:

- [doc/protocol/zwave/zpc_introduction.md](doc/protocol/zwave/zpc_introduction.md)

## Quickstart

### Native (Linux) build

The project is CMake based, to prepare the environment,
have a look at [./helper.mk](helper.mk)'s  details
for needed steps to setup developer system before using CMake normally.

At the moment stable version of Debian (12) is supported,
so it should work also in relatives projects (Ubuntu, RaspiOS, WSL2 etc)
and should be easy to adapt to other distributions.

```sh
sudo apt-get install -y sudo make git

git clone https://github.com/SiliconLabsSoftware/z-wave-protocol-controller
cd z-wave-protocol-controller

./helper.mk help
./helper.mk setup  # To setup developer system (once)
./helper.mk VERBOSE=1 # Default build tasks verbosely (depends on setup)"
./helper.mk run # Run entry-point application
```

It should print zpc's help.

To use it, a Silicon Labs' Z-Wave NCP should be plugged in USB port.

To setup the device zwave_ncp_serial_api_controller application should be flashed using
[simplicity-studio](https://www.silabs.com/developer-tools/simplicity-studio)

Alternatively demo-applications package can be downloaded from
[SiliconLabs SDK repository](https://github.com/SiliconLabs/simplicity_sdk/releases)
and flashed using command-line tool
[SimplicityCommander](https://www.silabs.com/documents/public/software/SimplicityCommander-Linux.zip)
.

Then to verify you can check firmware version using zpc:

```sh
lsusb # Should list the NCP device (WSL users should attach device first)
serial=$(ls /dev/serial/by-id/usb-Silicon_Labs* | head -n1)
./helper.mk all run run_args="--zpc.serial=${serial} --zpc.ncp_version"
# <i> [zpc_ncp_update] chip_serial_api_version: 7.23.1
```

Then let's interact with ZPC's inbuilt shell without installing it.

```sh
sudo apt install -y mosquitto # Is a strong runtime dependency
serial=$(ls /dev/serial/by-id/usb-Silicon_Labs* | head -n1)
run_args="--zpc.serial=${serial}"
mapdir="applications/zpc/components/dotdot_mapper/rules"
run_args="$run_args --mapdir=${mapdir}"
datastore_file="tmp.db"
run_args="$run_args --zpc.datastore_file=${datastore_file}"
cache_path="tmp"
run_args="$run_args --zpc.ota.cache_path=${cache_path}"
mkdir -p ${cache_path}
./helper.mk run run_args="$run_args"


ZPC>help
==================================================
Unify Command line interface Help:
==================================================
(...)
exit  :Exit the application
(...)
zwave_home_id Print Z-Wave Home ID
(...)
zwave_add_node  :Add a Z-Wave node to the network
(...)
zwave_set_default Reset Z-Wave network
(...)
ZPC> zwave_home_id
Z-Wave Home ID:
1BADC0DE
ZPC> zwave_add_node
(...)
```

Set endnode to learn mode and the controller should see it:

```sh
(...)
Please verify the DSK and insert the missing part (first 5 digits)
Usage: zwave_accept_dsk <insert the first two byte of the DSK in [decimal]>
ZPC> zwave_accept_dsk (...)
```

Then you will have to learn how to use couple of shell commands
"attribute_store_log" and "attribute_store_set_desired",
as explained in the reference documentation.

```sh
ZPC>attribute_store_log
(...)
ZPC>attribute_store_log_search OnOff
(...)
│   │   │   │───(110) ZCL OnOff OnOff .....................................      0
(...)
ZPC>attribute_store_set_desired 110,1
(...)
```

### More

Refer to [./doc](doc) for more (using shell, MQTT, WebApp etc).


### Docker build

The fastest (less than 20min) way to build z-wave-protocol-controller from scratch
is to delegate all tasks to docker.

```sh
docker build https://github.com/SiliconLabsSoftware/z-wave-protocol-controller.git#ver_1.7.0
```

This one-liner will do download latest release, setup environment, build, test, package...

Also a docker-compose file is provided to start ZPC and use it along a MQTT client
(eg:
[mosquitto-clients](https://github.com/eclipse-mosquitto/mosquitto/)
).

Power users might prefer to work in sources tree in a native GNU/Linux
environment as explained above.

## Contributing

- [CONTRIBUTING.md](CONTRIBUTING.md)

## References

- https://SiliconLabs.github.io/UnifySDK/
- https://github.com/SiliconLabs/UnifySDK/
- https://docs.silabs.com/z-wave/
- https://www.silabs.com/
- https://z-wavealliance.org/
- https://github.com/Z-Wave-Alliance/z-wave-stack/wiki/ZPC

## Legal info

**Copyright 2021 Silicon Laboratories Inc. www.silabs.com**

The licensor of this software is Silicon Laboratories Inc. Your use of this software is governed by the terms of Silicon Labs Master Software License Agreement (MSLA) available at www.silabs.com/about-us/legal/master-software-license-agreement. This software is distributed to you in Source Code format and is governed by the sections of the MSLA applicable to Source Code.
