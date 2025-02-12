# Getting Started as a Developer


Note: For reference documentation please refer to
[UnifySDK documentation](
https://siliconlabs.github.io/UnifySDK/doc/getting_started_as_developer
)

WARNING: The following chapters may be oudated,
they are still here until the new release of UnifySDK.
In the future, Z-Wave parts will be more isolated and the rest deduplicated.


The
[Unify Developer Guide](https://siliconlabs.github.io/UnifySDK/doc/readme_developer)
contains a lot of useful information for Unify developers. 

The 
[Unify Build Guide](https://siliconlabs.github.io/UnifySDK/doc/readme_building)
is the best starting point for working with the source code.

The
[Unify User Guide](https://siliconlabs.github.io/UnifySDK/doc/unify_readme_user)
contains helpful tips on debugging within Unify.

## Development Platform Recommendations

The Unify reference platform is a Raspberry Pi 4 however it is understood that users may not wish to run on a Raspberry Pi in a final product. Porting to Debian based linux should be straightforward.

System requirements for flash, RAM, and dependencies are listed at [here](system_requirements.md)

## Developing IoT Services

The IoT or cloud connector must use UCL (MQTT) to communicate with the other components of Unify. See [How to Develop an IoT service](how_to_develop_an_iot_service.md) Unify uses MQTT to communicate among each component using the Mosquitto MQTT broker. UCL is the format of the MQTT messages. 
The
[Unify Specifications](https://siliconlabs.github.io/UnifySDK/doc/unify_specifications)
contains the definitions for UCL.

Also see 
[UCL MQTT API](https://siliconlabs.github.io/UnifySDK/reference_ucl_mqtt")

## Developing protocol controllers

See
[how to develop Protocol Controller](https://siliconlabs.github.io/UnifySDK/doc/how_to_develop_a_protocol_controller)

The attribute mapper helps to translate protocol commands to UCL MQTT messages.
See 
[How to write UAM files](https://siliconlabs.github.io/UnifySDK/doc/how_to_write_uam_files)

## Extend UCL clusters

See [How to write a new Cluster](../applications/zpc/how_to_interact_with_clusters.rst)

It might be needed for non-zigbee controllers if some description are missing.

## Overview of relations among Unify Applications

The Unify Framework consists of several applications including Protocol Controllers, IoT
Services (e.g. Developer GUI, UPTICap) and multiple application services that
facilitate various functionalities such as SmartStart Provisioning, Group
Management, Name and Location Service and OTA Image Provider.

The following table presents the relationship amongst Unify Framework applications and
the Unify Protocol Controllers. If a given component is supported via a
Protocol Controller, it will be marked as [x].

|                                                                                  | [ZPC](../applications/zpc/readme_user.html) | [ZigPC](../applications/zigpc/readme_user.html) | [AoXPC](../applications/aox/applications/aoxpc/readme_user.html) |
| -------------------------------------------------------------------------------- | :------------------------------------------ | :---------------------------------------------- | :--------------------------------------------------------------- |
| [UPVL](../applications/upvl/readme_user.html)                                    | [x]                                         | [x]                                             |                                                                  |
| [GMS](../applications/gms/readme_user.html)                                      | [x]                                         | [x]                                             |                                                                  |
| [NAL](../applications/nal/readme_user.html)                                      | [x]                                         | [x]                                             | [x]                                                              |
| [OTA Image Provider](../applications/image_provider/readme_user.html)            | [x]                                         | [x]                                             |                                                                  |
| [Dev GUI](../applications/dev_ui/dev_gui/readme_user.html)                       | [x]                                         | [x]                                             | [x]                                                              |
| [UPTICap](../applications/upti_cap/readme_user.html)                             | [x]                                         | [x]                                             |                                                                  |
| [UPTIWriter](../applications/upti_writer/readme_user.html)                       | [x]                                         | [x]                                             |                                                                  |
| [AoX Positioning](../applications/aox/applications/positioning/readme_user.html) |                                             |                                                 | [x]                                                              |
