## COMMAND_CLASS_BATTERY MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [BATTERY_GET](#battery_get)
- [BATTERY_REPORT](#battery_report)
- [BATTERY_HEALTH_GET](#battery_health_get)
- [BATTERY_HEALTH_REPORT](#battery_health_report)
### BATTERY_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Battery/Command/BatteryGet
```

**Payload:**
```json
{ }
```
### BATTERY_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Battery/Report/BatteryReport
```

**Payload:**
```json
{
  "battery_level": "0x12",
  "properties1": {
    "replace_recharge": "0x05",
    "low_fluid": "0x05",
    "overheating": "0x05",
    "backup_battery": "0x05",
    "rechargeable": "0x05",
    "charging_status": "0x05"
  },
  "properties2": {
    "disconnected": "0x05",
    "low_temperature_status": "0x05",
    "reserved1": "0x05"
  }
}
```
### BATTERY_HEALTH_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Battery/Command/BatteryHealthGet
```

**Payload:**
```json
{ }
```
### BATTERY_HEALTH_REPORT
                                                                                                                                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Battery/Report/BatteryHealthReport
```

**Payload:**
```json
{
  "maximum_capacity": "0x12",
  "properties1": {
    "size": "0x05",
    "scale": "0x05",
    "precision": "0x05"
  },
  "battery_temperature": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
