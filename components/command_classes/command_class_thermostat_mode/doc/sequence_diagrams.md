# Thermostat Mode CC — Sequence Diagrams

## Interview Flow

```mermaid
sequenceDiagram
    participant ZPC
    participant Node

    ZPC->>Node: THERMOSTAT_MODE_SUPPORTED_GET
    Node-->>ZPC: THERMOSTAT_MODE_SUPPORTED_REPORT (bit_mask)

    ZPC->>Node: THERMOSTAT_MODE_GET
    Node-->>ZPC: THERMOSTAT_MODE_REPORT (mode)
    Note over ZPC: THERMOSTAT_MODE_CHANGED only if mode changed<br/>(per spec: Get requires only Report in response)
```

## Set Mode Flow (via MQTT)

```mermaid
sequenceDiagram
    participant MQTT Client
    participant ZPC
    participant Node

    MQTT Client->>ZPC: ThermostatModeSet { "mode": "0x01" }
    Note over ZPC: Sets desired mode in attribute store
    ZPC->>Node: THERMOSTAT_MODE_SET (mode)
    Node-->>ZPC: (Supervision ACK / implicit)
    ZPC->>Node: THERMOSTAT_MODE_GET
    Node-->>ZPC: THERMOSTAT_MODE_REPORT (mode)
    ZPC->>MQTT Client: ThermostatModeReport { "mode": "0x01" }
    Note over ZPC: Fires THERMOSTAT_MODE_CHANGED (mode changed)<br/>→ Setpoint CC may refresh setpoints for new mode
```

## Get Mode Flow (via MQTT)

Per Z-Wave spec, Thermostat Mode Get requires only Thermostat Mode Report in response. No setpoint or capability chain.

```mermaid
sequenceDiagram
    participant MQTT Client
    participant ZPC
    participant Node

    MQTT Client->>ZPC: ThermostatModeGet { }
    Note over ZPC: Clears reported state, starts resolution
    ZPC->>Node: THERMOSTAT_MODE_GET
    Node-->>ZPC: THERMOSTAT_MODE_REPORT (mode)
    ZPC->>MQTT Client: ThermostatModeReport { "mode": "0x01" }
    Note over ZPC: No THERMOSTAT_MODE_CHANGED if mode unchanged<br/>(no setpoint/capability follow-up)
```

## Inter-CC Communication

```mermaid
sequenceDiagram
    participant ThermostatMode CC
    participant component_connector
    participant ThermostatSetpoint CC

    Note over ThermostatMode CC: Only when reported mode actually changed
    ThermostatMode CC->>component_connector: fire_event(THERMOSTAT_MODE_CHANGED, {endpoint_node, mode})
    component_connector->>ThermostatSetpoint CC: on_thermostat_mode_changed callback
    Note over ThermostatSetpoint CC: Triggers THERMOSTAT_SETPOINT_GET<br/>to refresh setpoints for new mode
```
