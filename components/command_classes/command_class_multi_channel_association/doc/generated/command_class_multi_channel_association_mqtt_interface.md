## COMMAND_CLASS_MULTI_CHANNEL_ASSOCIATION MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | true | true |

### Table of Contents
- [MULTI_CHANNEL_ASSOCIATION_GET](#multi_channel_association_get)
- [MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET](#multi_channel_association_groupings_get)
- [MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT](#multi_channel_association_groupings_report)
- [MULTI_CHANNEL_ASSOCIATION_REMOVE](#multi_channel_association_remove)
- [MULTI_CHANNEL_ASSOCIATION_REPORT](#multi_channel_association_report)
- [MULTI_CHANNEL_ASSOCIATION_SET](#multi_channel_association_set)
### MULTI_CHANNEL_ASSOCIATION_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannelAssociation/Command/MultiChannelAssociationGet
```

**Payload:**
```json
{
  "grouping_identifier": "0x12"
}
```
### MULTI_CHANNEL_ASSOCIATION_GROUPINGS_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannelAssociation/Command/MultiChannelAssociationGroupingsGet
```

**Payload:**
```json
{ }
```
### MULTI_CHANNEL_ASSOCIATION_GROUPINGS_REPORT
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannelAssociation/Report/MultiChannelAssociationGroupingsReport
```

**Payload:**
```json
{
  "supported_groupings": "0x12"
}
```
### MULTI_CHANNEL_ASSOCIATION_REMOVE
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannelAssociation/Command/MultiChannelAssociationRemove
```

**Payload:**
```json
{
  "grouping_identifier": "0x12",
  "node_id": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "marker": "0xFF",
  "vg": [
    {
      "multi_channel_node_id": "0x12",
      "properties1": {
        "end_point": "0x05",
        "bit_address": "0x05"
      }
    }
  ]
}
```
### MULTI_CHANNEL_ASSOCIATION_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannelAssociation/Report/MultiChannelAssociationReport
```

**Payload:**
```json
{
  "grouping_identifier": "0x12",
  "max_nodes_supported": "0x12",
  "reports_to_follow": "0x12",
  "node_id": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "marker": "0xFF",
  "vg": [
    {
      "multi_channel_node_id": "0x12",
      "properties1": {
        "end_point": "0x05",
        "bit_address": "0x05"
      }
    }
  ]
}
```
### MULTI_CHANNEL_ASSOCIATION_SET
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/MultiChannelAssociation/Command/MultiChannelAssociationSet
```

**Payload:**
```json
{
  "grouping_identifier": "0x12",
  "node_id": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "marker": "0xFF",
  "vg": [
    {
      "multi_channel_node_id": "0x12",
      "properties1": {
        "end_point": "0x05",
        "bit_address": "0x05"
      }
    }
  ]
}
```
