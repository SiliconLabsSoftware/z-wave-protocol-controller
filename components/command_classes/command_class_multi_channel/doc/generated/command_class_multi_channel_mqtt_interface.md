## COMMAND_CLASS_MULTI_CHANNEL MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [MULTI_CHANNEL_CAPABILITY_GET](#multi_channel_capability_get)
- [MULTI_CHANNEL_CAPABILITY_REPORT](#multi_channel_capability_report)
- [MULTI_CHANNEL_CMD_ENCAP](#multi_channel_cmd_encap)
- [MULTI_CHANNEL_END_POINT_FIND](#multi_channel_end_point_find)
- [MULTI_CHANNEL_END_POINT_FIND_REPORT](#multi_channel_end_point_find_report)
- [MULTI_CHANNEL_END_POINT_GET](#multi_channel_end_point_get)
- [MULTI_CHANNEL_END_POINT_REPORT](#multi_channel_end_point_report)
- [MULTI_INSTANCE_CMD_ENCAP](#multi_instance_cmd_encap)
- [MULTI_INSTANCE_GET](#multi_instance_get)
- [MULTI_INSTANCE_REPORT](#multi_instance_report)
- [MULTI_CHANNEL_AGGREGATED_MEMBERS_GET](#multi_channel_aggregated_members_get)
- [MULTI_CHANNEL_AGGREGATED_MEMBERS_REPORT](#multi_channel_aggregated_members_report)
### MULTI_CHANNEL_CAPABILITY_GET
                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Command/MultiChannelCapabilityGet
```

**Payload:**
```json
{
  "properties1": {
    "end_point": "0x05"
  }
}
```
### MULTI_CHANNEL_CAPABILITY_REPORT
                                                                                                                                                                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Report/MultiChannelCapabilityReport
```

**Payload:**
```json
{
  "properties1": {
    "end_point": "0x05",
    "dynamic": "0x05"
  },
  "generic_device_class": "0x12",
  "specific_device_class": "0x12",
  "command_class": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### MULTI_CHANNEL_CMD_ENCAP
                                                                                                                                                                                                                                                                                                                        
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Command/MultiChannelCmdEncap
```

**Payload:**
```json
{
  "properties1": {
    "source_end_point": "0x05"
  },
  "properties2": {
    "destination_end_point": "0x05",
    "bit_address": "0x05"
  },
  "command_class": "0x12",
  "command": "0x12",
  "parameter": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### MULTI_CHANNEL_END_POINT_FIND
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Command/MultiChannelEndPointFind
```

**Payload:**
```json
{
  "generic_device_class": "0x12",
  "specific_device_class": "0x12"
}
```
### MULTI_CHANNEL_END_POINT_FIND_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Report/MultiChannelEndPointFindReport
```

**Payload:**
```json
{
  "reports_to_follow": "0x12",
  "generic_device_class": "0x12",
  "specific_device_class": "0x12",
  "vg": [
    {
      "properties1": {
        "end_point": "0x05"
      }
    }
  ]
}
```
### MULTI_CHANNEL_END_POINT_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Command/MultiChannelEndPointGet
```

**Payload:**
```json
{ }
```
### MULTI_CHANNEL_END_POINT_REPORT
                                                                                                                                                                                                                                                                                                                            
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Report/MultiChannelEndPointReport
```

**Payload:**
```json
{
  "properties1": {
    "identical": "0x05",
    "dynamic": "0x05"
  },
  "properties2": {
    "individual_end_points": "0x05"
  },
  "properties3": {
    "aggregated_end_points": "0x05"
  }
}
```
### MULTI_INSTANCE_CMD_ENCAP
                                                                                                                                                                                                            
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Command/MultiInstanceCmdEncap
```

**Payload:**
```json
{
  "properties1": {
    "instance": "0x05"
  },
  "command_class": "0x12",
  "command": "0x12",
  "parameter": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### MULTI_INSTANCE_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Command/MultiInstanceGet
```

**Payload:**
```json
{
  "command_class": "0x12"
}
```
### MULTI_INSTANCE_REPORT
                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Report/MultiInstanceReport
```

**Payload:**
```json
{
  "command_class": "0x12",
  "properties1": {
    "instances": "0x05"
  }
}
```
### MULTI_CHANNEL_AGGREGATED_MEMBERS_GET
                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Command/MultiChannelAggregatedMembersGet
```

**Payload:**
```json
{
  "properties1": {
    "aggregated_end_point": "0x05"
  }
}
```
### MULTI_CHANNEL_AGGREGATED_MEMBERS_REPORT
                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannel/Report/MultiChannelAggregatedMembersReport
```

**Payload:**
```json
{
  "properties1": {
    "aggregated_end_point": "0x05"
  },
  "number_of_bit_masks": "0x12"
}
```
