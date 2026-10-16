## COMMAND_CLASS_METER MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [METER_GET](#meter_get)
- [METER_REPORT](#meter_report)
- [METER_RESET](#meter_reset)
- [METER_SUPPORTED_GET](#meter_supported_get)
- [METER_SUPPORTED_REPORT](#meter_supported_report)
### METER_GET
                                                                                                                                                                            
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Meter/Command/MeterGet
```

**Payload:**
```json
{
  "properties1": {
    "scale": "0x05",
    "rate_type": "0x05"
  },
  "scale_2": "0x12"
}
```
### METER_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Meter/Report/MeterReport
```

**Payload:**
```json
{
  "properties1": {
    "meter_type": "0x05",
    "rate_type": "0x05",
    "scale_bit_2": "0x05"
  },
  "properties2": {
    "size": "0x05",
    "scale_bits_10": "0x05",
    "precision": "0x05"
  },
  "meter_value": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "delta_time": "0x2574",
  "previous_meter_value": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "scale_2": "0x12"
}
```
### METER_RESET
                                                                                                                                                                                                                                                                                                                                                                                      
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Meter/Command/MeterReset
```

**Payload:**
```json
{
  "properties1": {
    "meter_type": "0x05",
    "rate_type": "0x05",
    "scale_bit_2": "0x05"
  },
  "properties2": {
    "size": "0x05",
    "scale_bits_10": "0x05",
    "precision": "0x05"
  },
  "meter_value": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "scale_2": "0x12"
}
```
### METER_SUPPORTED_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Meter/Command/MeterSupportedGet
```

**Payload:**
```json
{ }
```
### METER_SUPPORTED_REPORT
                                                                                                                                                                                                                                                                                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Meter/Report/MeterSupportedReport
```

**Payload:**
```json
{
  "properties1": {
    "meter_type": "0x05",
    "rate_type": "0x05",
    "meter_reset": "0x05"
  },
  "properties2": {
    "scale_supported_0": "0x05",
    "m.s.t": "0x05"
  },
  "number_of_scale_supported_bytes_to_follow": "0x12",
  "scale_supported": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
