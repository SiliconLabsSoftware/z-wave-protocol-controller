# MQTT API Index

This page is the central index for all ZPC MQTT APIs. Use it to find topics, payloads, and related sequence documentation.

## Table of Contents

- [Discovery](#discovery)
- [Network Management](#network-management)
- [SmartStart](#smartstart)
- [Device Interview](#device-interview)
- [Network Status](#network-status)
- [OTA (Firmware Update)](#ota-firmware-update)
- [Command Classes](#command-classes)
- [Sequences](#sequences)

## Discovery

Obtain the Z-Wave Home ID of the controller (global topic, not scoped by home ID).

| Topic | Direction | Description |
|-------|-----------|-------------|
| `zpc/Discovery` | Command | Request Home ID (publish any payload, e.g. `{}`) |
| `zpc/Discovery/Report` | Report | Response with `home_id` |

**Full reference:** [Discovery MQTT API](discovery_mqtt_api.md)

## Network Management

Add/remove nodes, accept DSK, grant keys, list devices, factory reset, and Network Layer Security (NLS) operations.

| Topic pattern | Description |
|---------------|-------------|
| `zpc/<home_id>/Network/Node/Add` | Start inclusion |
| `zpc/<home_id>/Network/Node/Add/Report` | Node added report (`status` success/fail; fail reason `6404` = security fail) |
| `zpc/<home_id>/Network/Node/Add/Abort` | Abort ongoing inclusion |
| `zpc/<home_id>/Network/Node/Remove` | Start exclusion |
| `zpc/<home_id>/Network/Node/Remove/Report` | Node removed report (also fired on successful Remove Failed) |
| `zpc/<home_id>/Network/Node/Remove/Abort` | Abort ongoing exclusion |
| `zpc/<home_id>/Network/Node/RemoveFailed` | Remove a node believed to be failed (controller NOP-probes first) |
| `zpc/<home_id>/Network/Node/RemoveFailed/Report` | Remove Failed result (`ok` / `fail` with `reason`) |
| `zpc/<home_id>/Network/DSK/Accept` | Accept DSK during S2 inclusion |
| `zpc/<home_id>/Network/RequestedKeys/Report` | Keys requested (published by ZPC) |
| `zpc/<home_id>/Network/GrantKeys` | Grant keys during S2 inclusion |
| `zpc/<home_id>/Network/RequestedDSK/Report` | DSK verification needed (published by ZPC) |
| `zpc/<home_id>/Network/Node/List` | Request node list |
| `zpc/<home_id>/Network/Node/List/Report` | Node list report |
| `zpc/<home_id>/Network/Node/Properties` | Request node properties |
| `zpc/<home_id>/Network/Node/Properties/Report` | Node properties report |
| `zpc/<home_id>/Network/Node/Interview` | Request a (re-)interview / capability discovery for a node |
| `zpc/<home_id>/Network/FactoryReset` | Factory reset controller |
| `zpc/<home_id>/Network/NLS/Enable` | Enable NLS for a node |
| `zpc/<home_id>/Network/NLS/Enable/Report` | NLS enable result |
| `zpc/<home_id>/Network/NLS/State` | Request NLS state |
| `zpc/<home_id>/Network/NLS/State/Report` | NLS state report |

**Full reference:** [Network Management MQTT API](../../network_manager/doc/network_management_mqtt_api.md)

## SmartStart

Manage the SmartStart provisioning list (replace, append, remove, purge) and read it back. Pre-provisioned devices in the list are included automatically when they power up in range.

| Topic pattern | Direction | Description |
|---------------|-----------|-------------|
| `zpc/<home_id>/Network/SmartStart/Update` | Command | Replace SmartStart provisioning list with the payload (JSON) |
| `zpc/<home_id>/Network/SmartStart/Add` | Command | Append new entries to the SmartStart provisioning list (existing DSKs are skipped) |
| `zpc/<home_id>/Network/SmartStart/Remove` | Command | Remove entries identified by a list of DSKs (unknown DSKs are skipped) |
| `zpc/<home_id>/Network/SmartStart/Clear` | Command | Purge the entire SmartStart provisioning list (payload ignored) |
| `zpc/<home_id>/Network/SmartStart/List` | Command | Request the current SmartStart list (pending entries; payload ignored) |
| `zpc/<home_id>/Network/SmartStart/List/Report` | Report | SmartStart list contents (`value` array, same shape as `Update`) |

**Full reference:** [SmartStart MQTT API](smartstart_mqtt_api.md)

## Device Interview

Request a commissioning interview (via Network Management) and receive completion notifications (per endpoint).

| Topic pattern | Direction | Description |
|---------------|-----------|-------------|
| `zpc/<home_id>/Network/Node/Interview` | Command | Request a (re-)interview for a node (`node_id`); see [Network Management MQTT API](../../network_manager/doc/network_management_mqtt_api.md#network_node_interview) |
| `zpc/<home_id>/Interview/Report` | Report | Published when an endpoint interview finishes (`node_id`, `endpoint_id`, `status`) |

**Full reference:** [Device Interviewer](../../device_interviewer/docs/device_interviewer.md#mqtt-api) (MQTT API section)

## Network Status

Unsolicited reports reflecting the availability of Z-Wave nodes (online/offline/unknown transitions for Always-Listening, FLiRS, and Non-Listening devices).

| Topic pattern | Direction | Description |
|---------------|-----------|-------------|
| `zpc/<home_id>/Network/Status/Report` | Report | Node availability transitions (`node_id`, `status`, optional `reason`) |

**Full reference:** [Network Status](../../network_monitor/doc/network_status.md)

## OTA (Firmware Update)

Firmware update over the air: image management, start/abort, progress, and activation. Commands are relative to `zpc/<home_id>/` and reports are published under the same prefix.

| Topic pattern | Direction | Description |
|---------------|-----------|-------------|
| `zpc/<home_id>/OTA/UploadImage` | Command | Store a `.gbl` image in the cache |
| `zpc/<home_id>/OTA/UploadImage/Report` | Report | Store result |
| `zpc/<home_id>/OTA/ListImages` | Command | List cached images |
| `zpc/<home_id>/OTA/ListImages/Report` | Report | Array of cached images |
| `zpc/<home_id>/OTA/RemoveImage` | Command | Remove a cached image by name |
| `zpc/<home_id>/OTA/RemoveImage/Report` | Report | Remove result |
| `zpc/<home_id>/OTA/StartFirmwareUpload` | Command | Start OTA for a node (`node_id`, `image_name`, `wait_for_activation`) |
| `zpc/<home_id>/OTA/StartFirmwareUpload/Report` | Report | Accept / reject / error |
| `zpc/<home_id>/OTA/Progress` | Command | Request a one-shot progress snapshot (payload ignored) |
| `zpc/<home_id>/OTA/Progress/Report` | Report | Progress snapshot and final completion / failure status |
| `zpc/<home_id>/OTA/Abort` | Command | Abort the current transfer |
| `zpc/<home_id>/OTA/Activate` | Command | Apply firmware stored in "waiting for activation" (`node_id`) |
| `zpc/<home_id>/OTA/Activate/Report` | Report | Parse errors on the activation command (device status travels on `OTA/Progress/Report`) |

Per-node Firmware Update MD command/report topics are published alongside OTA and follow the command class topic pattern, e.g. `zpc/<home_id>/<node_id>/ep0/FirmwareUpdateMd/Report/FirmwareUpdateMdStatusReport`.

**Full reference:** [OTA Firmware Manager](../../ota/docs/ota.md) — state machine, end-to-end example, and payload conventions.

## Command Classes

Per–command-class MQTT topics for controlling and monitoring devices. Topic pattern:

`zpc/<home_id>/<node_id>/ep<endpoint_id>/<CommandClass>/Command/<Command>` and `.../Report/<Report>`.

**Full reference:** [Command Classes MQTT Interface](../../command_classes/doc/generated/mqtt_interface.md) — endpoint addressing and links to each command class MQTT doc.

## Sequences

High-level flows that involve multiple MQTT topics and internal state:

- **[Inclusion flow](../../../docs/sequences/inclusion_flow.md)** — From Add through S2/DSK (if needed), or from SmartStart (S2/DSK handled internally by ZPC), to node added and device interview; MQTT and state machine overview.
- **[Exclusion flow](../../../docs/sequences/exclusion_flow.md)** — From Node Remove command to node removed report; MQTT and state machine overview.

## See also

- [MQTT API Overview](mqtt_api_overview.md) — Architecture and how to implement MQTT API classes.
- [MQTT API Interface](mqtt_api_interface.md) — Topic naming conventions and `MqttApiBase` reference.
