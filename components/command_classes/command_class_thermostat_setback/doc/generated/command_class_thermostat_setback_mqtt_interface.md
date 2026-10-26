## COMMAND_CLASS_THERMOSTAT_SETBACK MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [THERMOSTAT_SETBACK_GET](#thermostat_setback_get)
- [THERMOSTAT_SETBACK_REPORT](#thermostat_setback_report)
- [THERMOSTAT_SETBACK_SET](#thermostat_setback_set)
### THERMOSTAT_SETBACK_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetback/Command/ThermostatSetbackGet
```

**Payload:**
```json
{ }
```
### THERMOSTAT_SETBACK_REPORT
                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetback/Report/ThermostatSetbackReport
```

**Payload:**
```json
{
  "properties1": {
    "setback_type": "0x05"
  },
  "setback_state": "0x12"
}
```
### THERMOSTAT_SETBACK_SET
                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatSetback/Command/ThermostatSetbackSet
```

**Payload:**
```json
{
  "properties1": {
    "setback_type": "0x05"
  },
  "setback_state": "0x12"
}
```
