# Z-Wave Protocol Controller (ZPC)

ZPC is a Linux/macOS Z-Wave gateway application that acts as the host-side static controller for a Silicon Labs NCP. It exposes network management, device interview, and Command Class control over **MQTT**, and persists state in an attribute store with a reported/desired value model.

This site is the **full documentation for ZPC**. For a quick project overview, the feature list, the supported-command-class table, and the Switch On/Off end-to-end demo, see the top-level [project README](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller/blob/main/README.md) on GitHub.

## Start here

- **[Getting started](getting-started.md)** — dependencies, NCP setup, build, configure, and run
- **[High-Level Architecture](high-level-architecture.md)** — overview of ZPC's main components and their roles
- **[Debian packaging](packaging-debian.md)** — build a `.deb` on Linux
- **[Switch On/Off demo](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller/blob/main/README.md#switch-onoff-demo)** — full end-to-end walk-through in the top-level README
- **[Command Class Implementation Guide](command_class_implementation_guide.md)** — add or modify Z-Wave command classes
- **[Known Failing CTT Test Cases](known_failing_ctt_test_cases.md)** — automated CTT test failures tracked against ZPC

## MQTT API

- **[MQTT API index](../components/mqtt_api/doc/mqtt_api_index.md)** — single source of truth for every MQTT topic (Discovery, Network Management, SmartStart, Device Interview, Network Status, OTA, Command Classes)
- [MQTT API Overview](../components/mqtt_api/doc/mqtt_api_overview.md) — architecture and per-component initialization
- [MQTT API Interface](../components/mqtt_api/doc/mqtt_api_interface.md) — `MqttApiBase` reference and topic conventions

## Component documentation

- [Network Management](../components/network_manager/doc/network_management_mqtt_api.md) — Add/Remove, Remove Failed, DSK/Grant Keys, Node List, Factory Reset, NLS
- [Device Interviewer](../components/device_interviewer/docs/device_interviewer.md) — interview state machine and `Interview/Report`
- [Network Status](../components/network_monitor/doc/network_status.md) — unsolicited `Network/Status/Report`
- [OTA Firmware Manager](../components/ota/docs/ota.md) — firmware update state machine, MQTT commands/reports, end-to-end example

## Command classes

- [Command Classes MQTT Interface](../components/command_classes/doc/generated/mqtt_interface.md) — endpoint addressing and per-CC MQTT topic references

## Sequences

- [Inclusion flow](sequences/inclusion_flow.md) — from Add / SmartStart through S2/DSK to interview completion
- [Exclusion flow](sequences/exclusion_flow.md) — from Node Remove to node removed

## Resources

- [Contributing](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller/blob/main/CONTRIBUTING.md)
- [License](https://github.com/SiliconLabsSoftware/z-wave-protocol-controller/blob/main/LICENSE.md)

## Legal info

**Copyright 2026 Silicon Laboratories Inc. www.silabs.com**

The licensor of this software is Silicon Laboratories Inc. Your use of this software is governed by the terms of the Silicon Labs Master Software License Agreement (MSLA), available at www.silabs.com/about-us/legal/master-software-license-agreement. This software is distributed in Source Code format and is governed by the sections of the MSLA applicable to Source Code.
