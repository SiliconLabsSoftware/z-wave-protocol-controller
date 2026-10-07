## COMMAND_CLASS_SENSOR_BINARY MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [SENSOR_BINARY_GET](#sensor_binary_get)
- [SENSOR_BINARY_REPORT](#sensor_binary_report)
- [SENSOR_BINARY_SUPPORTED_GET_SENSOR](#sensor_binary_supported_get_sensor)
- [SENSOR_BINARY_SUPPORTED_SENSOR_REPORT](#sensor_binary_supported_sensor_report)
### SENSOR_BINARY_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SensorBinary/Command/SensorBinaryGet
```

**Payload:**
```json
{
  "sensor_type": "0x12"
}
```
### SENSOR_BINARY_REPORT
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SensorBinary/Report/SensorBinaryReport
```

**Payload:**
```json
{
  "sensor_value": "0x01",
  "sensor_type": "0x12"
}
```
### SENSOR_BINARY_SUPPORTED_GET_SENSOR
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SensorBinary/Command/SensorBinarySupportedGetSensor
```

**Payload:**
```json
{ }
```
### SENSOR_BINARY_SUPPORTED_SENSOR_REPORT
            
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/SensorBinary/Report/SensorBinarySupportedSensorReport
```

**Payload:**
```json
{ }
```
