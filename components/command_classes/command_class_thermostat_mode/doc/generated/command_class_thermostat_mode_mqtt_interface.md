## COMMAND_CLASS_THERMOSTAT_MODE MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [THERMOSTAT_MODE_GET](#thermostat_mode_get)
- [THERMOSTAT_MODE_REPORT](#thermostat_mode_report)
- [THERMOSTAT_MODE_SET](#thermostat_mode_set)
- [THERMOSTAT_MODE_SUPPORTED_GET](#thermostat_mode_supported_get)
- [THERMOSTAT_MODE_SUPPORTED_REPORT](#thermostat_mode_supported_report)
### THERMOSTAT_MODE_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatMode/Command/ThermostatModeGet
```

**Payload:**
```json
{ }
```
### THERMOSTAT_MODE_REPORT
                                                                                                                                                      
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatMode/Report/ThermostatModeReport
```

**Payload:**
```json
{
  "level": {
    "mode": "0x05",
    "no_of_manufacturer_data_fields": "0x05"
  },
  "manufacturer_data": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### THERMOSTAT_MODE_SET
                                                                                                                                                      
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatMode/Command/ThermostatModeSet
```

**Payload:**
```json
{
  "level": {
    "mode": "0x05",
    "no_of_manufacturer_data_fields": "0x05"
  },
  "manufacturer_data": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### THERMOSTAT_MODE_SUPPORTED_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatMode/Command/ThermostatModeSupportedGet
```

**Payload:**
```json
{ }
```
### THERMOSTAT_MODE_SUPPORTED_REPORT
            
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatMode/Report/ThermostatModeSupportedReport
```

**Payload:**
```json
{ }
```
