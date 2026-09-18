## COMMAND_CLASS_SECURITY MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| false | true | true |

> When MQTT Support is `false`, only incoming (TX) commands are listed below. ZPC publishes these to MQTT but does not handle `Command/*` topics for this command class.

### Table of Contents
- [SECURITY_COMMANDS_SUPPORTED_REPORT](#security_commands_supported_report)
### SECURITY_COMMANDS_SUPPORTED_REPORT
                                                                                                                                                      
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Security/Report/SecurityCommandsSupportedReport
```

**Payload:**
```json
{
  "reports_to_follow": "0x12",
  "command_class_support": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "command_class_mark": "0xFF",
  "command_class_control": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
