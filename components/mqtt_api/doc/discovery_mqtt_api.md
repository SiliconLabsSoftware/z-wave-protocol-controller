# Discovery MQTT API

The Discovery API allows clients to obtain the Z-Wave Home ID of the controller. This is a global topic (not scoped by home ID) because the Home ID is unknown until after discovery.

## Table of Contents

- [DISCOVERY (Command)](#discovery-command)
- [DISCOVERY_REPORT (Report)](#discovery_report-report)

## DISCOVERY (Command)

**Topic:**
```sh
zpc/Discovery
```

**Payload:**
```json
{}
```

Publish any message (e.g. empty JSON `{}`) to this topic to request the current Home ID. ZPC responds on the report topic.

## DISCOVERY_REPORT (Report)

**Topic (published by ZPC):**
```sh
zpc/Discovery/Report
```

**Payload:**
```json
{
  "home_id": "CAFECAFE"
}
```

The `home_id` is the 8-character hexadecimal representation of the Z-Wave Home ID. Use this value to construct network-scoped topics: `zpc/<home_id>/...`.

## Example

```bash
# Request Home ID
mosquitto_pub -t "zpc/Discovery" -m '{}'

# Subscribe to receive the report
mosquitto_sub -t "zpc/Discovery/Report" -v
```
