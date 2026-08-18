## MQTT Interface Documentation

### Endpoint addressing for multi-endpoint devices

For devices that support multiple endpoints (e.g. multi-channel devices), include the endpoint identifier in the MQTT topic using the `ep{endpoint_id}` segment:

```
zpc/{home_id}/{node_id}/ep{endpoint_id}/{command_class}/Command/{command_name}
```

- **ep0** — root device (endpoint 0)
- **ep1**, **ep2**, **ep3**, … — endpoint 1, 2, 3, etc.

Example: `zpc/CAFECAFE/0004/ep3/ZWaveCC/Command/SwitchBinarySet` targets endpoint 3 of node 0x0004.

### Command class MQTT capabilities

| Command Class | MQTT Support | Support | Control |
|---------------|--------------|---------|---------|
| [COMMAND_CLASS_NOTIFICATION](command_class_notification/doc/generated/command_class_notification_mqtt_interface.md) | true | false | true |
| [COMMAND_CLASS_ASSOCIATION](command_class_association/doc/generated/command_class_association_mqtt_interface.md) | true | true | true |
| [COMMAND_CLASS_BASIC](command_class_basic/doc/generated/command_class_basic_mqtt_interface.md) | true | false | true |
| [COMMAND_CLASS_BATTERY](command_class_battery/doc/generated/command_class_battery_mqtt_interface.md) | true | false | true |
| [COMMAND_CLASS_DOOR_LOCK](command_class_door_lock/doc/generated/command_class_door_lock_mqtt_interface.md) | true | false | true |
| [COMMAND_CLASS_FIRMWARE_UPDATE_MD](command_class_firmware_update_md/doc/generated/command_class_firmware_update_md_mqtt_interface.md) | false | true | true |
| [COMMAND_CLASS_INDICATOR](command_class_indicator/doc/generated/command_class_indicator_mqtt_interface.md) | true | true | true |
| [COMMAND_CLASS_MANUFACTURER_SPECIFIC](command_class_manufacturer_specific/doc/generated/command_class_manufacturer_specific_mqtt_interface.md) | true | true | true |
| [COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION](command_class_multi_channel_association/doc/generated/command_class_multi_channel_association_mqtt_interface.md) | true | true | true |
| [COMMAND_CLASS_MULTI_CHANNEL](command_class_multi_channel/doc/generated/command_class_multi_channel_mqtt_interface.md) | true | false | true |
| [COMMAND_CLASS_POWERLEVEL](command_class_powerlevel/doc/generated/command_class_powerlevel_mqtt_interface.md) | false | true | false |
| [COMMAND_CLASS_SWITCH_BINARY](command_class_switch_binary/doc/generated/command_class_switch_binary_mqtt_interface.md) | true | false | true |
| [COMMAND_CLASS_SWITCH_MULTILEVEL](command_class_switch_multilevel/doc/generated/command_class_switch_multilevel_mqtt_interface.md) | true | false | true |
| [COMMAND_CLASS_THERMOSTAT_FAN_MODE](command_class_thermostat_fan_mode/doc/generated/command_class_thermostat_fan_mode_mqtt_interface.md) | true | false | true |
| [COMMAND_CLASS_THERMOSTAT_MODE](command_class_thermostat_mode/doc/generated/command_class_thermostat_mode_mqtt_interface.md) | true | false | true |
| [COMMAND_CLASS_THERMOSTAT_SETPOINT](command_class_thermostat_setpoint/doc/generated/command_class_thermostat_setpoint_mqtt_interface.md) | true | false | true |
| [COMMAND_CLASS_VERSION](command_class_version/doc/generated/command_class_version_mqtt_interface.md) | true | true | true |
| [COMMAND_CLASS_WAKE_UP](command_class_wake_up/doc/generated/command_class_wake_up_mqtt_interface.md) | true | false | true |
| [COMMAND_CLASS_SWITCH_COLOR](command_class_switch_color/doc/generated/command_class_switch_color_mqtt_interface.md) | true | false | true |
| [COMMAND_CLASS_ASSOCIATION_GRP_INFO](command_class_association_grp_info/doc/generated/command_class_association_grp_info_mqtt_interface.md) | true | true | true |
| [COMMAND_CLASS_DEVICE_RESET_LOCALLY](command_class_device_reset_locally/doc/generated/command_class_device_reset_locally_mqtt_interface.md) | false | true | true |
| [COMMAND_CLASS_ZWAVEPLUS_INFO](command_class_zwaveplus_info/doc/generated/command_class_zwaveplus_info_mqtt_interface.md) | false | true | true |
| [COMMAND_CLASS_SUPERVISION](command_class_supervision/doc/generated/command_class_supervision_mqtt_interface.md) | false | true | true |
| [COMMAND_CLASS_INCLUSION_CONTROLLER](command_class_inclusion_controller/doc/generated/command_class_inclusion_controller_mqtt_interface.md) | false | true | true |

