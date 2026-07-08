# MQTT API Interface Documentation

This page is the **developer reference** for the `MqttApiBase` class and topic naming rules. For the **catalog of live MQTT topics** exposed by ZPC (Discovery, Network Management, SmartStart, Device Interview, Network Status, OTA, Command Classes), see the [MQTT API Index](mqtt_api_index.md) — it is the single source of truth for what topics exist, in which direction they flow, and where to find their full reference.

## Table of Contents
- [MqttApiBase API Reference](#mqttapibase-api-reference)
- [Topic Naming Conventions](#topic-naming-conventions)
- [See also](#see-also)

## MqttApiBase API Reference

The `MqttApiBase` class provides protected methods for derived classes. These methods handle MQTT topic subscriptions, message publishing, and base topic management.

### `subscribe_topic()`

Subscribe to an MQTT topic. Base topic is automatically prepended unless `add_base_topic=false`.

**Signatures:**
```cpp
void subscribe_topic(const std::string &topic, 
                     const std::function<void(const std::string &, const std::string &)> &callback, 
                     bool add_base_topic = true);

void subscribe_topic(std::string_view topic, 
                     const std::function<void(const std::string &, const std::string &)> &callback, 
                     bool add_base_topic = true);
```

**Parameters:**
- `topic`: The MQTT topic to subscribe to (relative path if `add_base_topic=true`, full path if `add_base_topic=false`)
- `callback`: The callback function to invoke when a message is received. Signature: `void(const std::string &topic, const std::string &message)`
- `add_base_topic`: Whether to add the base topic to the topic (default: `true`)

**Usage:**
```cpp
// Subscribe to a topic with base topic prefix (default)
subscribe_topic("Network/Node/Add", [this](const std::string &topic, const std::string &message) {
    this->on_network_node_add(topic, message);
});
// Results in subscription to: zpc/{home_id}/Network/Node/Add

// Subscribe to a global topic without base topic prefix
subscribe_topic("zpc/Discovery", [this](const std::string &topic, const std::string &message) {
    this->on_discovery(topic, message);
}, false);
// Results in subscription to: zpc/Discovery
```

**Note:** Use `add_base_topic=false` for global topics like `zpc/Discovery`.

### `publish_report()`

Publish a message to an MQTT topic. Base topic is automatically prepended unless `add_base_topic=false`.

**Signatures:**
```cpp
void publish_report(std::string_view topic, 
                    const std::string &payload, 
                    bool retain = false, 
                    bool add_base_topic = true);
```

**Parameters:**
- `topic`: The MQTT topic to publish to (relative path if `add_base_topic=true`, full path if `add_base_topic=false`)
- `payload`: The message payload (typically JSON string)
- `retain`: Whether the message should be retained (default: `false`)
- `add_base_topic`: Whether to add the base topic to the topic (default: `true`)

**Usage:**
```cpp
// Publish to a topic with base topic prefix (default)
publish_report("Network/Node/Add/Report", json_payload, false);
// Publishes to: zpc/{home_id}/Network/Node/Add/Report

// Publish to a global topic without base topic prefix
publish_report("zpc/Discovery/Report", json_payload, false, false);
// Publishes to: zpc/Discovery/Report
```

### `get_base_topic()`

Get the current base topic string (e.g., `"zpc/ABCD1234"`).

**Signature:**
```cpp
std::string get_base_topic();
```

**Returns:**
- The base topic string in the format `"zpc/{home_id}"` where `{home_id}` is the 8-character hexadecimal representation of the Z-Wave home ID

**Throws:**
- `std::runtime_error` if home_id is not set

**Usage:**
```cpp
std::string base = get_base_topic();
// Example result: "zpc/ABCD1234"

// Use for custom topic construction
std::string custom_topic = base + "/Custom/Topic";
```

**Note:** The base topic is retrieved from the current Z-Wave network's home ID at runtime, ensuring topics always reflect the active network.

## Topic Naming Conventions

The MQTT API uses two topic patterns depending on whether the topic is network-specific or system-wide.

### Base Topic Pattern

**Format:** `zpc/{home_id}/...`

- Most topics include the Z-Wave home ID
- Automatically prefixed by `MqttApiBase` methods (default behavior)
- Example: `zpc/ABCD1234/Network/Node/Add`

**Usage:** Use relative topic paths in your API class (e.g., `"Network/Node/Add"`), and the base topic will be automatically prepended.

**Example:**
```cpp
// In your API class
subscribe_topic("Network/Node/Add", callback);
// Automatically subscribes to: zpc/{home_id}/Network/Node/Add
```

### Global Topics

**Format:** `zpc/...` (no home_id)

- Used for discovery and other system-level operations
- Must use `subscribe_topic()` or `publish_report()` with `add_base_topic=false`
- Example: `zpc/Discovery`

**Usage:** Pass the full topic path and set `add_base_topic=false` to prevent base topic prefixing.

**Example:**
```cpp
// For global topics
subscribe_topic("zpc/Discovery", callback, false);
// Subscribes to: zpc/Discovery (no home_id prefix)
```

### Topic Structure Guidelines

1. **Commands**: Use the pattern `{feature}/{action}` (e.g., `Network/Node/Add`)
2. **Reports**: Append `/Report` to the command topic (e.g., `Network/Node/Add/Report`)
3. **Node-specific topics**: Include node ID and endpoint (e.g., `{node_id}/ep{endpoint}/SwitchBinary/Command/SwitchBinarySet`)
4. **Global topics**: Use `zpc/` prefix directly (e.g., `zpc/Discovery`)

The base topic is retrieved from the current Z-Wave network's home ID at runtime, ensuring topics always reflect the active network.

## See also

- [MQTT API Index](mqtt_api_index.md) — Catalog of all topics and their full references (Discovery, Network Management, SmartStart, Device Interview, Network Status, OTA, Command Classes).
- [MQTT API Overview](mqtt_api_overview.md) — Architecture of the MQTT API component and how each specialized API class is initialized.
