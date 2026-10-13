## COMMAND_CLASS_SCHEDULE MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [SCHEDULE_SUPPORTED_GET](#schedule_supported_get)
- [SCHEDULE_SUPPORTED_REPORT](#schedule_supported_report)
- [COMMAND_SCHEDULE_SET](#command_schedule_set)
- [COMMAND_SCHEDULE_GET](#command_schedule_get)
- [COMMAND_SCHEDULE_REPORT](#command_schedule_report)
- [SCHEDULE_REMOVE](#schedule_remove)
- [SCHEDULE_STATE_SET](#schedule_state_set)
- [SCHEDULE_STATE_GET](#schedule_state_get)
- [SCHEDULE_STATE_REPORT](#schedule_state_report)
- [SCHEDULE_SUPPORTED_COMMANDS_GET](#schedule_supported_commands_get)
- [SCHEDULE_SUPPORTED_COMMANDS_REPORT](#schedule_supported_commands_report)
### SCHEDULE_SUPPORTED_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Schedule/Command/ScheduleSupportedGet
```

**Payload:**
```json
{
  "schedule_id_block": "0x12"
}
```
### SCHEDULE_SUPPORTED_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Schedule/Report/ScheduleSupportedReport
```

**Payload:**
```json
{
  "number_of_supported_schedule_id": "0x12",
  "properties1": {
    "start_time_support": "0x05",
    "fallback_support": "0x05",
    "support_enable_disable": "0x05"
  },
  "number_of_supported_cc": "0x12",
  "vg1": [
    {
      "supported_cc": "0x12",
      "properties2": {
        "supported_command": "0x05"
      }
    }
  ],
  "properties3": {
    "supported_override_types": "0x05",
    "override_support": "0x05"
  },
  "schedule_id_block": "0x12",
  "number_of_supported_schedule_blocks": "0x12"
}
```
### COMMAND_SCHEDULE_SET
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Schedule/Command/CommandScheduleSet
```

**Payload:**
```json
{
  "schedule_id": "0x12",
  "schedule_id_block": "0x12",
  "start_year": "0x12",
  "properties1": {
    "start_month": "0x05",
    "recurrence_offset": "0x05"
  },
  "properties2": {
    "start_day_of_month": "0x05",
    "recurrence_mode": "0x05",
    "reserved1": "0x05"
  },
  "properties3": {
    "start_weekday": "0x05",
    "reserved2": "0x05"
  },
  "properties4": {
    "start_hour": "0x05",
    "duration_type": "0x05"
  },
  "properties5": {
    "start_minute": "0x05",
    "relative": "0x05",
    "reserved3": "0x05"
  },
  "duration_byte": "0x2574",
  "reports_to_follow": "0x12",
  "number_of_cmd_to_follow": "0x12",
  "vg1": [
    {
      "cmd_length": "0x12",
      "cmd_byte": [
        "0x01",
        "0x02",
        "0x03"
      ]
    }
  ]
}
```
### COMMAND_SCHEDULE_GET
                                                                                                                                                                        
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Schedule/Command/CommandScheduleGet
```

**Payload:**
```json
{
  "schedule_id": "0x12",
  "schedule_id_block": "0x12",
  "properties1": {
    "aid_ro_ctl": "0x05"
  }
}
```
### COMMAND_SCHEDULE_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Schedule/Report/CommandScheduleReport
```

**Payload:**
```json
{
  "schedule_id": "0x12",
  "schedule_id_block": "0x12",
  "start_year": "0x12",
  "properties1": {
    "start_month": "0x05",
    "aid_ro": "0x05"
  },
  "properties2": {
    "start_day_of_month": "0x05",
    "recurrence_mode": "0x05",
    "aid_ro_ctl": "0x05"
  },
  "properties3": {
    "start_weekday": "0x05",
    "reserved1": "0x05"
  },
  "properties4": {
    "start_hour": "0x05",
    "duration_type": "0x05"
  },
  "properties5": {
    "start_minute": "0x05",
    "relative": "0x05",
    "reserved2": "0x05"
  },
  "duration_byte": "0x2574",
  "reports_to_follow": "0x12",
  "number_of_cmd_to_follow": "0x12",
  "vg1": [
    {
      "cmd_length": "0x12",
      "cmd_byte": [
        "0x01",
        "0x02",
        "0x03"
      ]
    }
  ]
}
```
### SCHEDULE_REMOVE
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Schedule/Command/ScheduleRemove
```

**Payload:**
```json
{
  "schedule_id": "0x12",
  "schedule_id_block": "0x12"
}
```
### SCHEDULE_STATE_SET
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Schedule/Command/ScheduleStateSet
```

**Payload:**
```json
{
  "schedule_id": "0x12",
  "schedule_state": "0x12",
  "schedule_id_block": "0x12"
}
```
### SCHEDULE_STATE_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Schedule/Command/ScheduleStateGet
```

**Payload:**
```json
{
  "schedule_id_block": "0x12"
}
```
### SCHEDULE_STATE_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Schedule/Report/ScheduleStateReport
```

**Payload:**
```json
{
  "number_of_supported_schedule_id": "0x12",
  "properties1": {
    "override": "0x05",
    "reports_to_follow": "0x05"
  },
  "vg1": [
    {
      "properties2": {
        "active_id_1": "0x05",
        "active_id_2": "0x05"
      }
    }
  ],
  "schedule_id_block": "0x12"
}
```
### SCHEDULE_SUPPORTED_COMMANDS_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Schedule/Command/ScheduleSupportedCommandsGet
```

**Payload:**
```json
{
  "schedule_id_block": "0x12"
}
```
### SCHEDULE_SUPPORTED_COMMANDS_REPORT
                                                                                                                                                                                                                                                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Schedule/Report/ScheduleSupportedCommandsReport
```

**Payload:**
```json
{
  "schedule_id_block": "0x12",
  "command_class_list_length": "0x12",
  "vg1": [
    {
      "command_class": "0x12",
      "supported_command_list_length": "0x12",
      "supported_command": [
        "0x01",
        "0x02",
        "0x03"
      ]
    }
  ]
}
```