> When MQTT Support is `false`, the linked documentation lists only TX (report/notification) commands. ZPC publishes those reports to MQTT but does not handle `Command/*` topics for that command class.

### Command Classes 

- [COMMAND_CLASS_NOTIFICATION](command_class_notification/doc/generated/command_class_notification_mqtt_interface.md)
- [COMMAND_CLASS_ASSOCIATION](command_class_association/doc/generated/command_class_association_mqtt_interface.md)
- [COMMAND_CLASS_BASIC](command_class_basic/doc/generated/command_class_basic_mqtt_interface.md)
- [COMMAND_CLASS_BATTERY](command_class_battery/doc/generated/command_class_battery_mqtt_interface.md)
- [COMMAND_CLASS_DOOR_LOCK](command_class_door_lock/doc/generated/command_class_door_lock_mqtt_interface.md)
- [COMMAND_CLASS_FIRMWARE_UPDATE_MD](command_class_firmware_update_md/doc/generated/command_class_firmware_update_md_mqtt_interface.md)
- [COMMAND_CLASS_INDICATOR](command_class_indicator/doc/generated/command_class_indicator_mqtt_interface.md)
- [COMMAND_CLASS_MANUFACTURER_SPECIFIC](command_class_manufacturer_specific/doc/generated/command_class_manufacturer_specific_mqtt_interface.md)
- [COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION](command_class_multi_channel_association/doc/generated/command_class_multi_channel_association_mqtt_interface.md)
- [COMMAND_CLASS_MULTI_CHANNEL](command_class_multi_channel/doc/generated/command_class_multi_channel_mqtt_interface.md)
- [COMMAND_CLASS_POWERLEVEL](command_class_powerlevel/doc/generated/command_class_powerlevel_mqtt_interface.md)
- [COMMAND_CLASS_SWITCH_BINARY](command_class_switch_binary/doc/generated/command_class_switch_binary_mqtt_interface.md)
- [COMMAND_CLASS_SWITCH_MULTILEVEL](command_class_switch_multilevel/doc/generated/command_class_switch_multilevel_mqtt_interface.md)
- [COMMAND_CLASS_THERMOSTAT_FAN_MODE](command_class_thermostat_fan_mode/doc/generated/command_class_thermostat_fan_mode_mqtt_interface.md)
- [COMMAND_CLASS_THERMOSTAT_MODE](command_class_thermostat_mode/doc/generated/command_class_thermostat_mode_mqtt_interface.md)
- [COMMAND_CLASS_THERMOSTAT_SETPOINT](command_class_thermostat_setpoint/doc/generated/command_class_thermostat_setpoint_mqtt_interface.md)
- [COMMAND_CLASS_VERSION](command_class_version/doc/generated/command_class_version_mqtt_interface.md)
- [COMMAND_CLASS_WAKE_UP](command_class_wake_up/doc/generated/command_class_wake_up_mqtt_interface.md)
- [COMMAND_CLASS_SWITCH_COLOR](command_class_switch_color/doc/generated/command_class_switch_color_mqtt_interface.md)
- [COMMAND_CLASS_ASSOCIATION_GRP_INFO](command_class_association_grp_info/doc/generated/command_class_association_grp_info_mqtt_interface.md)
- [COMMAND_CLASS_DEVICE_RESET_LOCALLY](command_class_device_reset_locally/doc/generated/command_class_device_reset_locally_mqtt_interface.md)
- [COMMAND_CLASS_ZWAVEPLUS_INFO](command_class_zwaveplus_info/doc/generated/command_class_zwaveplus_info_mqtt_interface.md)
- [COMMAND_CLASS_SUPERVISION](command_class_supervision/doc/generated/command_class_supervision_mqtt_interface.md)
- [COMMAND_CLASS_INCLUSION_CONTROLLER](command_class_inclusion_controller/doc/generated/command_class_inclusion_controller_mqtt_interface.md)
