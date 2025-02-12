# Unify Framework Getting Started


Note: For reference documentation please refer to
[UnifySDK documentation](
https://siliconlabs.github.io/UnifySDK/doc/getting_started_unify_sdk
)

WARNING: The following chapters may be oudated,
they are still here until the new release of UnifySDK.
In the future, Z-Wave parts will be more isolated and the rest deduplicated.


## Setup base system

Unify Framework has a number of service applications which enables different basic
functionality. For unlocking all features of the Unify Framework, install the Debian
packages of the applications below.
It is recommended to use the Developer GUI for controlling the Unify Framework IoT Protocol Controllers such as `ZPC`.

For installation steps refer to the [](how-to-install) section.

1. [Provisioning List (UPVL) User's Guide](https://siliconlabs.github.io/UnifySDK/applications/upvl/readme_user)
2. [Group Manager Service (GMS) User's Guide](https://siliconlabs.github.io/UnifySDK/applications/gms/readme_user).
3. [Name and Location service (NAL) User's Guide](https://siliconlabs.github.io/UnifySDK/applications/nal/readme_user).
4. [OTA Image Provider User's Guide](https://siliconlabs.github.io/UnifySDK/applications/image_provider/readme_user).
5. [Developer GUI User's Guide](https://siliconlabs.github.io/UnifySDK/applications/dev_ui/dev_gui/readme_user).

## Choose an IoT protocol

For including an IoT protocol stack, you need to install and setup at least one protocol controller.

- [](zpc)

(zpc)=

### Z-Wave Protocol Controller (ZPC)

**Prerequisite**: Required hardware for using the `ZPC` is a [Z-Wave module](https://www.silabs.com/wireless/z-wave)
which is flashed with a SerialAPI firmware.

The Z-Wave Protocol Controller allows Unify to control Z-Wave devices. Starting
quickly is achieved by just installing the `uic-zpc` Debian package. This should
automatically start up the `ZPC` after the configuration steps. You need to
provide the USB path for the Z-Wave module at the configuration steps.

A more in depth getting started guide specifically for the `ZPC` is
[ZPC User's Guide](../applications/zpc/readme_user.md).

### Evaluation after Installation

Once all Unify Framework Applications are installed and configured, one can evaluate the system via the Unify Framework Developer GUI.
Note that the RPi4 needs to be rebooted after installing the Debian packages for the first time.

After a reboot the Unify Framework Developer GUI can be accessed from a browser at [http://raspberrypi.local:3080](http://raspberrypi.local:3080).

_Note_ that the Unify Framework Developer GUI needs TCP access to the port 3080 and 1337
on the Raspberry Pi.

See the
[Dev-GUI manual](https://siliconlabs.github.io/UnifySDK/applications/dev_ui/dev_gui/readme_user)
for more information about using this interface.
