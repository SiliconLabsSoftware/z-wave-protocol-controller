## COMMAND_CLASS_SWITCH_COLOR MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [SWITCH_COLOR_SUPPORTED_GET](#switch_color_supported_get)
- [SWITCH_COLOR_SUPPORTED_REPORT](#switch_color_supported_report)
- [SWITCH_COLOR_GET](#switch_color_get)
- [SWITCH_COLOR_REPORT](#switch_color_report)
- [SWITCH_COLOR_SET](#switch_color_set)
- [SWITCH_COLOR_START_LEVEL_CHANGE](#switch_color_start_level_change)
- [SWITCH_COLOR_STOP_LEVEL_CHANGE](#switch_color_stop_level_change)
### SWITCH_COLOR_SUPPORTED_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchColor/Command/SwitchColorSupportedGet
```

**Payload:**
```json
{ }
```
### SWITCH_COLOR_SUPPORTED_REPORT
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchColor/Report/SwitchColorSupportedReport
```

**Payload:**
```json
{
  "color_component_mask": "0x2574"
}
```
### SWITCH_COLOR_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchColor/Command/SwitchColorGet
```

**Payload:**
```json
{
  "color_component_id": "0x12"
}
```
### SWITCH_COLOR_REPORT
                                                                                                                                                      
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchColor/Report/SwitchColorReport
```

**Payload:**
```json
{
  "color_component_id": "0x12",
  "current_value": "0x12",
  "target_value": "0x12",
  "duration": "0x12"
}
```
### SWITCH_COLOR_SET
                                                                                                                                                                                                                                                                                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchColor/Command/SwitchColorSet
```

**Payload:**
```json
{
  "properties1": {
    "color_component_count": "0x05"
  },
  "vg1": [
    {
      "color_component_id": "0x12",
      "value": "0x12"
    }
  ],
  "duration": "0x12"
}
```
### SWITCH_COLOR_START_LEVEL_CHANGE
                                                                                                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchColor/Command/SwitchColorStartLevelChange
```

**Payload:**
```json
{
  "properties1": {
    "ignore_start_state": "0x05",
    "up_down": "0x05"
  },
  "color_component_id": "0x12",
  "start_level": "0x12",
  "duration": "0x12"
}
```
### SWITCH_COLOR_STOP_LEVEL_CHANGE
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchColor/Command/SwitchColorStopLevelChange
```

**Payload:**
```json
{
  "color_component_id": "0x12"
}
```
