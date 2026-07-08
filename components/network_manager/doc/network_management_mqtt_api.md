## NETWORK MANAGEMENT MQTT API

> **Breaking change:** Node inventory topics were renamed from the old
> Device-prefixed variants to the Node-prefixed variants. Use:
> `Network/Node/List`, `Network/Node/List/Report`,
> `Network/Node/Properties`, `Network/Node/Properties/Report`.

### Table of Contents
- [Inclusion](#inclusion)
    - [`NETWORK_NODE_ADD`](#network_node_add)
    - [`NETWORK_NODE_ADD_REPORT`](#network_node_add_report)
    - [`NETWORK_NODE_ADD_ABORT`](#network_node_add_abort)
- [Exclusion and Remove Failed](#exclusion-and-remove-failed)
    - [`NETWORK_NODE_REMOVE`](#network_node_remove)
    - [`NETWORK_NODE_REMOVE_REPORT`](#network_node_remove_report)
    - [`NETWORK_NODE_REMOVE_FAILED`](#network_node_remove_failed)
    - [`NETWORK_NODE_REMOVE_FAILED_REPORT`](#network_node_remove_failed_report)
    - [`NETWORK_NODE_REMOVE_ABORT`](#network_node_remove_abort)
- [S2 Security During Inclusion](#s2-security-during-inclusion)
    - [`NETWORK_DSK_ACCEPT`](#network_dsk_accept)
    - [`NETWORK_REQUESTED_KEYS_REPORT`](#network_requested_keys_report)
    - [`NETWORK_GRANT_KEYS`](#network_grant_keys)
    - [`NETWORK_REQUESTED_DSK_REPORT`](#network_requested_dsk_report)
- [Node Inventory and Properties](#node-inventory-and-properties)
    - [`NETWORK_NODE_LIST`](#network_node_list)
    - [`NETWORK_NODE_LIST_REPORT`](#network_node_list_report)
    - [`NETWORK_NODE_PROPERTIES`](#network_node_properties)
    - [`NETWORK_NODE_PROPERTIES_REPORT`](#network_node_properties_report)
- [Factory Reset](#factory-reset)
    - [`NETWORK_FACTORY_RESET`](#network_factory_reset)
    - [`NETWORK_FACTORY_RESET_REPORT`](#network_factory_reset_report)
- [Network Layer Security (NLS)](#network-layer-security-nls)
    - [`NETWORK_NLS_ENABLE`](#network_nls_enable)
    - [`NETWORK_NLS_ENABLE_REPORT`](#network_nls_enable_report)
    - [`NETWORK_NLS_STATE`](#network_nls_state)
    - [`NETWORK_NLS_STATE_REPORT`](#network_nls_state_report)

## Inclusion

### NETWORK_NODE_ADD

**Command:**
```sh
zpc/<home_id>/Network/Node/Add
```

**Payload:**
```json
{ }
```

### NETWORK_NODE_ADD_REPORT

**Report (published by ZPC):**
```sh
zpc/<home_id>/Network/Node/Add/Report
```

**Payload (success):**
```json
{
  "node_id": 2,
  "dsk": <dsk_string>,
  "status": <status_code>
}
```

Published when a node has been successfully added (inclusion and security bootstrapping succeeded). `dsk` is included when available.

| Field | Type | Description |
|-------|------|-------------|
| `node_id` | number | Present on success. The NodeID of the added node. |
| `dsk` | string | Present on success when available. The DSK of the added node. |
| `status` | string | Success or failure of the operation. See status codes below. |

**Payload (fail — request rejected):**
```json
{
  "status": <status_code>,
  "reason": <reason_code>,
  "activity": <activity_string>
}
```

Published immediately if `Network/Node/Add` is rejected due to any of the following reasons:
- Network management is busy
- Reset is ongoing

| Field | Type | Description |
|-------|------|-------------|
| `status` | string | Success or failure of the operation. See status codes below. |
| `reason` | string | Present on failure. Why the request was rejected. See reason codes below. |
| `activity` | string | Present when `reason` is `6401` (`NETWORK_MANAGEMENT_BUSY`). Describes what the network management module is currently doing. See activity values below. |

**Payload (fail — security bootstrapping):**
```json
{
  "node_id": 273,
  "dsk": "xxxxx-xxxxx-xxxxx-xxxxx-xxxxx-xxxxx-xxxxx-xxxxx",
  "status": "fail",
  "reason": 6404
}
```

Published when inclusion assigned a NodeID but S2/S0 bootstrapping failed or secure-add timed out. For SmartStart the DSK is kept and may retry on the next NIF.

| Field | Type | Description |
|-------|------|-------------|
| `node_id` | number | The NodeID assigned during the add attempt. |
| `dsk` | string | The DSK of the joining node when available (SmartStart uses the provisioned DSK even if the S2 challenge did not complete). |
| `status` | string | `"fail"` when security bootstrapping failed. |
| `reason` | number | `6404` (`NODE_ADD_SECURITY_FAIL`) — secure-add timeout or S2/S0 kex failure. |

On security failure (classic **and** SmartStart), ZPC publishes this fail report **before** self-destruct/remove-failed of the ghost NodeID. Interview is not started. For SmartStart, the DSK remains in the provisioning cache and add mode stays/re-enables so a later NIF can retry without a client re-provision. For classic inclusion, reinclude with a new `Network/Node/Add` after the ghost is removed.

**Status codes:**

| Code | Meaning |
|------|---------|
| `"success"` | Success |
| `"fail"` | Fail |

**Reason codes:**

| Code | Meaning |
|------|---------|
| `6401` (`zpc_status_t::NETWORK_MANAGEMENT_BUSY`) | Network management is busy |
| `6402` (`zpc_status_t::FACTORY_RESET_ONGOING`) | Factory reset is ongoing |
| `6404` (`NODE_ADD_SECURITY_FAIL`) | Security bootstrapping failed (secure-add timeout or S2/S0 kex failure) |

**Activity values:**

> **Note:** The `activity` field is a temporary field and will be removed in the future.

| Value | Meaning |
|-------|---------|
| `"inclusion"` | An inclusion (add node) operation is in progress |
| `"exclusion"` | An exclusion (remove node) operation is in progress |
| `"learning"` | Learn mode operation is in progress |
| `"internal"` | An internal operation is in progress |
| `"idle"` | No operation is in progress |
| `"unknown"` | An unknown operation is in progress |

### NETWORK_NODE_ADD_ABORT

**Command:**
```sh
zpc/<home_id>/Network/Node/Add/Abort
```

**Payload:**
```json
{ }
```

Aborts an ongoing add node (inclusion) operation. If the controller is currently in add mode, the operation is stopped and ZPC returns to idle. No report is published.

## Exclusion and Remove Failed

### NETWORK_NODE_REMOVE

**Command:**
```sh
zpc/<home_id>/Network/Node/Remove
```

**Payload:**
```json
{ }
```

### NETWORK_NODE_REMOVE_REPORT

**Report (published by ZPC):**
```sh
zpc/<home_id>/Network/Node/Remove/Report
```

**Payload (success):**
```json
{
  "node_id": 2,
  "dsk": "xxxxx-xxxxx-xxxxx-xxxxx-xxxxx-xxxxx-xxxxx-xxxxx",
  "status": <status_code>
}
```

Published when a node is removed from the network, whether by normal exclusion (`Network/Node/Remove`) or by a successful remove-failed operation (`Network/Node/RemoveFailed`).

| Field | Type | Description |
|-------|------|-------------|
| `node_id` | number | Present on success. The NodeID of the removed node. |
| `dsk` | string | Present on success when available. The DSK of the removed node. |
| `status` | string | Success or failure of the operation. See status codes below. |

**Payload (fail):**
```json
{
  "status": <status_code>,
  "reason": <reason_code>,
  "activity": <activity_string>
}
```

Published immediately if `Network/Node/Remove` is rejected due to any of the following reasons:
- Network management is busy
- Reset is ongoing

| Field | Type | Description |
|-------|------|-------------|
| `status` | string | Success or failure of the operation. See status codes below. |
| `reason` | string | Present on failure. Why the request was rejected. See reason codes below. |
| `activity` | string | Present when `reason` is `6401` (`NETWORK_MANAGEMENT_BUSY`). Describes what the network management module is currently doing. See activity values below. |

**Status codes:**

| Code | Meaning |
|------|---------|
| `"success"` | Success |
| `"fail"` | Fail |

**Reason codes:**

| Code | Meaning |
|------|---------|
| `6401` (`zpc_status_t::NETWORK_MANAGEMENT_BUSY`) | Network management is busy |
| `6402` (`zpc_status_t::FACTORY_RESET_ONGOING`) | Factory reset is ongoing |

**Activity values:**

> **Note:** The `activity` field is a temporary field and will be removed in the future.

| Value | Meaning |
|-------|---------|
| `"inclusion"` | An inclusion (add node) operation is in progress |
| `"exclusion"` | An exclusion (remove node) operation is in progress |
| `"learning"` | Learn mode operation is in progress |
| `"internal"` | An internal operation is in progress |
| `"idle"` | No operation is in progress |
| `"unknown"` | An unknown operation is in progress |

### NETWORK_NODE_REMOVE_FAILED

**Command:**
```sh
zpc/<home_id>/Network/Node/RemoveFailed
```

**Payload:**
```json
{
  "node_id": 2
}
```

Removes a failing (non-responsive) node from the network. Use this when a node cannot be excluded normally because it is unreachable or non-functional. The Z-Wave controller first sends a NOP to verify the node is truly unreachable, then marks it as failed and removes it from the routing tables.

| Field | Type | Description |
|-------|------|-------------|
| `node_id` | number | The NodeID of the failing node to remove (must be non-zero). |

**Behavior:**

- If the node is **not reachable** (NOP fails): the controller proceeds with the removal and reports `{"status": "ok", "reason": "operation_successful"}` on `zpc/<home_id>/Network/Node/RemoveFailed/Report`. A `zpc/<home_id>/Network/Node/Remove/Report` is also published with the node's DSK, just like a normal exclusion.
- If the node is **reachable** (NOP succeeds): the node is not considered failed, so the controller does not remove it and reports `{"status": "fail", "reason": "node_online"}` on `zpc/<home_id>/Network/Node/RemoveFailed/Report`. Use the normal exclusion flow (`zpc/<home_id>/Network/Node/Remove`) instead.

### NETWORK_NODE_REMOVE_FAILED_REPORT

**Report (published by ZPC):**
```sh
zpc/<home_id>/Network/Node/RemoveFailed/Report
```

**Payload (success — node removed):**
```json
{
  "node_id": 2,
  "status": "ok",
  "reason": "operation_successful"
}
```

**Payload (fail — node is reachable):**
```json
{
  "node_id": 2,
  "status": "fail",
  "reason": "node_online"
}
```

When the removal succeeds, a separate [`Network/Node/Remove/Report`](#network_node_remove_report) is also published with the removed node's ID and DSK.

| Field | Type | Description |
|-------|------|-------------|
| `node_id` | number | The NodeID that was requested for removal. |
| `status` | string | `"ok"` if the node was successfully removed, `"fail"` otherwise. |
| `reason` | string | The detailed reason for the result. See table below. |

**Possible `reason` values:**

| Reason | Status | Description |
|--------|--------|-------------|
| `operation_successful` | `ok` | The node was successfully removed from the network. |
| `operation_failed` | `fail` | The remove-failed operation could not be started. |
| `operation_aborted` | `fail` | The operation was aborted by the user. |
| `not_removed` | `fail` | The node could not be removed (NOP timed out before a response). |
| `node_online` | `fail` | The node responded to a NOP; it is still reachable and not considered failed. |
| `timeout` | `fail` | Timed out waiting for the protocol to complete the removal. |
| `not_ready` | `fail` | The network management is not in an idle state to start the operation. |

### NETWORK_NODE_REMOVE_ABORT

**Command:**
```sh
zpc/<home_id>/Network/Node/Remove/Abort
```

**Payload:**
```json
{ }
```

Aborts an ongoing remove node (exclusion) operation. If the controller is currently in remove mode, the operation is stopped and ZPC returns to idle. No report is published.

## S2 Security During Inclusion

### NETWORK_DSK_ACCEPT

**Command:**
```sh
zpc/<home_id>/Network/DSK/Accept
```

**Payload:**
```json
{
  "dsk": "12345"
}
```

During a standard S2 inclusion (started via [`Network/Node/Add`](#network_node_add)), when the node reports its DSK and user verification is needed, ZPC publishes to `zpc/<home_id>/Network/RequestedDSK/Report`. The client should respond by publishing to this topic with the accepted DSK. SmartStart inclusion does not use this topic — DSK verification is handled internally by ZPC against the provisioning entry.

### NETWORK_REQUESTED_KEYS_REPORT

**Report (published by ZPC):**
```sh
zpc/<home_id>/Network/RequestedKeys/Report
```

**Payload:**
```json
{
  "Keys": "0x7",
  "CSA": false
}
```

Published during a standard S2 inclusion (started via [`Network/Node/Add`](#network_node_add)) when the node requests security keys. The client should respond by publishing to `zpc/<home_id>/Network/GrantKeys` with the desired keys (see [NETWORK_GRANT_KEYS](#network_grant_keys)). This topic is **not** published during SmartStart inclusion — ZPC grants all requested keys internally.

### NETWORK_GRANT_KEYS

**Command:**
```sh
zpc/<home_id>/Network/GrantKeys
```

**Payload:**
```json
{
  "Accept": true,
  "Keys": 7,
  "CSA": false
}
```

| Field | Type | Description |
|-------|------|-------------|
| `Accept` | boolean | Whether to accept the key request. |
| `Keys` | number or string | Keys to grant (bitmask; use value from RequestedKeys/Report). |
| `CSA` | boolean | (Optional) Client Side Authentication; use the CSA value from RequestedKeys/Report if present. |

Sent in response to `Network/RequestedKeys/Report` during a standard S2 inclusion to grant or deny the requested security keys. This topic is not used during SmartStart inclusion — granted keys cannot be selected for SmartStart nodes and ZPC grants every key the node requests.

### NETWORK_REQUESTED_DSK_REPORT

**Report (published by ZPC):**
```sh
zpc/<home_id>/Network/RequestedDSK/Report
```

**Payload:**
```json
{
  "DSK": "12345-67890-..."
}
```

Published during a standard S2 inclusion (started via [`Network/Node/Add`](#network_node_add)) when DSK verification is needed. The client should respond by publishing to `zpc/<home_id>/Network/DSK/Accept` with the accepted DSK string. SmartStart inclusion does not publish this topic — DSK verification is performed automatically against the provisioning entry.

## Node Inventory and Properties

### NETWORK_NODE_LIST

**Command:**
```sh
zpc/<home_id>/Network/Node/List
```

**Payload:**
```json
{ }
```

### NETWORK_NODE_LIST_REPORT

**Report (published by ZPC):**
```sh
zpc/<home_id>/Network/Node/List/Report
```

**Payload:**
```json
[
  {
    "node_information": {
      "node_id": 2,
      "listening_protocol": 211,
      "optional_protocol": 156,
      "basic_device_class": 4,
      "generic_device_class": 16,
      "specific_device_class": 0,
      "command_class_list": [113, 114, 115],
      "s2_command_class_list": [113, 114, 115],
      "s0_command_class_list": [113, 114],
      "inclusion_protocol": 0,
      "granted_keys": 2
    },
    "version_report": {
      "z_wave_library_type": 6,
      "z_wave_protocol_version": 7,
      "z_wave_protocol_sub_version": 0,
      "firmware_0_version": 1,
      "firmware_0_sub_version": 0,
      "hardware_version": 1,
      "number_of_firmware_targets": 1
    }
  }
]
```

Every node entry uses the same `node_information` and `version_report` key set. Command class list fields are always arrays; they are empty when no value is available (for example `s2_command_class_list` or `s0_command_class_list` on a non-secure node). Other `node_information` fields may still be `null` when unavailable.

| Field | Type | Description |
|-------|------|-------------|
| `command_class_list` | array | Non-secure Node Information command classes (endpoint 0). Each element is an 8-bit command class identifier (decimal in JSON). Extended command classes (`0xF1`/`0xF2`/`0xF3` wire pairs) are omitted; do not treat extended second bytes as separate CCs. Empty when not yet available. |
| `s2_command_class_list` | array | S2 Commands Supported normal command class identifiers when Security 2 (0x9F) is in `command_class_list`; otherwise empty. Same 8-bit-only encoding as `command_class_list` (extended IDs are not expanded into their second byte). |
| `s0_command_class_list` | array | S0 Commands Supported normal command class identifiers from the `S0_COMMANDS_SUPPORTED_REPORT_GROUP` attribute store (populated when the S0 Commands Supported report is received). Populated when Security 0 (0x98) is in `command_class_list` and the report has been received; otherwise empty. Same 8-bit-only encoding as `command_class_list`. Not read from `command_class_list` or `ATTRIBUTE_ZWAVE_SECURE_NIF`. |
| `inclusion_protocol` | number | `0` = Z-Wave, `1` = Z-Wave Long Range (`zwave_protocol_t`). |
| `granted_keys` | number | Security key bitmask (`zwave_keyset_definitions.h`: `0x80` S0, `0x01` S2 Unauthenticated, `0x02` S2 Authenticated, `0x04` S2 Access). Reflects keys verified in the attribute store (may be `0` until discovery completes; not necessarily the inclusion-time grant). |

### NETWORK_NODE_PROPERTIES

**Command:**
```sh
zpc/<home_id>/Network/Node/Properties
```

**Payload:**
```json
{
  "node_id": 2
}
```

### NETWORK_NODE_PROPERTIES_REPORT

**Report (published by ZPC):**
```sh
zpc/<home_id>/Network/Node/Properties/Report
```

**Payload:**
```json
{
  "node_id": 2,
  "inclusion_protocol": 1,
  "granted_keys": 7,
  "last_rx_rssi": -50,
  "last_rx_tx_timestamp": 1234567890,
  "last_routing_path": [1, 2, 3, 0],
  "last_tx_ticks": 100,
  "last_number_of_repeaters": 2,
  "last_tx_power": 0,
  "s2_capability": true
}
```

The report always contains the same keys. Fields are `null` when no attribute-store value exists for the node (`s2_capability` is `false` when the node is not S2-capable).

| Field | Type | Description |
|-------|------|-------------|
| `inclusion_protocol` | number or null | `0` = Z-Wave, `1` = Z-Wave Long Range. |
| `granted_keys` | number or null | Security key bitmask; see Node List report table. |
| `last_rx_tx_timestamp` | number or null | Unix time (seconds) of last successful TX to or RX from the node. |
| `last_rx_rssi` | number or null | RSSI of the last received application frame (dBm). |
| `last_routing_path` | array or null | Last route repeaters `[r0, r1, r2, r3]`. |
| `last_tx_ticks` | number or null | Transmit duration of the last successful frame. |
| `last_number_of_repeaters` | number or null | Repeaters used on the last successful transmission. |
| `last_tx_power` | number or null | TX power used on the last successful transmission. |
| `s2_capability` | boolean | `true` if the node is S2-capable; `false` otherwise. |

## Factory Reset

### NETWORK_FACTORY_RESET

**Command:**
```sh
zpc/<home_id>/Network/FactoryReset
```

**Payload:**
```json
{ }
```

Initiates a factory reset of the Z-Wave controller. The controller will leave its current network and start a new one.

### NETWORK_FACTORY_RESET_REPORT

**Report (published by ZPC):**
```sh
zpc/Network/FactoryReset/Report
```

**Payload:**
```json
{
  "status": "ready",
  "home_id": "AABBCCDD"
}
```

Published exactly once after a successful factory reset, when the Z-Wave Controller
has entered the new network and is ready to be operated. The topic is global (no
`home_id` segment, like `zpc/Discovery/Report`), so clients can subscribe to a
stable string before issuing the [`Network/FactoryReset`](#network_factory_reset)
command without having to know the new Home ID in advance. The new Home ID is
delivered in the payload's `home_id` field.

| Field | Type | Description |
|-------|------|-------------|
| `status` | string | `"ready"` when the reset chain has completed and ZPC is operational on the new network. |
| `home_id` | string | The new 8-hex-digit Home ID assigned to ZPC after the reset. |

The report is not retained; only one report is emitted per completed reset. Plain
ZPC startup and learn-mode joins into another network do not emit this report.

## Network Layer Security (NLS)

### NETWORK_NLS_ENABLE

**Command:**
```sh
zpc/<home_id>/Network/NLS/Enable
```

**Payload:**
```json
{
  "node_id": 2
}
```

Enables Network Layer Security (NLS) for the given node. ZPC stores the desired NLS state and may report result on the report topic.

### NETWORK_NLS_ENABLE_REPORT

**Report (published by ZPC):**
```sh
zpc/<home_id>/Network/NLS/Enable/Report
```

**Payload:**
```json
{
  "node_id": 2,
  "status": "ok"
}
```

### NETWORK_NLS_STATE

**Command:**
```sh
zpc/<home_id>/Network/NLS/State
```

**Payload:**
```json
{
  "node_id": 2
}
```

Requests the current Network Layer Security (NLS) state and support for the given node.

### NETWORK_NLS_STATE_REPORT

**Report (published by ZPC):**
```sh
zpc/<home_id>/Network/NLS/State/Report
```

**Payload:**
```json
{
  "node_id": 2,
  "nls_support": true,
  "nls_state": true,
  "status": "ok"
}
```
