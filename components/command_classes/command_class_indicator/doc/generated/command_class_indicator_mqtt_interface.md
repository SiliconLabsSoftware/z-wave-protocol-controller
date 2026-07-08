## COMMAND_CLASS_INDICATOR MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | true | true |

### Table of Contents
- [INDICATOR_GET](#indicator_get)
- [INDICATOR_REPORT](#indicator_report)
- [INDICATOR_SET](#indicator_set)
- [INDICATOR_SUPPORTED_GET](#indicator_supported_get)
- [INDICATOR_SUPPORTED_REPORT](#indicator_supported_report)
- [INDICATOR_DESCRIPTION_GET](#indicator_description_get)
- [INDICATOR_DESCRIPTION_REPORT](#indicator_description_report)
### INDICATOR_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Indicator/Command/IndicatorGet
```

**Payload:**
```json
{
  "indicator_id": "0x01"
}
```
### INDICATOR_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Indicator/Report/IndicatorReport
```

**Payload:**
```json
{
  "indicator_0_value": "0x12",
  "properties1": {
    "indicator_object_count": "0x05"
  },
  "vg1": [
    {
      "indicator_id": "0x01",
      "property_id": "0x01",
      "value": "0x12"
    }
  ]
}
```
### INDICATOR_SET
                                                                                                                                                                                                                                                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Indicator/Command/IndicatorSet
```

**Payload:**
```json
{
  "indicator_0_value": "0x12",
  "properties1": {
    "indicator_object_count": "0x05"
  },
  "vg1": [
    {
      "indicator_id": "0x01",
      "property_id": "0x01",
      "value": "0x12"
    }
  ]
}
```
### INDICATOR_SUPPORTED_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Indicator/Command/IndicatorSupportedGet
```

**Payload:**
```json
{
  "indicator_id": "0x01"
}
```
### INDICATOR_SUPPORTED_REPORT
                                                                                                                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Indicator/Report/IndicatorSupportedReport
```

**Payload:**
```json
{
  "indicator_id": "0x01",
  "next_indicator_id": "0x01",
  "properties1": {
    "property_supported_bit_mask_length": "0x05"
  }
}
```
### INDICATOR_DESCRIPTION_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Indicator/Command/IndicatorDescriptionGet
```

**Payload:**
```json
{
  "indicator_id": "0x12"
}
```
### INDICATOR_DESCRIPTION_REPORT
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Indicator/Report/IndicatorDescriptionReport
```

**Payload:**
```json
{
  "indicator_id": "0x12",
  "description_length": "0x12",
  "description": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
