## COMMAND_CLASS_SECURITY_2 MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| false | true | true |

> When MQTT Support is `false`, only incoming (TX) commands are listed below. ZPC publishes these to MQTT but does not handle `Command/*` topics for this command class.

### Table of Contents
- [SECURITY_2_COMMANDS_SUPPORTED_REPORT](#security_2_commands_supported_report)
### SECURITY_2_COMMANDS_SUPPORTED_REPORT
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Security2/Report/Security2CommandsSupportedReport
```

**Payload:**
```json
{
  "command_class": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
