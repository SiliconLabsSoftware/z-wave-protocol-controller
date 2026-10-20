## COMMAND_CLASS_TIME MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | true | true |

### Table of Contents
- [DATE_GET](#date_get)
- [DATE_REPORT](#date_report)
- [TIME_GET](#time_get)
- [TIME_OFFSET_GET](#time_offset_get)
- [TIME_OFFSET_REPORT](#time_offset_report)
- [TIME_OFFSET_SET](#time_offset_set)
- [TIME_REPORT](#time_report)
- [DATE_SET](#date_set)
- [TIME_SET](#time_set)
### DATE_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Time/Command/DateGet
```

**Payload:**
```json
{ }
```
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
### TIME_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Time/Command/TimeGet
```

**Payload:**
```json
{ }
```
### TIME_OFFSET_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Time/Command/TimeOffsetGet
```

**Payload:**
```json
{ }
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
### TIME_OFFSET_SET
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Time/Command/TimeOffsetSet
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
### DATE_SET
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Time/Command/DateSet
```

**Payload:**
```json
{
  "year": "0x2574",
  "month": "0x12",
  "day": "0x12"
}
```
### TIME_SET
                                                                                                                                                                        
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Time/Command/TimeSet
```

**Payload:**
```json
{
  "properties1": {
    "hour": "0x05"
  },
  "minute": "0x12",
  "second": "0x12"
}
```
