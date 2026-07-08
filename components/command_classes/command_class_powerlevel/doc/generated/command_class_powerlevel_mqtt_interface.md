## COMMAND_CLASS_POWERLEVEL MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| false | true | false |

> When MQTT Support is `false`, only incoming (TX) commands are listed below. ZPC publishes these to MQTT but does not handle `Command/*` topics for this command class.

### Table of Contents
- [POWERLEVEL_REPORT](#powerlevel_report)
- [POWERLEVEL_TEST_NODE_REPORT](#powerlevel_test_node_report)
### POWERLEVEL_REPORT
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Powerlevel/Report/PowerlevelReport
```

**Payload:**
```json
{
  "power_level": "0x01",
  "timeout": "0x12"
}
```
### POWERLEVEL_TEST_NODE_REPORT
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Powerlevel/Report/PowerlevelTestNodeReport
```

**Payload:**
```json
{
  "test_nodeid": "0x01",
  "status_of_operation": "0x01",
  "test_frame_count": "0x2574"
}
```
