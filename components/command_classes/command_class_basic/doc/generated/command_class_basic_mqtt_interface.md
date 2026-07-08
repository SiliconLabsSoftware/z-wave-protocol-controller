## COMMAND_CLASS_BASIC MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [BASIC_GET](#basic_get)
- [BASIC_REPORT](#basic_report)
- [BASIC_SET](#basic_set)
### BASIC_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Basic/Command/BasicGet
```

**Payload:**
```json
{ }
```
### BASIC_REPORT
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Basic/Report/BasicReport
```

**Payload:**
```json
{
  "current_value": "0x12",
  "target_value": "0x12",
  "duration": "0x12"
}
```
### BASIC_SET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Basic/Command/BasicSet
```

**Payload:**
```json
{
  "value": "0x12"
}
```
