## COMMAND_CLASS_MANUFACTURER_SPECIFIC MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | true | true |

### Table of Contents
- [MANUFACTURER_SPECIFIC_GET](#manufacturer_specific_get)
- [MANUFACTURER_SPECIFIC_REPORT](#manufacturer_specific_report)
- [DEVICE_SPECIFIC_GET](#device_specific_get)
- [DEVICE_SPECIFIC_REPORT](#device_specific_report)
### MANUFACTURER_SPECIFIC_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ManufacturerSpecific/Command/ManufacturerSpecificGet
```

**Payload:**
```json
{ }
```
### MANUFACTURER_SPECIFIC_REPORT
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ManufacturerSpecific/Report/ManufacturerSpecificReport
```

**Payload:**
```json
{
  "manufacturer_id": "0x2574",
  "product_type_id": "0x2574",
  "product_id": "0x2574"
}
```
### DEVICE_SPECIFIC_GET
                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ManufacturerSpecific/Command/DeviceSpecificGet
```

**Payload:**
```json
{
  "properties1": {
    "device_id_type": "0x05"
  }
}
```
### DEVICE_SPECIFIC_REPORT
                                                                                                                                                                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/ManufacturerSpecific/Report/DeviceSpecificReport
```

**Payload:**
```json
{
  "properties1": {
    "device_id_type": "0x05"
  },
  "properties2": {
    "device_id_data_length_indicator": "0x05",
    "device_id_data_format": "0x05"
  },
  "device_id_data": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
