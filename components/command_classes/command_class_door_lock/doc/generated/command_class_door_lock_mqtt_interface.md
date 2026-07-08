## COMMAND_CLASS_DOOR_LOCK MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [DOOR_LOCK_CONFIGURATION_GET](#door_lock_configuration_get)
- [DOOR_LOCK_CONFIGURATION_REPORT](#door_lock_configuration_report)
- [DOOR_LOCK_CONFIGURATION_SET](#door_lock_configuration_set)
- [DOOR_LOCK_OPERATION_GET](#door_lock_operation_get)
- [DOOR_LOCK_OPERATION_REPORT](#door_lock_operation_report)
- [DOOR_LOCK_OPERATION_SET](#door_lock_operation_set)
- [DOOR_LOCK_CAPABILITIES_GET](#door_lock_capabilities_get)
- [DOOR_LOCK_CAPABILITIES_REPORT](#door_lock_capabilities_report)
### DOOR_LOCK_CONFIGURATION_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Command/DoorLockConfigurationGet
```

**Payload:**
```json
{ }
```
### DOOR_LOCK_CONFIGURATION_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Report/DoorLockConfigurationReport
```

**Payload:**
```json
{
  "operation_type": "0x01",
  "properties1": {
    "inside_door_handles_enabled": "0x05",
    "outside_door_handles_enabled": "0x05"
  },
  "lock_timeout_minutes": "0x12",
  "lock_timeout_seconds": "0x12",
  "auto_relock_time": "0x2574",
  "hold_and_release_time": "0x2574",
  "properties2": {
    "ta": "0x05",
    "btb": "0x05",
    "reserved1": "0x05"
  }
}
```
### DOOR_LOCK_CONFIGURATION_SET
                                                                                                                                                                                                                                                                                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Command/DoorLockConfigurationSet
```

**Payload:**
```json
{
  "operation_type": "0x01",
  "properties1": {
    "inside_door_handles_enabled": "0x05",
    "outside_door_handles_enabled": "0x05"
  },
  "lock_timeout_minutes": "0x12",
  "lock_timeout_seconds": "0x12",
  "auto_relock_time": "0x2574",
  "hold_and_release_time": "0x2574",
  "properties2": {
    "ta": "0x05",
    "btb": "0x05",
    "reserved1": "0x05"
  }
}
```
### DOOR_LOCK_OPERATION_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Command/DoorLockOperationGet
```

**Payload:**
```json
{ }
```
### DOOR_LOCK_OPERATION_REPORT
                                                                                                                                                                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Report/DoorLockOperationReport
```

**Payload:**
```json
{
  "current_door_lock_mode": "0x01",
  "properties1": {
    "inside_door_handles_mode": "0x05",
    "outside_door_handles_mode": "0x05"
  },
  "door_condition": "0x12",
  "remaining_lock_time_minutes": "0x12",
  "remaining_lock_time_seconds": "0x12",
  "target_door_lock_mode": "0x01",
  "duration": "0x01"
}
```
### DOOR_LOCK_OPERATION_SET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Command/DoorLockOperationSet
```

**Payload:**
```json
{
  "door_lock_mode": "0x01"
}
```
### DOOR_LOCK_CAPABILITIES_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Command/DoorLockCapabilitiesGet
```

**Payload:**
```json
{ }
```
### DOOR_LOCK_CAPABILITIES_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/DoorLock/Report/DoorLockCapabilitiesReport
```

**Payload:**
```json
{
  "properties1": {
    "supported_operation_type_bit_mask_length": "0x05"
  },
  "supported_operation_type_bit_mask": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "supported_door_lock_mode_list_length": "0x12",
  "supported_door_lock_mode": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "properties2": {
    "supported_inside_handle_modes_bitmask": "0x05",
    "supported_outside_handle_modes_bitmask": "0x05"
  },
  "supported_door_components": "0x12",
  "properties3": {
    "btbs": "0x05",
    "tas": "0x05",
    "hrs": "0x05",
    "ars": "0x05"
  }
}
```
