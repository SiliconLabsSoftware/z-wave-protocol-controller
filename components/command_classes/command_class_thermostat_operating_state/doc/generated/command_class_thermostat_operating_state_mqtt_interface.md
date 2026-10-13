## COMMAND_CLASS_THERMOSTAT_OPERATING_STATE MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [THERMOSTAT_OPERATING_STATE_GET](#thermostat_operating_state_get)
- [THERMOSTAT_OPERATING_STATE_REPORT](#thermostat_operating_state_report)
- [THERMOSTAT_OPERATING_STATE_LOGGING_SUPPORTED_GET](#thermostat_operating_state_logging_supported_get)
- [THERMOSTAT_OPERATING_LOGGING_SUPPORTED_REPORT](#thermostat_operating_logging_supported_report)
- [THERMOSTAT_OPERATING_STATE_LOGGING_GET](#thermostat_operating_state_logging_get)
- [THERMOSTAT_OPERATING_STATE_LOGGING_REPORT](#thermostat_operating_state_logging_report)
### THERMOSTAT_OPERATING_STATE_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatOperatingState/Command/ThermostatOperatingStateGet
```

**Payload:**
```json
{ }
```
### THERMOSTAT_OPERATING_STATE_REPORT
                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatOperatingState/Report/ThermostatOperatingStateReport
```

**Payload:**
```json
{
  "properties1": {
    "operating_state": "0x05"
  }
}
```
### THERMOSTAT_OPERATING_STATE_LOGGING_SUPPORTED_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatOperatingState/Command/ThermostatOperatingStateLoggingSupportedGet
```

**Payload:**
```json
{ }
```
### THERMOSTAT_OPERATING_LOGGING_SUPPORTED_REPORT
            
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatOperatingState/Report/ThermostatOperatingLoggingSupportedReport
```

**Payload:**
```json
{ }
```
### THERMOSTAT_OPERATING_STATE_LOGGING_GET
            
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatOperatingState/Command/ThermostatOperatingStateLoggingGet
```

**Payload:**
```json
{ }
```
### THERMOSTAT_OPERATING_STATE_LOGGING_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ThermostatOperatingState/Report/ThermostatOperatingStateLoggingReport
```

**Payload:**
```json
{
  "reports_to_follow": "0x12",
  "vg1": [
    {
      "properties1": {
        "operating_state_log_type": "0x05"
      },
      "usage_today_(hours)": "0x12",
      "usage_today_(minutes)": "0x12",
      "usage_yesterday_(hours)": "0x12",
      "usage_yesterday_(minutes)": "0x12"
    }
  ]
}
```
