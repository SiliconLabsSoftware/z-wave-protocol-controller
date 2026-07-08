## COMMAND_CLASS_THERMOSTAT_FAN_MODE MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [THERMOSTAT_FAN_MODE_GET](#thermostat_fan_mode_get)
- [THERMOSTAT_FAN_MODE_REPORT](#thermostat_fan_mode_report)
- [THERMOSTAT_FAN_MODE_SET](#thermostat_fan_mode_set)
- [THERMOSTAT_FAN_MODE_SUPPORTED_GET](#thermostat_fan_mode_supported_get)
- [THERMOSTAT_FAN_MODE_SUPPORTED_REPORT](#thermostat_fan_mode_supported_report)
### THERMOSTAT_FAN_MODE_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatFanMode/Command/ThermostatFanModeGet
```

**Payload:**
```json
{ }
```
### THERMOSTAT_FAN_MODE_REPORT
                                                                                                                                        
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatFanMode/Report/ThermostatFanModeReport
```

**Payload:**
```json
{
  "properties1": {
    "fan_mode": "0x05",
    "off": "0x05"
  }
}
```
### THERMOSTAT_FAN_MODE_SET
                                                                                                                                        
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatFanMode/Command/ThermostatFanModeSet
```

**Payload:**
```json
{
  "properties1": {
    "fan_mode": "0x05",
    "off": "0x05"
  }
}
```
### THERMOSTAT_FAN_MODE_SUPPORTED_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatFanMode/Command/ThermostatFanModeSupportedGet
```

**Payload:**
```json
{ }
```
### THERMOSTAT_FAN_MODE_SUPPORTED_REPORT
            
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatFanMode/Report/ThermostatFanModeSupportedReport
```

**Payload:**
```json
{ }
```
