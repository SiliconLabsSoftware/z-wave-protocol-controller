## COMMAND_CLASS_NOTIFICATION MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [NOTIFICATION_GET](#notification_get)
- [NOTIFICATION_REPORT](#notification_report)
- [NOTIFICATION_SET](#notification_set)
- [NOTIFICATION_SUPPORTED_GET](#notification_supported_get)
- [NOTIFICATION_SUPPORTED_REPORT](#notification_supported_report)
- [EVENT_SUPPORTED_GET](#event_supported_get)
- [EVENT_SUPPORTED_REPORT](#event_supported_report)
### NOTIFICATION_GET
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Notification/Command/NotificationGet
```

**Payload:**
```json
{
  "v1_alarm_type": "0x12",
  "notification_type": "0x01",
  "event": "0x12"
}
```
### NOTIFICATION_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                            
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Notification/Report/NotificationReport
```

**Payload:**
```json
{
  "v1_alarm_type": "0x12",
  "v1_alarm_level": "0x12",
  "notification_status": "0x01",
  "notification_type": "0x01",
  "event": "0x12",
  "properties1": {
    "event_parameters_length": "0x05",
    "reserved2": "0x05",
    "sequence": "0x05"
  },
  "event_parameter": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "sequence_number": "0x12"
}
```
### NOTIFICATION_SET
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Notification/Command/NotificationSet
```

**Payload:**
```json
{
  "notification_type": "0x01",
  "notification_status": "0x01"
}
```
### NOTIFICATION_SUPPORTED_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Notification/Command/NotificationSupportedGet
```

**Payload:**
```json
{ }
```
### NOTIFICATION_SUPPORTED_REPORT
                                                                                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Notification/Report/NotificationSupportedReport
```

**Payload:**
```json
{
  "properties1": {
    "number_of_bit_masks": "0x05",
    "v1_alarm": "0x05"
  }
}
```
### EVENT_SUPPORTED_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Notification/Command/EventSupportedGet
```

**Payload:**
```json
{
  "notification_type": "0x01"
}
```
### EVENT_SUPPORTED_REPORT
                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Notification/Report/EventSupportedReport
```

**Payload:**
```json
{
  "notification_type": "0x01",
  "properties1": {
    "number_of_bit_masks": "0x05"
  }
}
```
