# Jamming Detection

## Overview

The `jamming_detection` component exposes NCP RF jamming detection functionality through MQTT.

- Background RSSI sampling: every 100 ms, with a rolling window per channel.
- Notifications: jamming detected, cleared, or periodically reported.
- Configuration: stored in NCP RAM; lost on reset.

## NCP compatibility

Requires proprietary Serial API function `0xF1`, advertised in `SERIAL_API_GET_CAPABILITIES`; otherwise configuration returns `not_supported`.

This is not a standard Z-Wave Command Class. Verify firmware support.

Configuration responses must echo the requested sub-command and all configuration fields. Notifications are unsolicited frames.

## MQTT API

All topics are relative to `zpc/<home_id>`.

Configuration reports use NCP-echoed values, which may differ from the request.
Success reports omit `reason`; failures use the format in [Failure reports](#failure-reports).

### Configuration/Channel

Configure jamming detection threshold and trigger for one channel.

**Subscribe:** `Network/JammingDetection/Configuration/Channel`

```json
{
  "channel": 3,
  "threshold_dbm": -50,
  "nb_samples": 95
}
```

| Field | Type | Range | Description |
|---|---|---|---|
| `channel` | Integer or numeric string | 0–4 | Channel index |
| `threshold_dbm` | Integer or numeric string | -128–0 | RSSI threshold in dBm |
| `nb_samples` | Integer or numeric string | 1–150 | Samples above threshold to trigger jamming |

**Publish:** `Network/JammingDetection/Configuration/Channel/Report`

```json
{
  "status": "success",
  "channel": 3,
  "threshold_dbm": -50,
  "nb_samples": 95
}
```

### Configuration/JammingReportPeriod

Set the report interval while a jamming condition persists.

**Subscribe:** `Network/JammingDetection/Configuration/JammingReportPeriod`

```json
{"period_secs": 5}
```

| Field | Type | Range | Description |
|---|---|---|---|
| `period_secs` | Integer or numeric string | 0–20 | Report period. 0 disables periodic reporting. |

**Publish:** `Network/JammingDetection/Configuration/JammingReportPeriod/Report`

```json
{
  "status": "success",
  "period_secs": 5
}
```

### Configuration/RssiCollection

Start or stop periodic RSSI collection.

**Subscribe:** `Network/JammingDetection/Configuration/RssiCollection`

```json
{"period_100ms": 100}
```

| Field | Type | Range | Description |
|---|---|---|---|
| `period_100ms` | Integer or numeric string | 0–65535 | 0 stops collection; 65535 (0xFFFF) is continuous. |

**Publish:** `Network/JammingDetection/Configuration/RssiCollection/Report`

```json
{
  "status": "success",
  "period_100ms": 100
}
```

### Failure reports

```json
{"status": "fail", "reason": "invalid_parameter"}
```

| Reason | Meaning |
|---|---|
| `"invalid_parameter"` | Malformed JSON, missing fields, wrong types, or values outside the documented ranges. |
| `"operation_failed"` | Generic Serial API failure, including transport failures and missing or mismatched NCP echoes. |
| `"timeout"` | When Serial API operation returns `SL_STATUS_TIMEOUT`. |
| `"not_supported"` | When Serial API operation returns `SL_STATUS_NOT_SUPPORTED`. |
| `"busy"` | Configuration queue is full, or Serial API operation returns `SL_STATUS_BUSY`. |
| `"not_ready"` | Component is shut down, or Serial API operation returns `SL_STATUS_NOT_READY`. |
| `"unknown"` | When Serial API operation returns an unrecognized status. |

### Notification/Jamming

Published when the NCP detects a change in jamming state or reports while jamming persists.

**Publish:** `Network/JammingDetection/Notification/Jamming`

```json
{
  "channels": [
    {
      "channel": 0,
      "jammed": true,
      "threshold_dbm": -50,
      "nb_samples_trigger_notification": 95,
      "samples_above_threshold": 101
    },
    {"channel": 1, "jammed": false, "threshold_dbm": -50, "nb_samples_trigger_notification": 95, "samples_above_threshold": 0},
    {"channel": 2, "jammed": false, "threshold_dbm": -50, "nb_samples_trigger_notification": 95, "samples_above_threshold": 0},
    {"channel": 3, "jammed": false, "threshold_dbm": -50, "nb_samples_trigger_notification": 95, "samples_above_threshold": 0},
    {"channel": 4, "jammed": false, "threshold_dbm": 0,   "nb_samples_trigger_notification": 0,  "samples_above_threshold": 0}
  ]
}
```

### Notification/RssiCollection

Published for each 100 ms collection period while RSSI collection is active.

**Publish:** `Network/JammingDetection/Notification/RssiCollection`

```json
{
  "channels": [
    {"channel": 0, "rssi_dbm": -102},
    {"channel": 1, "rssi_dbm": -103},
    {"channel": 2, "rssi_dbm": -103},
    {"channel": 3, "rssi_dbm": -97},
    {"channel": 4, "rssi_dbm": -128}
  ]
}
```

`rssi_dbm` of `-128` indicates an invalid or unavailable RSSI reading for that channel.
