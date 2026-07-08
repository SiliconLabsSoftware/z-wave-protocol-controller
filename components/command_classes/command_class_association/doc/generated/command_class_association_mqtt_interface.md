## COMMAND_CLASS_ASSOCIATION MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | true | true |

### Table of Contents
- [ASSOCIATION_GET](#association_get)
- [ASSOCIATION_GROUPINGS_GET](#association_groupings_get)
- [ASSOCIATION_GROUPINGS_REPORT](#association_groupings_report)
- [ASSOCIATION_REMOVE](#association_remove)
- [ASSOCIATION_REPORT](#association_report)
- [ASSOCIATION_SET](#association_set)
- [ASSOCIATION_SPECIFIC_GROUP_GET](#association_specific_group_get)
- [ASSOCIATION_SPECIFIC_GROUP_REPORT](#association_specific_group_report)
### ASSOCIATION_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Command/AssociationGet
```

**Payload:**
```json
{
  "grouping_identifier": "0x12"
}
```
### ASSOCIATION_GROUPINGS_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Command/AssociationGroupingsGet
```

**Payload:**
```json
{ }
```
### ASSOCIATION_GROUPINGS_REPORT
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Report/AssociationGroupingsReport
```

**Payload:**
```json
{
  "supported_groupings": "0x12"
}
```
### ASSOCIATION_REMOVE
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Command/AssociationRemove
```

**Payload:**
```json
{
  "grouping_identifier": "0x12",
  "node_id": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### ASSOCIATION_REPORT
                                                                                                                                                      
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Report/AssociationReport
```

**Payload:**
```json
{
  "grouping_identifier": "0x12",
  "max_nodes_supported": "0x12",
  "reports_to_follow": "0x12",
  "nodeid": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### ASSOCIATION_SET
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Command/AssociationSet
```

**Payload:**
```json
{
  "grouping_identifier": "0x12",
  "node_id": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### ASSOCIATION_SPECIFIC_GROUP_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Command/AssociationSpecificGroupGet
```

**Payload:**
```json
{ }
```
### ASSOCIATION_SPECIFIC_GROUP_REPORT
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Association/Report/AssociationSpecificGroupReport
```

**Payload:**
```json
{
  "group": "0x12"
}
```
