## COMMAND_CLASS_THERMOSTAT_FAN_STATE MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [THERMOSTAT_FAN_STATE_GET](#thermostat_fan_state_get)
- [THERMOSTAT_FAN_STATE_REPORT](#thermostat_fan_state_report)
### THERMOSTAT_FAN_STATE_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatFanState/Command/ThermostatFanStateGet
```

**Payload:**
```json
{ }
```
### THERMOSTAT_FAN_STATE_REPORT
                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatFanState/Report/ThermostatFanStateReport
```

**Payload:**
```json
{
  "level": {
    "fan_operating_state": "0x05"
  }
}
```
