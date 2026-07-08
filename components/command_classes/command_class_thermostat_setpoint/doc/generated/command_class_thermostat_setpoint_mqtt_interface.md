## COMMAND_CLASS_THERMOSTAT_SETPOINT MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [THERMOSTAT_SETPOINT_GET](#thermostat_setpoint_get)
- [THERMOSTAT_SETPOINT_REPORT](#thermostat_setpoint_report)
- [THERMOSTAT_SETPOINT_SET](#thermostat_setpoint_set)
- [THERMOSTAT_SETPOINT_SUPPORTED_GET](#thermostat_setpoint_supported_get)
- [THERMOSTAT_SETPOINT_SUPPORTED_REPORT](#thermostat_setpoint_supported_report)
- [THERMOSTAT_SETPOINT_CAPABILITIES_GET](#thermostat_setpoint_capabilities_get)
- [THERMOSTAT_SETPOINT_CAPABILITIES_REPORT](#thermostat_setpoint_capabilities_report)
### THERMOSTAT_SETPOINT_GET
                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetpoint/Command/ThermostatSetpointGet
```

**Payload:**
```json
{
  "level": {
    "setpoint_type": "0x05"
  }
}
```
### THERMOSTAT_SETPOINT_REPORT
                                                                                                                                                                                                                                                                                        
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetpoint/Report/ThermostatSetpointReport
```

**Payload:**
```json
{
  "level": {
    "setpoint_type": "0x05"
  },
  "level2": {
    "size": "0x05",
    "scale": "0x05",
    "precision": "0x05"
  },
  "value": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### THERMOSTAT_SETPOINT_SET
                                                                                                                                                                                                                                                                                        
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetpoint/Command/ThermostatSetpointSet
```

**Payload:**
```json
{
  "level": {
    "setpoint_type": "0x05"
  },
  "level2": {
    "size": "0x05",
    "scale": "0x05",
    "precision": "0x05"
  },
  "value": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### THERMOSTAT_SETPOINT_SUPPORTED_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetpoint/Command/ThermostatSetpointSupportedGet
```

**Payload:**
```json
{ }
```
### THERMOSTAT_SETPOINT_SUPPORTED_REPORT
            
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetpoint/Report/ThermostatSetpointSupportedReport
```

**Payload:**
```json
{ }
```
### THERMOSTAT_SETPOINT_CAPABILITIES_GET
                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetpoint/Command/ThermostatSetpointCapabilitiesGet
```

**Payload:**
```json
{
  "properties1": {
    "setpoint_type": "0x05"
  }
}
```
### THERMOSTAT_SETPOINT_CAPABILITIES_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetpoint/Report/ThermostatSetpointCapabilitiesReport
```

**Payload:**
```json
{
  "properties1": {
    "setpoint_type": "0x05"
  },
  "properties2": {
    "size1": "0x05",
    "scale1": "0x05",
    "precision1": "0x05"
  },
  "min_value": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "properties3": {
    "size2": "0x05",
    "scale2": "0x05",
    "precision2": "0x05"
  },
  "maxvalue": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
