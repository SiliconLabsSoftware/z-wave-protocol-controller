## COMMAND_CLASS_ASSOCIATION_GRP_INFO MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | true | true |

### Table of Contents
- [ASSOCIATION_GROUP_NAME_GET](#association_group_name_get)
- [ASSOCIATION_GROUP_NAME_REPORT](#association_group_name_report)
- [ASSOCIATION_GROUP_INFO_GET](#association_group_info_get)
- [ASSOCIATION_GROUP_INFO_REPORT](#association_group_info_report)
- [ASSOCIATION_GROUP_COMMAND_LIST_GET](#association_group_command_list_get)
- [ASSOCIATION_GROUP_COMMAND_LIST_REPORT](#association_group_command_list_report)
### ASSOCIATION_GROUP_NAME_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/AssociationGrpInfo/Command/AssociationGroupNameGet
```

**Payload:**
```json
{
  "grouping_identifier": "0x12"
}
```
### ASSOCIATION_GROUP_NAME_REPORT
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/AssociationGrpInfo/Report/AssociationGroupNameReport
```

**Payload:**
```json
{
  "grouping_identifier": "0x12",
  "length_of_name": "0x12",
  "name": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### ASSOCIATION_GROUP_INFO_GET
                                                                                                                                                                            
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/AssociationGrpInfo/Command/AssociationGroupInfoGet
```

**Payload:**
```json
{
  "properties1": {
    "list_mode": "0x05",
    "refresh_cache": "0x05"
  },
  "grouping_identifier": "0x12"
}
```
### ASSOCIATION_GROUP_INFO_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/AssociationGrpInfo/Report/AssociationGroupInfoReport
```

**Payload:**
```json
{
  "properties1": {
    "group_count": "0x05",
    "dynamic_info": "0x05",
    "list_mode": "0x05"
  },
  "vg1": [
    {
      "grouping_identifier": "0x12",
      "mode": "0x12",
      "profile1": "0x01",
      "profile2": "0x4B",
      "event_code": "0x2574"
    }
  ]
}
```
### ASSOCIATION_GROUP_COMMAND_LIST_GET
                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/AssociationGrpInfo/Command/AssociationGroupCommandListGet
```

**Payload:**
```json
{
  "properties1": {
    "allow_cache": "0x05"
  },
  "grouping_identifier": "0x12"
}
```
### ASSOCIATION_GROUP_COMMAND_LIST_REPORT
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/AssociationGrpInfo/Report/AssociationGroupCommandListReport
```

**Payload:**
```json
{
  "grouping_identifier": "0x12",
  "list_length": "0x12",
  "command": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
