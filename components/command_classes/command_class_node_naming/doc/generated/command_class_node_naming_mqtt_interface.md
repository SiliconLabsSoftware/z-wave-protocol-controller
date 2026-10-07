## COMMAND_CLASS_NODE_NAMING MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | true | false |

### Table of Contents
- [NODE_NAMING_NODE_LOCATION_REPORT](#node_naming_node_location_report)
- [NODE_NAMING_NODE_LOCATION_SET](#node_naming_node_location_set)
- [NODE_NAMING_NODE_LOCATION_GET](#node_naming_node_location_get)
- [NODE_NAMING_NODE_NAME_GET](#node_naming_node_name_get)
- [NODE_NAMING_NODE_NAME_REPORT](#node_naming_node_name_report)
- [NODE_NAMING_NODE_NAME_SET](#node_naming_node_name_set)
### NODE_NAMING_NODE_LOCATION_REPORT
                                                                                                      
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/NodeNaming/Report/NodeNamingNodeLocationReport
```

**Payload:**
```json
{
  "level": {
    "char__presentation": "0x05"
  }
}
```
### NODE_NAMING_NODE_LOCATION_SET
                                                                                                      
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/NodeNaming/Command/NodeNamingNodeLocationSet
```

**Payload:**
```json
{
  "level": {
    "char__presentation": "0x05"
  }
}
```
### NODE_NAMING_NODE_LOCATION_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/NodeNaming/Command/NodeNamingNodeLocationGet
```

**Payload:**
```json
{ }
```
### NODE_NAMING_NODE_NAME_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/NodeNaming/Command/NodeNamingNodeNameGet
```

**Payload:**
```json
{ }
```
### NODE_NAMING_NODE_NAME_REPORT
                                                                                                      
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/NodeNaming/Report/NodeNamingNodeNameReport
```

**Payload:**
```json
{
  "level": {
    "char__presentation": "0x05"
  }
}
```
### NODE_NAMING_NODE_NAME_SET
                                                                                                      
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/NodeNaming/Command/NodeNamingNodeNameSet
```

**Payload:**
```json
{
  "level": {
    "char__presentation": "0x05"
  }
}
```
