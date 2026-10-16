## COMMAND_CLASS_SENSOR_MULTILEVEL MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [SENSOR_MULTILEVEL_GET](#sensor_multilevel_get)
- [SENSOR_MULTILEVEL_REPORT](#sensor_multilevel_report)
- [SENSOR_MULTILEVEL_SUPPORTED_GET_SENSOR](#sensor_multilevel_supported_get_sensor)
- [SENSOR_MULTILEVEL_SUPPORTED_SENSOR_REPORT](#sensor_multilevel_supported_sensor_report)
- [SENSOR_MULTILEVEL_SUPPORTED_GET_SCALE](#sensor_multilevel_supported_get_scale)
- [SENSOR_MULTILEVEL_SUPPORTED_SCALE_REPORT](#sensor_multilevel_supported_scale_report)
### SENSOR_MULTILEVEL_GET
                                                                                                                                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SensorMultilevel/Command/SensorMultilevelGet
```

**Payload:**
```json
{
  "sensor_type": "0x01",
  "properties1": {
    "reserved1": "0x05",
    "scale": "0x05",
    "reserved2": "0x05"
  }
}
```
### SENSOR_MULTILEVEL_REPORT
                                                                                                                                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SensorMultilevel/Report/SensorMultilevelReport
```

**Payload:**
```json
{
  "sensor_type": "0x01",
  "level": {
    "size": "0x05",
    "scale": "0x05",
    "precision": "0x05"
  },
  "sensor_value": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### SENSOR_MULTILEVEL_SUPPORTED_GET_SENSOR
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SensorMultilevel/Command/SensorMultilevelSupportedGetSensor
```

**Payload:**
```json
{ }
```
### SENSOR_MULTILEVEL_SUPPORTED_SENSOR_REPORT
            
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SensorMultilevel/Report/SensorMultilevelSupportedSensorReport
```

**Payload:**
```json
{ }
```
### SENSOR_MULTILEVEL_SUPPORTED_GET_SCALE
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SensorMultilevel/Command/SensorMultilevelSupportedGetScale
```

**Payload:**
```json
{
  "sensor_type": "0x01"
}
```
### SENSOR_MULTILEVEL_SUPPORTED_SCALE_REPORT
                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SensorMultilevel/Report/SensorMultilevelSupportedScaleReport
```

**Payload:**
```json
{
  "sensor_type": "0x01",
  "properties1": {
    "scale_bit_mask": "0x05"
  }
}
```
