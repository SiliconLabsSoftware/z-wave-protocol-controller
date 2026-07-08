## COMMAND_CLASS_ZWAVEPLUS_INFO MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| false | true | true |

> When MQTT Support is `false`, only incoming (TX) commands are listed below. ZPC publishes these to MQTT but does not handle `Command/*` topics for this command class.

### Table of Contents
- [ZWAVEPLUS_INFO_REPORT](#zwaveplus_info_report)
### ZWAVEPLUS_INFO_REPORT
                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ZwaveplusInfo/Report/ZwaveplusInfoReport
```

**Payload:**
```json
{
  "z_wave_plus_version": "0x12",
  "role_type": "0x01",
  "node_type": "0x01",
  "installer_icon_type": "0x2574",
  "user_icon_type": "0x2574"
}
```
