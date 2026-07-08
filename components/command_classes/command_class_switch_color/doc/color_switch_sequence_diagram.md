# Color Switch Command Class - Sequence Diagrams

This document describes the communication flows for the Color Switch command class (version 3).

## Interview Flow - Supported Colors Query

```mermaid
sequenceDiagram
    participant Interview
    participant ColorSwitch
    participant Resolver
    participant Device
    participant AttributeStore

    Interview->>ColorSwitch: on_interview(endpoint)
    ColorSwitch->>AttributeStore: emplace SWITCH_COLOR_SUPPORTED_GET_GROUP
    ColorSwitch->>Resolver: start_group_resolution
    Resolver->>ColorSwitch: Get rule callback
    ColorSwitch->>Device: SWITCH_COLOR_SUPPORTED_GET (no args)
    Device->>ColorSwitch: SWITCH_COLOR_SUPPORTED_REPORT (color_component_mask)
    ColorSwitch->>AttributeStore: on_switch_color_supported_report_received_store
    ColorSwitch->>AttributeStore: Store color_component_mask
    ColorSwitch->>Resolver: stop_group_resolution
```

## Get Color Value Flow

```mermaid
sequenceDiagram
    participant MQTT
    participant ColorSwitch
    participant Resolver
    participant Device
    participant AttributeStore

    MQTT->>ColorSwitch: SwitchColorGet(color_component_id)
    ColorSwitch->>AttributeStore: Set color_component_id desired
    ColorSwitch->>Resolver: start_group_resolution
    Resolver->>ColorSwitch: on_switch_color_get_requested_assemble_frame
    ColorSwitch->>Device: SWITCH_COLOR_GET (color_component_id)
    Device->>ColorSwitch: SWITCH_COLOR_REPORT (color_component_id, current_value, target_value, duration)
    ColorSwitch->>AttributeStore: on_switch_color_report_received_store
    ColorSwitch->>MQTT: Publish SwitchColorReport
```

## Set Color Flow

```mermaid
sequenceDiagram
    participant MQTT
    participant ColorSwitch
    participant Resolver
    participant Device
    participant AttributeStore

    MQTT->>ColorSwitch: SwitchColorSet(vg1, duration)
    ColorSwitch->>AttributeStore: Set color_component_count, vg1, duration desired
    ColorSwitch->>Resolver: start_group_resolution
    Resolver->>ColorSwitch: on_switch_color_set_requested_assemble_frame
    ColorSwitch->>Device: SWITCH_COLOR_SET (properties1, vg1, duration)
    Device->>ColorSwitch: SWITCH_COLOR_REPORT (optional)
```

## Start Level Change Flow

```mermaid
sequenceDiagram
    participant MQTT
    participant ColorSwitch
    participant Resolver
    participant Device

    MQTT->>ColorSwitch: SwitchColorStartLevelChange(properties1, color_component_id, start_level, duration)
    ColorSwitch->>Resolver: start_group_resolution
    Resolver->>ColorSwitch: Set rule (assemble Properties1, color_component_id, start_level, duration)
    ColorSwitch->>Device: SWITCH_COLOR_START_LEVEL_CHANGE
```

## Stop Level Change Flow

```mermaid
sequenceDiagram
    participant MQTT
    participant ColorSwitch
    participant Resolver
    participant Device

    MQTT->>ColorSwitch: SwitchColorStopLevelChange(color_component_id)
    ColorSwitch->>Resolver: start_group_resolution
    Resolver->>ColorSwitch: Set rule (assemble color_component_id)
    ColorSwitch->>Device: SWITCH_COLOR_STOP_LEVEL_CHANGE
```

## Color Component IDs (from Z-Wave specification)

| ID | Label                    | Value Range |
|----|--------------------------|-------------|
| 0  | Warm White               | 0x00..0xFF  |
| 1  | Cold White               | 0x00..0xFF  |
| 2  | Red                      | 0x00..0xFF  |
| 3  | Green                    | 0x00..0xFF  |
| 4  | Blue                     | 0x00..0xFF  |
| 5  | Amber (6ch color mixing) | 0x00..0xFF  |
| 6  | Cyan (6ch color mixing)  | 0x00..0xFF  |
| 7  | Purple (6ch color mixing)| 0x00..0xFF  |
| 8  | Indexed Color [OBSOLETED]| 0x00..0xFF  |

## Dependencies

Nodes supporting Color Switch Command Class must support one of:
- Multilevel Switch Command Class, version 4
- Binary Switch Command Class, version 2

Color component levels are scaled by the brightness level set via Multilevel Switch or Basic Set.
