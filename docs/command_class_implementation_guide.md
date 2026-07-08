# Command Class Implementation Guide

This guide provides comprehensive instructions for implementing Z-Wave command classes in the Z-Wave Protocol Controller (ZPC) project. It covers the complete process from configuration to implementation, using `COMMAND_CLASS_SWITCH_MULTILEVEL` as a practical example.

## Table of Contents

1. [Overview](#overview)
2. [Prerequisites](#prerequisites)
3. [Step-by-Step Implementation Tutorial](#step-by-step-implementation-tutorial)
4. [Understanding the Generated Structure](#understanding-the-generated-structure)
5. [Implementation Details](#implementation-details)
6. [Best Practices](#best-practices)
7. [Troubleshooting](#troubleshooting)
8. [Complete Example: COMMAND_CLASS_SWITCH_MULTILEVEL](#complete-example-command_class_switch_multilevel)
9. [Summary](#summary)

---

## Overview

Command classes are implemented using a code generation approach. The process involves:

1. **Configuration**: Adding the command class to be implemented to the generation configuration file
2. **Generation**: Running a Python script during CMake configuration that generates command class skeleton files
3. **Implementation**: Implementing the command class logic in the generated skeleton files

The generator creates a standardized structure that handles:
- Z-Wave frame validation, parsing and assembly
- Permanent storage interface
- MQTT interface
- MQTT interface documentation
- Command class specific types
- Command class core logic
- Build system integration

---

## Prerequisites

Before implementing a command class, ensure you have:

1. **Python Dependencies**: Install required Python packages
```bash
pip install -r scripts/command_class_generator/requirements.txt
```

2. **clang-format**: The generator uses `clang-format` to format generated C++ code. Ensure it's installed and available in your PATH.

3. **Z-Wave XML Schema**: The generator uses `zwave.xml` and `zwave.xsd` files located in `scripts/command_class_generator/` to validate and parse Z-Wave command class definitions.

---

## Step-by-Step Implementation Tutorial

### Step 1: Add Command Class to Configuration

Edit the file `scripts/command_class_generator/config.yaml` and add your command class entry to the `supported_command_classes` list.

```yaml
supported_command_classes:
  - name: "COMMAND_CLASS_SWITCH_MULTILEVEL" # Command class name (must match Z-Wave spec)
    version: 4 # Supported version
    mqtt_support: true # Enable MQTT interface and documentation
    support: false # Whether this CC is supported (ZPC acts as a supporting device)
    control: true # Whether this CC is controlled (ZPC controls devices on the network)
    interview_attributes: # Attributes to query during interview
      - "Current Value"
    minimal_scheme: ZWAVE_CONTROLLER_ENCAPSULATION_NONE # Security scheme
```

For Multilevel Switch, `support: false` and `control: true` means ZPC controls multilevel switch devices on the network but does not expose this command class as a supporting device.

---

### Step 2: Generate Command Class Files

The generation process is triggered automatically during CMake configuration (not during the build). When you run `cmake --preset <preset>`, the generator runs via `execute_process` in `components/command_classes/CMakeLists.txt` and produces the command class files before compilation starts.

The generator creates some files that should not be manually edited and some files called "skeleton" that should be implemented.

The generator creates the following directory structure:

```
command_class_switch_multilevel/
├── CMakeLists.txt
├── doc/ # Command class documentation (you can add files here, e.g. sequence diagrams)
│   ├── command_class_switch_multilevel.md
│   └── generated/ # Auto-generated MQTT interface documentation (do not edit)
│       └── command_class_switch_multilevel_mqtt_interface.md
├── generated/ # Command class core logic (do not edit)
│   ├── inc/
│   │   └── command_class_switch_multilevel_core.hpp
│   └── src/
│       └── command_class_switch_multilevel_core.cpp
├── inc/ # Command class skeleton header files (to be implemented)
│   ├── command_class_switch_multilevel.hpp
│   ├── command_class_switch_multilevel_attribute_store.hpp
│   └── command_class_switch_multilevel_mqtt.hpp
├── interface/ # Interface library
│   ├── CMakeLists.txt
│   ├── generated/
│   │   └── inc/
│   │       └── command_class_switch_multilevel_generated_types.hpp 
│   └── inc/
│       ├── command_class_switch_multilevel_constants.hpp
│       ├── command_class_switch_multilevel_events.hpp
│       └── command_class_switch_multilevel_types.hpp
└── src/ # Command class skeleton source files (to be implemented)
    ├── command_class_switch_multilevel.cpp
    ├── command_class_switch_multilevel_attribute_store.cpp
    └── command_class_switch_multilevel_mqtt.cpp
```

**Important**: Files under `generated/` and `doc/generated/` are automatically generated and should **never** be manually edited. Any changes will be overwritten on the next generation. The `doc/` directory itself is for your own documentation (sequence diagrams, design notes, and similar).

---

### Step 3: Implement Command Class

After the generation, the following sources can be modified

**Command Class Implementation**
Everything related to the specific command class' implementation should be done in these sources.
- **`src/command_class_switch_multilevel.cpp`**: in case any additional logic should be implemented for a command class.
- **`src/command_class_switch_multilevel_attribute_store.cpp`**: in case any value from a received frame should be stored in the permanent storage for later use inside ZPC. See the note below on when this is optional.
- **`src/command_class_switch_multilevel_mqtt.cpp`**: MQTT interface should be implemented in this source for a specific command class.

**Interface**
In case a command class has attributes to share with other components, e.g. data types or constant values, or public events, those should be defined here.
- **`interface/inc/command_class_switch_multilevel_constants.hpp`**: all of the constant values goes here for a specific command class.
- **`interface/inc/command_class_switch_multilevel_events.hpp`**: in case a command class has events to subscribe to by other components, those events should be defined here.
- **`interface/inc/command_class_switch_multilevel_types.hpp`**: command class specific data types.

---

## Understanding the Generated Structure

### Source Files to Implement

The generator creates three main source files that you need to implement. Each file serves a specific purpose in the command class implementation:

#### 1. `command_class_switch_multilevel.cpp`

**Purpose**: This is the main command class implementation file containing the core logic for the command class.

**Typical Contents**:
- **Constructor**: Initializes the command class and its base classes (attribute store and MQTT components). May contain initialization logic, event subscriptions, or permanent store callbacks.
- **`on_interview()`**: Called during device interview to perform command class-specific interview operations, such as querying interview attributes or setting up initial state.
- **Parsed Methods** (`on_*_parsed`): Called after a Z-Wave frame has been successfully parsed, stored (if implemented), and published to MQTT. Use them for custom logic like validation, logging, or triggering other events.
- **Frame Assembly Methods** (`on_*_requested_assemble_frame`): Optional methods to customize frame assembly when the default generated behavior is insufficient. For GET and FIND commands with no parameters, the default implementation automatically generates the frame.

**Example**:
```cpp
command_class_switch_multilevel::command_class_switch_multilevel()
    : command_class_switch_multilevel_attribute_store(), command_class_switch_multilevel_mqtt() {}

void command_class_switch_multilevel::on_interview(
    attribute_store::attribute endpoint_node,
    uint8_t supported_version)
{
    auto get_group_node = endpoint_node.emplace_node(
        static_cast<attribute_store_type_t>(
            switch_multilevel_get_group_attributes_t::SWITCH_MULTILEVEL_GET_GROUP
        )
    );
    start_group_resolution(get_group_node, interview_resolution_options());

    if (supported_version >= 3) {
        auto supported_get_group_node = endpoint_node.emplace_node(
            static_cast<attribute_store_type_t>(
                switch_multilevel_supported_get_group_attributes_t::SWITCH_MULTILEVEL_SUPPORTED_GET_GROUP
            )
        );
        start_group_resolution(supported_get_group_node, interview_resolution_options());
    }
}

sl_status_t command_class_switch_multilevel::on_switch_multilevel_set_requested_assemble_frame(
    const set_requested_args &args,
    uint8_t *data,
    uint16_t *length)
{
    auto group_node       = args.node;
    auto &frame_generator = args.set_frame_generator;

    auto value_node = group_node.emplace_node(
        static_cast<attribute_store_type_t>(switch_multilevel_set_group_attributes_t::value)
    );
    if (!value_node.desired_exists()) {
        return SL_STATUS_NOT_READY;
    }
    frame_generator->add_value(value_node, DESIRED_ATTRIBUTE);

    auto duration_node = group_node.emplace_node(
        static_cast<attribute_store_type_t>(switch_multilevel_set_group_attributes_t::duration)
    );
    if (!duration_node.desired_exists()) {
        return SL_STATUS_NOT_READY;
    }
    frame_generator->add_value(duration_node, DESIRED_ATTRIBUTE);

    return frame_generator->generate_frame();
}
```

#### 2. `command_class_switch_multilevel_attribute_store.cpp`

**Purpose**: This file handles the integration with the permanent store, responsible for persisting received command data.

> **When is storage needed?** The generated core publishes MQTT reports directly from the parsed frame (`attribute_map`), not from the attribute store. After a report is received, the flow is: parse frame → `on_*_received_store` → `mqtt_publish_report(attribute_map)` → `on_*_parsed`. If your application only consumes reported values through MQTT, you do not need to implement storage — the default generated `on_*_received_store` methods return `SL_STATUS_OK` without persisting anything, and MQTT reporting still works.
>
> Implement storage only when ZPC needs the data later inside the application: another component reads from the attribute store, interview or resolver logic depends on persisted state, or outbound frame assembly needs previously reported values.
>
> **Examples where storage is omitted:** `command_class_supervision`, `command_class_powerlevel`, and `command_class_device_reset_locally` leave their `*_attribute_store.cpp` empty and rely on the generated default.
>
> **Wake Up command class example:** `WAKE_UP_INTERVAL_REPORT` stores the reported interval (`seconds`) in the attribute store (`command_class_wake_up_attribute_store::on_wake_up_interval_report_received_store`). This stored value is later queried through component connector (`COMMAND_CLASS_WAKE_UP_INTERVAL_REQUESTED`) in `command_class_wake_up::on_wake_up_interval_requested`, and `network_monitor` uses it to set/restart NL-node offline timers. This is a good example of when storage is required because ZPC-internal logic depends on the value after the report is received.

**Typical Contents**:
- **Constructor**: Typically empty, but can contain attribute store-specific initialization.
- **Store Methods** (`on_*_received_store`): These methods extract values from the parsed attribute map and store them in the appropriate permanent store nodes. This is where data persistence happens — values received from Z-Wave devices are stored for later retrieval by ZPC-internal logic.

**Example**:
```cpp
command_class_switch_multilevel_attribute_store::command_class_switch_multilevel_attribute_store() {}

sl_status_t command_class_switch_multilevel_attribute_store::on_switch_multilevel_report_received_store(
    attribute_store::attribute endpoint_node,
    command_class_switch_multilevel_attribute_map_t attribute_map)
{
    auto group_node = endpoint_node.emplace_node(
        static_cast<attribute_store_type_t>(
            switch_multilevel_report_group_attributes_t::SWITCH_MULTILEVEL_REPORT_GROUP
        )
    );

    switch_multilevel_report_current_value_t current_value = 0;
    current_value = get_value_or_default(attribute_map, "current_value", current_value);
    auto current_value_node = group_node.emplace_node(
        static_cast<attribute_store_type_t>(switch_multilevel_report_group_attributes_t::current_value)
    );
    current_value_node.set_reported<switch_multilevel_report_current_value_t>(current_value);

    switch_multilevel_report_target_value_t target_value = 0;
    target_value = get_value_or_default(attribute_map, "target_value", target_value);
    auto target_value_node = group_node.emplace_node(
        static_cast<attribute_store_type_t>(switch_multilevel_report_group_attributes_t::target_value)
    );
    target_value_node.set_reported<switch_multilevel_report_target_value_t>(target_value);

    switch_multilevel_report_duration_t duration = 0;
    duration = get_value_or_default(attribute_map, "duration", duration);
    auto duration_node = group_node.emplace_node(
        static_cast<attribute_store_type_t>(switch_multilevel_report_group_attributes_t::duration)
    );
    duration_node.set_reported<switch_multilevel_report_duration_t>(duration);

    return SL_STATUS_OK;
}
```

#### 3. `command_class_switch_multilevel_mqtt.cpp`

**Purpose**: This file implements the MQTT interface, handling MQTT commands received from clients and publishing MQTT reports.

**Typical Contents**:
- **Constructor**: Registers MQTT command handlers by inserting command name to callback mappings into `mqtt_callback_map` and calling `mqtt_register_command_handler()`.
- **MQTT Command Handlers** (`mqtt_on_*_command`): These methods handle MQTT commands received from clients. They parse the MQTT payload (typically JSON), validate parameters, and trigger Z-Wave frame transmission by manipulating the attribute store and starting group resolution.

**Example**:
```cpp
command_class_switch_multilevel_mqtt::command_class_switch_multilevel_mqtt()
{
    mqtt_callback_map.insert({"SwitchMultilevelGet", [this](attribute_store::attribute &endpoint_node, std::string payload) {
                                  this->mqtt_on_switch_multilevel_get_command(endpoint_node, payload);
                              }});
    mqtt_callback_map.insert({"SwitchMultilevelSet", [this](attribute_store::attribute &endpoint_node, std::string payload) {
                                  this->mqtt_on_switch_multilevel_set_command(endpoint_node, payload);
                              }});
    mqtt_callback_map.insert({"SwitchMultilevelStartLevelChange", [this](attribute_store::attribute &endpoint_node, std::string payload) {
                                  this->mqtt_on_switch_multilevel_start_level_change_command(endpoint_node, payload);
                              }});
    mqtt_callback_map.insert({"SwitchMultilevelStopLevelChange", [this](attribute_store::attribute &endpoint_node, std::string payload) {
                                  this->mqtt_on_switch_multilevel_stop_level_change_command(endpoint_node, payload);
                              }});
    mqtt_callback_map.insert({"SwitchMultilevelSupportedGet", [this](attribute_store::attribute &endpoint_node, std::string payload) {
                                  this->mqtt_on_switch_multilevel_supported_get_command(endpoint_node, payload);
                              }});

    mqtt_register_command_handler();
}

sl_status_t command_class_switch_multilevel_mqtt::mqtt_on_switch_multilevel_get_command(
    attribute_store::attribute &endpoint_node,
    std::string payload)
{
    auto group_node = endpoint_node.emplace_node(
        static_cast<attribute_store_type_t>(
            switch_multilevel_get_group_attributes_t::SWITCH_MULTILEVEL_GET_GROUP
        )
    );
    command_class_switch_multilevel_core::start_group_resolution(group_node);

    return SL_STATUS_OK;
}

sl_status_t command_class_switch_multilevel_mqtt::mqtt_on_switch_multilevel_set_command(
    attribute_store::attribute &endpoint_node,
    std::string payload)
{
    uint8_t value    = 0;
    uint8_t duration = 0;

    mqtt_payload_parser parser {payload, LOG_TAG.data()};
    parser.parse("value", value).parse("duration", duration);
    if (parser.status() != SL_STATUS_OK) {
        return parser.status();
    }

    auto group_node = endpoint_node.emplace_node(
        static_cast<attribute_store_type_t>(
            switch_multilevel_set_group_attributes_t::SWITCH_MULTILEVEL_SET_GROUP
        )
    );

    auto value_node = group_node.emplace_node(
        static_cast<attribute_store_type_t>(switch_multilevel_set_group_attributes_t::value)
    );
    value_node.set_desired(value);

    auto duration_node = group_node.emplace_node(
        static_cast<attribute_store_type_t>(switch_multilevel_set_group_attributes_t::duration)
    );
    duration_node.set_desired(duration);

    command_class_switch_multilevel_core::start_group_resolution(group_node);

    return SL_STATUS_OK;
}
```

---

## Implementation Details

### Working with Attribute Maps

The generator creates a type-safe attribute map (`command_class_switch_multilevel_attribute_map_t`) that contains parsed frame data. Use the `get_value_or_default` helper method to extract values:

```cpp
// Get a current value with a default
switch_multilevel_report_current_value_t current_value = 0;
current_value = get_value_or_default(attribute_map, "current_value", current_value);

// The method is templated and works with different types
switch_multilevel_report_target_value_t target_value = 0;
target_value = get_value_or_default(attribute_map, "target_value", target_value);
```

### Attribute Store Integration

The attribute store is separate from the MQTT reporting path. MQTT reports are assembled from the parsed `attribute_map` at receive time; storing values in the attribute store is only needed when other ZPC logic must read them later (see the **When is storage needed?** note in the attribute store section above).

The attribute store uses a hierarchical structure. Each command class typically has:

1. **Command Group Nodes**: Parent nodes for command-specific groups (for example GET, SET, REPORT, START_LEVEL_CHANGE)
2. **Attribute Nodes**: Child nodes under those groups that store command parameters or reported values

Example structure:
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

### MQTT Interface

When `mqtt_support: true` is set in the configuration:

1. **Commands**: MQTT commands are published to:
   ```
   zpc/<home_id>/<node_id>/<endpoint_id>/SwitchMultilevel/Command/SwitchMultilevelGet
   zpc/<home_id>/<node_id>/<endpoint_id>/SwitchMultilevel/Command/SwitchMultilevelSet
   zpc/<home_id>/<node_id>/<endpoint_id>/SwitchMultilevel/Command/SwitchMultilevelStartLevelChange
   zpc/<home_id>/<node_id>/<endpoint_id>/SwitchMultilevel/Command/SwitchMultilevelStopLevelChange
   zpc/<home_id>/<node_id>/<endpoint_id>/SwitchMultilevel/Command/SwitchMultilevelSupportedGet
   ```

2. **Reports**: MQTT reports are published to:
   ```
   zpc/<home_id>/<node_id>/<endpoint_id>/SwitchMultilevel/Report/SwitchMultilevelReport
   zpc/<home_id>/<node_id>/<endpoint_id>/SwitchMultilevel/Report/SwitchMultilevelSupportedReport
   ```

3. **Command Registration**: Commands are automatically registered in the constructor of `command_class_switch_multilevel_mqtt`:

```cpp
command_class_switch_multilevel_mqtt::command_class_switch_multilevel_mqtt()
{
    mqtt_callback_map.insert({"SwitchMultilevelGet", [this](attribute_store::attribute &endpoint_node, std::string payload) {
                                  this->mqtt_on_switch_multilevel_get_command(endpoint_node, payload);
                              }});
    mqtt_callback_map.insert({"SwitchMultilevelSet", [this](attribute_store::attribute &endpoint_node, std::string payload) {
                                  this->mqtt_on_switch_multilevel_set_command(endpoint_node, payload);
                              }});
    mqtt_register_command_handler();
}
```

### Frame Assembly and Parsing

The `command_class_switch_multilevel_core` class (generated) handles:

- **Frame Parsing**: Automatically parses incoming Z-Wave frames based on the XML definition
- **Frame Assembly**: Uses resolver rules that call `on_*_requested_assemble_frame` for GET/SET/FIND group nodes
  - **Commands with no parameters**: The generated default implementation uses `generate_no_args_frame()`, so no custom code is needed
  - **Commands with parameters (GET/FIND/SET)**: Implement `on_*_requested_assemble_frame` and add values with `frame_generator->add_value(..., DESIRED_ATTRIBUTE)`
- **Version Handling**: Parsing and interview behavior are version-aware (generated parsing honors per-parameter `min_version`, and interview uses endpoint-reported CC version when available)

Generated parsing and frame-generator helpers reduce low-level byte handling, but for commands with parameters (especially multi-field or variable layouts) you usually implement `on_*_requested_assemble_frame` and add fields with `add_value` (and sometimes `add_raw_byte` for custom encoding).

### Triggering Frame Transmission

To trigger a Z-Wave command transmission, you need to:

1. **Create or get the command group node, then set required desired values**: Use `emplace_node()` for the group and any parameter nodes, then set desired values for parameters when needed (applies to GET/FIND/SET)
2. **Start group resolution**: Call `command_class_switch_multilevel_core::start_group_resolution()` on the group node

**For GET commands** (querying a value):
```cpp
auto group_node = endpoint_node.emplace_node(
    static_cast<attribute_store_type_t>(
        switch_multilevel_get_group_attributes_t::SWITCH_MULTILEVEL_GET_GROUP
    )
);
// Start group resolution to trigger query
command_class_switch_multilevel_core::start_group_resolution(group_node);
```

**Note**: For simple GET and FIND commands with no parameters (like `SwitchMultilevelGet`), the frame assembly is automatically handled by the default implementation. You only need to implement `on_*_requested_assemble_frame` if the GET or FIND command has parameters that need to be included in the frame (see examples below).

**For commands with parameters** (example with `SwitchMultilevelSet`):

In the MQTT command handler:
```cpp
uint8_t value    = 0;
uint8_t duration = 0;

mqtt_payload_parser parser {payload, LOG_TAG.data()};
parser.parse("value", value).parse("duration", duration);
if (parser.status() != SL_STATUS_OK) {
    return parser.status();
}

// Ensure the command group node exists
auto group_node = endpoint_node.emplace_node(
    static_cast<attribute_store_type_t>(
        switch_multilevel_set_group_attributes_t::SWITCH_MULTILEVEL_SET_GROUP
    )
);
// Create the value and duration nodes and set desired values
auto value_node = group_node.emplace_node(
    static_cast<attribute_store_type_t>(
        switch_multilevel_set_group_attributes_t::value
    )
);
value_node.set_desired(value);

auto duration_node = group_node.emplace_node(
    static_cast<attribute_store_type_t>(
        switch_multilevel_set_group_attributes_t::duration
    )
);
duration_node.set_desired(duration);

// Start group resolution to trigger transmission
command_class_switch_multilevel_core::start_group_resolution(group_node);
```

Implement `on_*_requested_assemble_frame` in `command_class_switch_multilevel.cpp` when parameters must be added to the frame:

**For SET commands** (when default behavior is insufficient):
```cpp
sl_status_t command_class_switch_multilevel::on_switch_multilevel_set_requested_assemble_frame(
    const set_requested_args &args,
    uint8_t *data,
    uint16_t *length)
{
    auto group_node       = args.node;
    auto &frame_generator = args.set_frame_generator;

    auto value_node = group_node.emplace_node(
        static_cast<attribute_store_type_t>(switch_multilevel_set_group_attributes_t::value)
    );
    if (!value_node.desired_exists()) {
        return SL_STATUS_NOT_READY;
    }
    frame_generator->add_value(value_node, DESIRED_ATTRIBUTE);

    auto duration_node = group_node.emplace_node(
        static_cast<attribute_store_type_t>(switch_multilevel_set_group_attributes_t::duration)
    );
    if (!duration_node.desired_exists()) {
        return SL_STATUS_NOT_READY;
    }
    frame_generator->add_value(duration_node, DESIRED_ATTRIBUTE);

    return frame_generator->generate_frame();
}
```

**Tip**: GET/FIND/SET assembly logic can share the same pattern when they use similar parameters. You can reuse the same attribute extraction approach (`emplace_node(...)` + `add_value(..., DESIRED_ATTRIBUTE)`) across command types, while keeping separate group types and frame layouts per command.

**Note**: For GET and FIND commands with no parameters, you don't need to implement this method - the default implementation automatically generates a no-args frame.

The `start_group_resolution()` method will automatically:
- Call `on_*_requested_assemble_frame` (default or override) to assemble the Z-Wave frame from attribute store desired values
- Send the frame via the attribute resolver
- Handle retries and timeouts

---

## Best Practices

### 1. Type Safety

Use the generated types instead of raw integers:

```cpp
// Good
switch_multilevel_report_current_value_t current_value = get_value_or_default(...);

// Bad
uint8_t current_value = get_value_or_default(...);
```
---

## Troubleshooting

### Generation Fails

**Problem**: Generator script fails with errors.

**Solutions**:
- Verify Python dependencies are installed: `pip install -r scripts/command_class_generator/requirements.txt`
- Check that `zwave.xml` contains your command class definition
- Verify YAML syntax in `config.yaml`
- Ensure `clang-format` is installed and in PATH

### MQTT Commands Not Working

**Problem**: MQTT commands are not received or processed.

**Solutions**:
- Verify `mqtt_support: true` in config.yaml
- Check that MQTT callbacks are registered in constructor
- Verify MQTT topic names match the expected format
- Check MQTT broker connection and topic subscriptions
- Check if home and node ids are properly set in the MQTT topic

---

## Complete Example: COMMAND_CLASS_SWITCH_MULTILEVEL

This chapter provides a complete, working example of implementing `COMMAND_CLASS_SWITCH_MULTILEVEL` from start to finish. It demonstrates all the key components: configuration setup, interview logic with version handling, attribute store integration for data persistence, frame assembly customization, and MQTT command handling.

Instead of duplicating source code in this document, use the canonical implementation files under `components/command_classes/command_class_switch_multilevel/` as the reference.

### Configuration (`config.yaml`)

```yaml
- name: "COMMAND_CLASS_SWITCH_MULTILEVEL"
  version: 4
  mqtt_support: true
  support: false
  control: true
  interview_attributes:
    - "Current Value"
  minimal_scheme: ZWAVE_CONTROLLER_ENCAPSULATION_NONE
```

After modifying the configuration file, re-run CMake configuration (e.g. `cmake --preset macos`) to regenerate the multilevel switch command class skeleton.

### Main Implementation (`src/command_class_switch_multilevel.cpp`)

Canonical source (relative path):
- `components/command_classes/command_class_switch_multilevel/src/command_class_switch_multilevel.cpp`

What to look at:
- Constructor wiring (`attribute_store` + `mqtt` base classes)
- `on_interview(...)` for interview GET flows
- `on_switch_multilevel_set_requested_assemble_frame(...)` and
  `on_switch_multilevel_start_level_change_requested_assemble_frame(...)`
  for frame assembly from desired attributes

### Attribute Store Implementation (`src/command_class_switch_multilevel_attribute_store.cpp`)

Canonical source (relative path):
- `components/command_classes/command_class_switch_multilevel/src/command_class_switch_multilevel_attribute_store.cpp`

What to look at:
- `on_switch_multilevel_report_received_store(...)`
- `on_switch_multilevel_supported_report_received_store(...)`
- Attribute group + child node creation and `set_reported(...)` usage

### MQTT Implementation (`src/command_class_switch_multilevel_mqtt.cpp`)

Canonical source (relative path):
- `components/command_classes/command_class_switch_multilevel/src/command_class_switch_multilevel_mqtt.cpp`

What to look at:
- MQTT handler registration in constructor
- `mqtt_on_switch_multilevel_*` handlers
- Payload parsing and desired-value updates before `start_group_resolution(...)`

---

## Summary

Implementing a command class involves:

1. Adding configuration to `config.yaml`
2. Running the generator (automatic during CMake configuration, or manual)
3. Implementing parsed methods (optional custom logic)
4. Implementing store methods (only when ZPC-internal logic needs persisted data; optional for MQTT-only integrations)
5. Implementing MQTT methods (required if MQTT support is enabled)
6. Implementing interview method (required for device interview)
7. Implementing frame assembly methods (only needed for GET commands with parameters or when customizing SET command assembly)

The generator handles the complex parts (frame parsing, assembly, routing), allowing you to focus on your specific business logic. 

**Important Notes**:
- **GET and FIND commands with no parameters** (like `SwitchMultilevelGet`): Frame assembly is automatically handled - no implementation needed
- **GET and FIND commands with parameters** (like `AssociationGet` with `grouping_identifier`, or `MultiChannelEndPointFind` with device classes): You must implement `on_*_requested_assemble_frame` to add parameters to the frame
- **SET commands**: Frame assembly is automatically handled from attribute store values, but can be customized if needed (as shown for `SwitchMultilevelSet` and `SwitchMultilevelStartLevelChange`)

Follow the patterns shown in `COMMAND_CLASS_SWITCH_MULTILEVEL` for a working implementation.

For questions or issues, refer to existing command class implementations in `components/command_classes/` or consult the Z-Wave specification for command class details.

