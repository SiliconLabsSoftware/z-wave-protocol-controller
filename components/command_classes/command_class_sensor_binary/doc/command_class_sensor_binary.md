# Binary Sensor Command Class

Binary Sensor CC (0x30, version 2, DEPRECATED) — ZPC controls binary sensor devices
(motion, door/window, smoke, etc.) on the Z-Wave network. ZPC does not act as a sensor.

The spec recommends Notification CC (v3+) for new implementations, but Binary Sensor is
still present in deployed devices and must be controlled.

## Interview Flow — Version 2

Version 2 adds a supported-sensor-type discovery step before querying individual types.

```mermaid
sequenceDiagram
    participant Interview
    participant SensorBinary
    participant Resolver
    participant Device
    participant AttributeStore

    Interview->>SensorBinary: on_interview(v2)
    SensorBinary->>AttributeStore: emplace SENSOR_BINARY_SUPPORTED_SENSOR_REPORT_GROUP
    SensorBinary->>Resolver: start_group_resolution "SUPPORTED_GET_SENSOR"
    Resolver->>Device: SENSOR_BINARY_SUPPORTED_GET_SENSOR
    Device->>SensorBinary: SENSOR_BINARY_SUPPORTED_SENSOR_REPORT (bit_mask)
    SensorBinary->>AttributeStore: store bit_mask
    Note over SensorBinary: For each supported sensor type N
    SensorBinary->>AttributeStore: emplace SENSOR_BINARY_REPORT_GROUP(N) require sensor_value
    SensorBinary->>AttributeStore: set GET_GROUP sensor_type desired = first type
    SensorBinary->>Resolver: start_group_resolution "GET_GROUP"
    Resolver->>SensorBinary: on_sensor_binary_get_requested_assemble_frame
    SensorBinary->>Device: SENSOR_BINARY_GET (sensor_type=N)
    Device->>SensorBinary: SENSOR_BINARY_REPORT (sensor_value, sensor_type=N)
    SensorBinary->>AttributeStore: store sensor_value in REPORT_GROUP(N)
    Note over SensorBinary: If more types remain, update desired and restart GET
    SensorBinary->>Resolver: start_group_resolution "GET_GROUP" with next sensor_type
    Note over SensorBinary: Repeat until all types queried
    SensorBinary->>Interview: cc_interview_finish_if_complete
```

## Interview Flow — Version 1

Version 1 has no sensor type; a single GET is sent and the response is stored under
sensor_type = 0x00 (sentinel).

```mermaid
sequenceDiagram
    participant Interview
    participant SensorBinary
    participant Resolver
    participant Device
    participant AttributeStore

    Interview->>SensorBinary: on_interview(v1)
    SensorBinary->>AttributeStore: emplace SENSOR_BINARY_REPORT_GROUP(0x00) require sensor_value
    SensorBinary->>Resolver: start_group_resolution "GET_GROUP" (no sensor_type)
    Resolver->>SensorBinary: on_sensor_binary_get_requested_assemble_frame
    SensorBinary->>Device: SENSOR_BINARY_GET (no params)
    Device->>SensorBinary: SENSOR_BINARY_REPORT (sensor_value)
    SensorBinary->>AttributeStore: store sensor_value in REPORT_GROUP(0x00)
    SensorBinary->>Interview: cc_interview_finish_if_complete
```

## MQTT Command Flow — SensorBinaryGet

```mermaid
sequenceDiagram
    participant Client
    participant SensorBinary
    participant Resolver
    participant Device

    Client->>SensorBinary: SensorBinaryGet "{"sensor_type": 12}"
    SensorBinary->>SensorBinary: set GET_GROUP sensor_type desired = 12
    SensorBinary->>Resolver: start_group_resolution
    Resolver->>Device: SENSOR_BINARY_GET (sensor_type=12)
    Device->>SensorBinary: SENSOR_BINARY_REPORT (sensor_value, sensor_type=12)
    SensorBinary->>Client: SensorBinaryReport "{"sensor_value":255,"sensor_type":12}"
```
