## COMMAND_CLASS_TIME MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| false | true | false |

> When MQTT Support is `false`, only incoming (TX) commands are listed below. ZPC publishes these to MQTT but does not handle `Command/*` topics for this command class.

### Table of Contents
- [DATE_REPORT](#date_report)
- [TIME_OFFSET_REPORT](#time_offset_report)
- [TIME_REPORT](#time_report)
### DATE_REPORT
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Time/Report/DateReport
```

**Payload:**
```json
{
  "year": "0x2574",
  "month": "0x12",
  "day": "0x12"
}
```
### TIME_OFFSET_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Time/Report/TimeOffsetReport
```

**Payload:**
```json
{
  "level": {
    "hour_tzo": "0x05",
    "sign_tzo": "0x05"
  },
  "minute_tzo": "0x12",
  "level2": {
    "minute_offset_dst": "0x05",
    "sign_offset_dst": "0x05"
  },
  "month_start_dst": "0x12",
  "day_start_dst": "0x12",
  "hour_start_dst": "0x12",
  "month_end_dst": "0x12",
  "day_end_dst": "0x12",
  "hour_end_dst": "0x12"
}
```
### TIME_REPORT
                                                                                                                                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Time/Report/TimeReport
```

**Payload:**
```json
{
  "properties1": {
    "hour_local_time": "0x05",
    "time_source": "0x05",
    "rtc_failure": "0x05"
  },
  "minute_local_time": "0x12",
  "second_local_time": "0x12"
}
```
