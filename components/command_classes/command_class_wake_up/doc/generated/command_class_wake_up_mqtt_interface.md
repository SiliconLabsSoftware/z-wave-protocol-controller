## COMMAND_CLASS_WAKE_UP MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [WAKE_UP_INTERVAL_CAPABILITIES_GET](#wake_up_interval_capabilities_get)
- [WAKE_UP_INTERVAL_CAPABILITIES_REPORT](#wake_up_interval_capabilities_report)
- [WAKE_UP_INTERVAL_GET](#wake_up_interval_get)
- [WAKE_UP_INTERVAL_REPORT](#wake_up_interval_report)
- [WAKE_UP_INTERVAL_SET](#wake_up_interval_set)
- [WAKE_UP_NO_MORE_INFORMATION](#wake_up_no_more_information)
- [WAKE_UP_NOTIFICATION](#wake_up_notification)
### WAKE_UP_INTERVAL_CAPABILITIES_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/WakeUp/Command/WakeUpIntervalCapabilitiesGet
```

**Payload:**
```json
{ }
```
### WAKE_UP_INTERVAL_CAPABILITIES_REPORT
                                                                                                                                                                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/WakeUp/Report/WakeUpIntervalCapabilitiesReport
```

**Payload:**
```json
{
  "minimum_wake_up_interval_seconds": "0x123",
  "maximum_wake_up_interval_seconds": "0x123",
  "default_wake_up_interval_seconds": "0x123",
  "wake_up_interval_step_seconds": "0x123",
  "properties1": {
    "wake_up_on_demand": "0x05"
  }
}
```
### WAKE_UP_INTERVAL_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/WakeUp/Command/WakeUpIntervalGet
```

**Payload:**
```json
{ }
```
### WAKE_UP_INTERVAL_REPORT
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/WakeUp/Report/WakeUpIntervalReport
```

**Payload:**
```json
{
  "seconds": "0x123",
  "nodeid": "0x12"
}
```
### WAKE_UP_INTERVAL_SET
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/WakeUp/Command/WakeUpIntervalSet
```

**Payload:**
```json
{
  "seconds": "0x123",
  "nodeid": "0x12"
}
```
### WAKE_UP_NO_MORE_INFORMATION
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/WakeUp/Command/WakeUpNoMoreInformation
```

**Payload:**
```json
{ }
```
### WAKE_UP_NOTIFICATION
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/WakeUp/Report/WakeUpNotification
```

**Payload:**
```json
{ }
```
