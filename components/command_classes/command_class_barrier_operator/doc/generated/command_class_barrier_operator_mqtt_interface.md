## COMMAND_CLASS_BARRIER_OPERATOR MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [BARRIER_OPERATOR_SET](#barrier_operator_set)
- [BARRIER_OPERATOR_GET](#barrier_operator_get)
- [BARRIER_OPERATOR_REPORT](#barrier_operator_report)
- [BARRIER_OPERATOR_SIGNAL_SUPPORTED_GET](#barrier_operator_signal_supported_get)
- [BARRIER_OPERATOR_SIGNAL_SUPPORTED_REPORT](#barrier_operator_signal_supported_report)
- [BARRIER_OPERATOR_EVENT_SIGNAL_SET](#barrier_operator_event_signal_set)
- [BARRIER_OPERATOR_EVENT_SIGNALING_GET](#barrier_operator_event_signaling_get)
- [BARRIER_OPERATOR_EVENT_SIGNALING_REPORT](#barrier_operator_event_signaling_report)
### BARRIER_OPERATOR_SET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/BarrierOperator/Command/BarrierOperatorSet
```

**Payload:**
```json
{
  "target_value": "0x01"
}
```
### BARRIER_OPERATOR_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/BarrierOperator/Command/BarrierOperatorGet
```

**Payload:**
```json
{ }
```
### BARRIER_OPERATOR_REPORT
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/BarrierOperator/Report/BarrierOperatorReport
```

**Payload:**
```json
{
  "state": "0x01"
}
```
### BARRIER_OPERATOR_SIGNAL_SUPPORTED_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/BarrierOperator/Command/BarrierOperatorSignalSupportedGet
```

**Payload:**
```json
{ }
```
### BARRIER_OPERATOR_SIGNAL_SUPPORTED_REPORT
            
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/BarrierOperator/Report/BarrierOperatorSignalSupportedReport
```

**Payload:**
```json
{ }
```
### BARRIER_OPERATOR_EVENT_SIGNAL_SET
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/BarrierOperator/Command/BarrierOperatorEventSignalSet
```

**Payload:**
```json
{
  "subsystem_type": "0x12",
  "subsystem_state": "0x01"
}
```
### BARRIER_OPERATOR_EVENT_SIGNALING_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/BarrierOperator/Command/BarrierOperatorEventSignalingGet
```

**Payload:**
```json
{
  "subsystem_type": "0x12"
}
```
### BARRIER_OPERATOR_EVENT_SIGNALING_REPORT
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/BarrierOperator/Report/BarrierOperatorEventSignalingReport
```

**Payload:**
```json
{
  "subsystem_type": "0x12",
  "subsystem_state": "0x01"
}
```
