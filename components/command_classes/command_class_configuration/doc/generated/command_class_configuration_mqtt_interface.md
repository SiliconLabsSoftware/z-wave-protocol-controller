## COMMAND_CLASS_CONFIGURATION MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [CONFIGURATION_BULK_GET](#configuration_bulk_get)
- [CONFIGURATION_BULK_REPORT](#configuration_bulk_report)
- [CONFIGURATION_BULK_SET](#configuration_bulk_set)
- [CONFIGURATION_GET](#configuration_get)
- [CONFIGURATION_REPORT](#configuration_report)
- [CONFIGURATION_SET](#configuration_set)
- [CONFIGURATION_NAME_GET](#configuration_name_get)
- [CONFIGURATION_NAME_REPORT](#configuration_name_report)
- [CONFIGURATION_INFO_GET](#configuration_info_get)
- [CONFIGURATION_INFO_REPORT](#configuration_info_report)
- [CONFIGURATION_PROPERTIES_GET](#configuration_properties_get)
- [CONFIGURATION_PROPERTIES_REPORT](#configuration_properties_report)
- [CONFIGURATION_DEFAULT_RESET](#configuration_default_reset)
### CONFIGURATION_BULK_GET
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Configuration/Command/ConfigurationBulkGet
```

**Payload:**
```json
{
  "parameter_offset": "0x2574",
  "number_of_parameters": "0x12"
}
```
### CONFIGURATION_BULK_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Configuration/Report/ConfigurationBulkReport
```

**Payload:**
```json
{
  "parameter_offset": "0x2574",
  "number_of_parameters": "0x12",
  "reports_to_follow": "0x12",
  "properties1": {
    "size": "0x05",
    "handshake": "0x05",
    "default": "0x05"
  },
  "vg": [
    {
      "parameter": [
        "0x01",
        "0x02",
        "0x03"
      ]
    }
  ]
}
```
### CONFIGURATION_BULK_SET
                                                                                                                                                                                                                                                                                                                                                                                      
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Configuration/Command/ConfigurationBulkSet
```

**Payload:**
```json
{
  "parameter_offset": "0x2574",
  "number_of_parameters": "0x12",
  "properties1": {
    "size": "0x05",
    "handshake": "0x05",
    "default": "0x05"
  },
  "vg": [
    {
      "parameter": [
        "0x01",
        "0x02",
        "0x03"
      ]
    }
  ]
}
```
### CONFIGURATION_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Configuration/Command/ConfigurationGet
```

**Payload:**
```json
{
  "parameter_number": "0x12"
}
```
### CONFIGURATION_REPORT
                                                                                                                                                                        
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Configuration/Report/ConfigurationReport
```

**Payload:**
```json
{
  "parameter_number": "0x12",
  "level": {
    "size": "0x05"
  },
  "configuration_value": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### CONFIGURATION_SET
                                                                                                                                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Configuration/Command/ConfigurationSet
```

**Payload:**
```json
{
  "parameter_number": "0x12",
  "level": {
    "size": "0x05",
    "default": "0x05"
  },
  "configuration_value": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### CONFIGURATION_NAME_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Configuration/Command/ConfigurationNameGet
```

**Payload:**
```json
{
  "parameter_number": "0x2574"
}
```
### CONFIGURATION_NAME_REPORT
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Configuration/Report/ConfigurationNameReport
```

**Payload:**
```json
{
  "parameter_number": "0x2574",
  "reports_to_follow": "0x12",
  "name": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### CONFIGURATION_INFO_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Configuration/Command/ConfigurationInfoGet
```

**Payload:**
```json
{
  "parameter_number": "0x2574"
}
```
### CONFIGURATION_INFO_REPORT
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Configuration/Report/ConfigurationInfoReport
```

**Payload:**
```json
{
  "parameter_number": "0x2574",
  "reports_to_follow": "0x12",
  "info": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### CONFIGURATION_PROPERTIES_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Configuration/Command/ConfigurationPropertiesGet
```

**Payload:**
```json
{
  "parameter_number": "0x2574"
}
```
### CONFIGURATION_PROPERTIES_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Configuration/Report/ConfigurationPropertiesReport
```

**Payload:**
```json
{
  "parameter_number": "0x2574",
  "properties1": {
    "size": "0x05",
    "format": "0x05",
    "readonly": "0x05",
    "altering_capabilities": "0x05"
  },
  "min_value": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "max_value": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "default_value": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "next_parameter_number": "0x2574",
  "properties2": {
    "advanced": "0x05",
    "no_bulk_support": "0x05",
    "reserved1": "0x05"
  }
}
```
### CONFIGURATION_DEFAULT_RESET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Configuration/Command/ConfigurationDefaultReset
```

**Payload:**
```json
{ }
```
