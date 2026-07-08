## COMMAND_CLASS_SWITCH_MULTILEVEL MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [SWITCH_MULTILEVEL_GET](#switch_multilevel_get)
- [SWITCH_MULTILEVEL_REPORT](#switch_multilevel_report)
- [SWITCH_MULTILEVEL_SET](#switch_multilevel_set)
- [SWITCH_MULTILEVEL_START_LEVEL_CHANGE](#switch_multilevel_start_level_change)
- [SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE](#switch_multilevel_stop_level_change)
- [SWITCH_MULTILEVEL_SUPPORTED_GET](#switch_multilevel_supported_get)
- [SWITCH_MULTILEVEL_SUPPORTED_REPORT](#switch_multilevel_supported_report)
### SWITCH_MULTILEVEL_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchMultilevel/Command/SwitchMultilevelGet
```

**Payload:**
```json
{ }
```
### SWITCH_MULTILEVEL_REPORT
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchMultilevel/Report/SwitchMultilevelReport
```

**Payload:**
```json
{
  "current_value": "0x12",
  "target_value": "0x12",
  "duration": "0x01"
}
```
### SWITCH_MULTILEVEL_SET
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchMultilevel/Command/SwitchMultilevelSet
```

**Payload:**
```json
{
  "value": "0x12",
  "duration": "0x12"
}
```
### SWITCH_MULTILEVEL_START_LEVEL_CHANGE
                                                                                                                                                                                                                                                                                            
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchMultilevel/Command/SwitchMultilevelStartLevelChange
```

**Payload:**
```json
{
  "properties1": {
    "inc_dec": "0x05",
    "ignore_start_level": "0x05",
    "up_down": "0x05"
  },
  "start_level": "0x12",
  "dimming_duration": "0x12",
  "step_size": "0x12"
}
```
### SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchMultilevel/Command/SwitchMultilevelStopLevelChange
```

**Payload:**
```json
{ }
```
### SWITCH_MULTILEVEL_SUPPORTED_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchMultilevel/Command/SwitchMultilevelSupportedGet
```

**Payload:**
```json
{ }
```
### SWITCH_MULTILEVEL_SUPPORTED_REPORT
                                                                                                                                                                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchMultilevel/Report/SwitchMultilevelSupportedReport
```

**Payload:**
```json
{
  "properties1": {
    "primary_switch_type": "0x05",
    "reserved1": "0x05"
  },
  "properties2": {
    "secondary_switch_type": "0x05",
    "reserved2": "0x05"
  }
}
```
