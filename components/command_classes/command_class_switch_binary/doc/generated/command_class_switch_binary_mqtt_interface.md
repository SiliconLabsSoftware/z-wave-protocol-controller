## COMMAND_CLASS_SWITCH_BINARY MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [SWITCH_BINARY_GET](#switch_binary_get)
- [SWITCH_BINARY_REPORT](#switch_binary_report)
- [SWITCH_BINARY_SET](#switch_binary_set)
### SWITCH_BINARY_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchBinary/Command/SwitchBinaryGet
```

**Payload:**
```json
{ }
```
### SWITCH_BINARY_REPORT
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchBinary/Report/SwitchBinaryReport
```

**Payload:**
```json
{
  "current_value": "0x01",
  "target_value": "0x01",
  "duration": "0x01"
}
```
### SWITCH_BINARY_SET
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SwitchBinary/Command/SwitchBinarySet
```

**Payload:**
```json
{
  "target_value": "0x01",
  "duration": "0x01"
}
```
