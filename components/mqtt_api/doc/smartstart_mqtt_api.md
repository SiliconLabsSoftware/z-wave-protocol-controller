# SmartStart MQTT API

SmartStart allows pre-provisioned devices to join the network automatically when they are powered and in range. The client maintains a SmartStart provisioning list (DSKs and optional metadata) and synchronizes it with ZPC via the [`Update`](#network_smartstart_update), [`Add`](#network_smartstart_add), [`Remove`](#network_smartstart_remove), and [`Clear`](#network_smartstart_clear) topics. The current contents of the list can be read back at any time via [`List`](#network_smartstart_list). When a provisioned node is detected, ZPC initiates inclusion automatically.

## Table of Contents

- [NETWORK_SMARTSTART_UPDATE](#network_smartstart_update)
- [NETWORK_SMARTSTART_ADD](#network_smartstart_add)
- [NETWORK_SMARTSTART_REMOVE](#network_smartstart_remove)
- [NETWORK_SMARTSTART_CLEAR](#network_smartstart_clear)
- [NETWORK_SMARTSTART_LIST](#network_smartstart_list)
- [Entry lifecycle and persistence](#entry-lifecycle-and-persistence)

## NETWORK_SMARTSTART_UPDATE

**Topic:**
```sh
zpc/<home_id>/Network/SmartStart/Update
```

**Semantics:**

This message **replaces ZPC's entire SmartStart provisioning list** with the entries in the payload. It is not an incremental "add": every publish on this topic is a complete snapshot of the list. Any entry that was previously known to ZPC but is **not** present in the new payload is removed from the list. For incremental changes, prefer the dedicated topics: [`Add`](#network_smartstart_add) to append entries, [`Remove`](#network_smartstart_remove) to remove specific DSKs, or [`Clear`](#network_smartstart_clear) to purge the list.

**Payload:**

The client publishes the SmartStart provisioning list as JSON. The payload contains a `value` array of provisioning entries. Each entry includes the device DSK and optional fields such as `PreferredProtocols` (e.g. `"Z-Wave"` or `"Z-Wave Long Range"`). Every entry in the list is eligible for automatic inclusion; to keep a DSK from being included, leave it out of the list (or remove it via [`Remove`](#network_smartstart_remove) / [`Clear`](#network_smartstart_clear)).

**Example:**
```json
{
  "value": [
    {
      "DSK": "06743-56104-10648-10659-64918-00784-01021-46920",
      "PreferredProtocols": [
        "Z-Wave"
      ]
    }
  ]
}
```

When ZPC receives a message on this topic, it rebuilds its SmartStart provisioning list from the payload. Nodes that appear in the list and are powered in range are then included automatically without any further user interaction: ZPC grants all keys requested by the node and verifies the DSK against the provisioning entry internally. The `Network/RequestedKeys/Report`, `Network/GrantKeys`, `Network/RequestedDSK/Report`, and `Network/DSK/Accept` topics are **not** used during SmartStart inclusion, they apply only to standard inclusion via [`Network/Node/Add`](../../network_manager/doc/network_management_mqtt_api.md#network_node_add).

## NETWORK_SMARTSTART_ADD

**Topic:**
```sh
zpc/<home_id>/Network/SmartStart/Add
```

**Semantics:**

This message **appends entries** to ZPC's existing SmartStart provisioning list. Unlike [`Network/SmartStart/Update`](#network_smartstart_update), the rest of the list is left untouched. Entries whose `DSK` is already present in the list are **skipped** (the existing entry is kept as-is); to change an existing entry, use [`Network/SmartStart/Update`](#network_smartstart_update) with the full new list.

**Payload:**

Same shape as [`Network/SmartStart/Update`](#network_smartstart_update): a `value` array of provisioning entries. One or more new DSKs may be added in a single publish.

**Example (adding a single DSK):**
```json
{
  "value": [
    {
      "DSK": "06743-56104-10648-10659-64918-00784-01021-46920",
      "PreferredProtocols": [
        "Z-Wave"
      ]
    }
  ]
}
```

There is no dedicated report topic for this command. The resulting list state can be observed via [`Network/SmartStart/List`](#network_smartstart_list).

## NETWORK_SMARTSTART_REMOVE

**Topic:**
```sh
zpc/<home_id>/Network/SmartStart/Remove
```

**Semantics:**

This message **removes the entries** identified by the DSKs in the payload from ZPC's SmartStart provisioning list. The rest of the list is left untouched. DSKs that are not in the list are silently skipped.

**Payload:**

The payload uses the same shape as [`Network/SmartStart/Update`](#network_smartstart_update): a `value` array of entry objects. Only the `DSK` field of each entry is read; any other fields (e.g. `PreferredProtocols`, …) are tolerated and ignored. This lets clients pass entries copied from [`Network/SmartStart/List/Report`](#network_smartstart_list) straight back into `Remove` without re-shaping them.

**Example (removing two DSKs):**
```json
{
  "value": [
    {
      "DSK": "06743-56104-10648-10659-64918-00784-01021-46920"
    },
    {
      "DSK": "12345-56789-01234-56789-01234-56789-01234-56789"
    }
  ]
}
```

There is no dedicated report topic for this command. The resulting list state can be observed via [`Network/SmartStart/List`](#network_smartstart_list).

## NETWORK_SMARTSTART_CLEAR

**Topic:**
```sh
zpc/<home_id>/Network/SmartStart/Clear
```

**Semantics:**

This message **purges the entire SmartStart provisioning list**. After clear, the list is empty and SmartStart automatic inclusion is disabled until new entries are added (via [`Network/SmartStart/Update`](#network_smartstart_update) or [`Network/SmartStart/Add`](#network_smartstart_add)).

The request payload is ignored (publish any value, e.g. `{}`).

There is no dedicated report topic for this command. The resulting list state can be observed via [`Network/SmartStart/List`](#network_smartstart_list).

> Note: The same effect can be achieved with [`Network/SmartStart/Update`](#network_smartstart_update) and an empty `value` array. `Clear` is provided as a more explicit and self-documenting alternative.

## NETWORK_SMARTSTART_LIST

Returns the current SmartStart provisioning list, which represents the set of pending nodes still tracked for SmartStart inclusion. Once a device has been **successfully** included (`kex_fail` none), ZPC removes its entry from the list. On **security bootstrapping failure**, the DSK is retained and SmartStart add mode stays/re-enables so a later NIF can retry without a client re-provision. A leftover NodeID from a failed attempt (ghost / self-destruct pending) does **not** count as successfully included and must not remove the DSK from the list.

**Topic (request):**
```sh
zpc/<home_id>/Network/SmartStart/List
```

The request payload is ignored (publish any value, e.g. `{}`).

**Topic (report):**
```sh
zpc/<home_id>/Network/SmartStart/List/Report
```

**Report payload:**

The report mirrors the shape used by [`Network/SmartStart/Update`](#network_smartstart_update) — a `value` array of provisioning entries. Each entry contains the same fields as the input.

**Example:**
```json
{
  "value": [
    {
      "DSK": "06743-56104-10648-10659-64918-00784-01021-46920",
      "PreferredProtocols": [
        "Z-Wave"
      ]
    }
  ]
}
```

> **Notes:**
>
> - Every entry in the list is pending automatic inclusion; there is no per-entry flag to keep a tracked DSK from being included. To stop a DSK from being included, remove it from the list via [`Network/SmartStart/Remove`](#network_smartstart_remove) or [`Network/SmartStart/Clear`](#network_smartstart_clear).
> - An empty `value` array means the list is empty (e.g. all provisioned devices have been included or have had an inclusion attempt, or the list was cleared via [`Network/SmartStart/Clear`](#network_smartstart_clear)).

## Entry lifecycle and persistence

The SmartStart list tracks **pending** entries — DSKs that ZPC is still actively waiting to include. ZPC automatically removes an entry as soon as it is no longer pending. The following events cause removal:

- **Inclusion attempt completes (success or failure).** Once a node is added — or the inclusion attempt fails — ZPC removes the matching DSK from the list so the SmartStart engine does not retry inclusion for that device. This applies whether the inclusion was started via SmartStart or via [`Network/Node/Add`](../../network_manager/doc/network_management_mqtt_api.md#network_node_add).
- **DSK is already in the current network.** When the list is updated (via [`Update`](#network_smartstart_update) or [`Add`](#network_smartstart_add)), ZPC checks each entry against the existing network. If the corresponding device is already included, ZPC removes that entry from the list immediately — no retry, no re-inclusion.

**Re-including a device after exclusion.** Because the entry is removed at inclusion time, an excluded device will **not** be re-included by SmartStart, even if it is powered on again afterwards. To re-enable automatic inclusion for that DSK, the client must re-publish it via [`Update`](#network_smartstart_update) or [`Add`](#network_smartstart_add).

**Persistence.** The SmartStart list is held in memory only — it is **not** preserved across ZPC restarts. After ZPC restarts (or otherwise re-initializes), the list is empty, and the client must re-publish it (e.g. via [`Update`](#network_smartstart_update)) before SmartStart can include any device again.

## Related topics

- **Inclusion flow:** See [Inclusion flow](../../../docs/sequences/inclusion_flow.md) for the full sequence from add request to interviewed node.
- **S2 during SmartStart inclusion:** SmartStart inclusion does not involve user interaction in the bootstrapping process. ZPC grants every key the node requests and verifies the DSK against the provisioning entry on its own. The `Network/RequestedKeys/Report`, `Network/GrantKeys`, `Network/RequestedDSK/Report`, and `Network/DSK/Accept` topics — documented in [Network Management MQTT API](../../network_manager/doc/network_management_mqtt_api.md) — are used only during standard inclusion started via [`Network/Node/Add`](../../network_manager/doc/network_management_mqtt_api.md#network_node_add).
