# Multilevel Switch Command Class - Sequence Diagrams

This document describes the communication flows for the Multilevel Switch command class (version 4).

## Interview Flow

```mermaid
sequenceDiagram
    participant Interview
    participant MultilevelSwitch
    participant Resolver
    participant Device
    participant AttributeStore

    Interview->>MultilevelSwitch: on_interview(endpoint, supported_version)
    MultilevelSwitch->>AttributeStore: emplace SWITCH_MULTILEVEL_GET_GROUP
    MultilevelSwitch->>Resolver: start_group_resolution
    Resolver->>MultilevelSwitch: Get rule callback
    MultilevelSwitch->>Device: SWITCH_MULTILEVEL_GET (no args)
    Device->>MultilevelSwitch: SWITCH_MULTILEVEL_REPORT (current_value, target_value, duration)
    MultilevelSwitch->>AttributeStore: on_switch_multilevel_report_received_store
    MultilevelSwitch->>Resolver: stop_group_resolution

    alt supported_version >= 3
        MultilevelSwitch->>AttributeStore: emplace SWITCH_MULTILEVEL_SUPPORTED_GET_GROUP
        MultilevelSwitch->>Resolver: start_group_resolution
        Resolver->>MultilevelSwitch: Get rule callback
        MultilevelSwitch->>Device: SWITCH_MULTILEVEL_SUPPORTED_GET (no args)
        Device->>MultilevelSwitch: SWITCH_MULTILEVEL_SUPPORTED_REPORT (primary_switch_type, secondary_switch_type)
        MultilevelSwitch->>AttributeStore: on_switch_multilevel_supported_report_received_store
        MultilevelSwitch->>Resolver: stop_group_resolution
    end
```

## Get Current Value Flow

```mermaid
sequenceDiagram
    participant MQTT
    participant MultilevelSwitch
    participant Resolver
    participant Device
    participant AttributeStore

    MQTT->>MultilevelSwitch: SwitchMultilevelGet
    MultilevelSwitch->>AttributeStore: emplace SWITCH_MULTILEVEL_GET_GROUP
    MultilevelSwitch->>Resolver: start_group_resolution
    Resolver->>MultilevelSwitch: Get rule callback (no-args frame)
    MultilevelSwitch->>Device: SWITCH_MULTILEVEL_GET
    Device->>MultilevelSwitch: SWITCH_MULTILEVEL_REPORT (current_value, target_value, duration)
    MultilevelSwitch->>AttributeStore: Store current_value, target_value, duration
    MultilevelSwitch->>MQTT: Publish SwitchMultilevelReport
    MultilevelSwitch->>Resolver: stop_group_resolution
```

## Set Value Flow

```mermaid
sequenceDiagram
    participant MQTT
    participant MultilevelSwitch
    participant Resolver
    participant Device
    participant AttributeStore

    MQTT->>MultilevelSwitch: SwitchMultilevelSet(value, duration)
    MultilevelSwitch->>AttributeStore: Set value, duration desired
    MultilevelSwitch->>Resolver: start_group_resolution
    Resolver->>MultilevelSwitch: on_switch_multilevel_set_requested_assemble_frame
    Note over MultilevelSwitch: v1: Value only<br/>v2+: Value + Duration
    MultilevelSwitch->>Device: SWITCH_MULTILEVEL_SET (value, duration)
```

## Start Level Change Flow

```mermaid
sequenceDiagram
    participant MQTT
    participant MultilevelSwitch
    participant Resolver
    participant Device
    participant AttributeStore

    MQTT->>MultilevelSwitch: SwitchMultilevelStartLevelChange(up_down, ignore_start_level, start_level, dimming_duration, inc_dec, step_size)
    MultilevelSwitch->>AttributeStore: Set up_down, ignore_start_level, start_level, dimming_duration desired
    opt v3+ parameters
        MultilevelSwitch->>AttributeStore: Set inc_dec, step_size desired
    end
    MultilevelSwitch->>Resolver: start_group_resolution
    Resolver->>MultilevelSwitch: on_switch_multilevel_start_level_change_requested_assemble_frame
    Note over MultilevelSwitch: Assembles Properties1 byte<br/>(Up/Down, Ignore Start Level, Inc/Dec)
    MultilevelSwitch->>Device: SWITCH_MULTILEVEL_START_LEVEL_CHANGE
```

## Stop Level Change Flow

```mermaid
sequenceDiagram
    participant MQTT
    participant MultilevelSwitch
    participant Resolver
    participant Device

    MQTT->>MultilevelSwitch: SwitchMultilevelStopLevelChange
    MultilevelSwitch->>Resolver: start_group_resolution
    Resolver->>MultilevelSwitch: Set rule (no-args frame)
    MultilevelSwitch->>Device: SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE
```

## Supported Get/Report Flow

```mermaid
sequenceDiagram
    participant MQTT
    participant MultilevelSwitch
    participant Resolver
    participant Device
    participant AttributeStore

    MQTT->>MultilevelSwitch: SwitchMultilevelSupportedGet
    MultilevelSwitch->>AttributeStore: emplace SWITCH_MULTILEVEL_SUPPORTED_GET_GROUP
    MultilevelSwitch->>Resolver: start_group_resolution
    Resolver->>MultilevelSwitch: Get rule callback (no-args frame)
    MultilevelSwitch->>Device: SWITCH_MULTILEVEL_SUPPORTED_GET
    Device->>MultilevelSwitch: SWITCH_MULTILEVEL_SUPPORTED_REPORT (primary_switch_type, secondary_switch_type)
    MultilevelSwitch->>AttributeStore: Store primary_switch_type, secondary_switch_type
    MultilevelSwitch->>MQTT: Publish SwitchMultilevelSupportedReport
    MultilevelSwitch->>Resolver: stop_group_resolution
```

## Switch Types (from Z-Wave specification)

| Value | Type             | Direction A (0x00)  | Direction B (0x63/0xFF) |
|-------|------------------|---------------------|-------------------------|
| 0x00  | Undefined        | -                   | -                       |
| 0x01  | On/Off           | Off                 | On                      |
| 0x02  | Up/Down          | Down                | Up                      |
| 0x03  | Open/Close       | Close               | Open                    |
| 0x04  | CCW/CW           | Counter-Clockwise   | Clockwise               |
| 0x05  | Left/Right       | Left                | Right                   |
| 0x06  | Reverse/Forward  | Reverse             | Forward                 |
| 0x07  | Pull/Push        | Pull                | Push                    |

## Attribute Store Tree

```
Endpoint Node
├── SWITCH_MULTILEVEL_GET_GROUP
├── SWITCH_MULTILEVEL_SET_GROUP
│   ├── value
│   └── duration
├── SWITCH_MULTILEVEL_REPORT_GROUP
│   ├── current_value
│   ├── target_value
│   └── duration
├── SWITCH_MULTILEVEL_START_LEVEL_CHANGE_GROUP
│   ├── up_down
│   ├── ignore_start_level
│   ├── inc_dec
│   ├── start_level
│   ├── dimming_duration
│   └── step_size
├── SWITCH_MULTILEVEL_STOP_LEVEL_CHANGE_GROUP
├── SWITCH_MULTILEVEL_SUPPORTED_GET_GROUP
└── SWITCH_MULTILEVEL_SUPPORTED_REPORT_GROUP
    ├── primary_switch_type
    └── secondary_switch_type
```
