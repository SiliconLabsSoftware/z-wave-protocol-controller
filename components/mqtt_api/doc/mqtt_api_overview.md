# MQTT API Implementation Guide

## Table of Contents
- [Architecture Overview](#architecture-overview)
- [Initialization Locations](#initialization-locations)
- [MqttApiBase - Factory Pattern](#mqttapibase-factory-pattern)
- [Creating MQTT API Classes](#creating-mqtt-api-classes)

## Architecture Overview

The MQTT API component uses a **distributed initialization pattern** where each MQTT API class is initialized in its respective component location:

1. **`SmartStartMqttApi`** - Initialized in `smartstart_handler::initialize()`
2. **`NetworkManagementMqttApi`** - Initialized in `network_management_handler::initialize()`
3. **`DeviceInterviewerMqttApi`** - Initialized in `device_interviewer::initialize()`
4. **`DiscoveryMqttApi`** - Initialized via `DiscoveryMqttApiInitializer` in `main.cpp`
5. **`OTAMqttApi`** - Initialized in `update_manager::initialize()` (OTA component)
6. **`NetworkMonitorMqttApi`** - Initialized in `network_monitor::initialize()` and publishes unsolicited `Network/Status/Report`
7. **`MqttApiBase`** - Uses the **Factory Pattern** to provide a framework for creating MQTT API classes

This approach improves cohesion by keeping MQTT API initialization close to the components that use them.

### Architecture Diagram

```mermaid
graph TB
    subgraph SmartStart["SmartStart Component"]
        SmartStartHandler[smartstart_handler]
        SmartStartAPI[SmartStartMqttApi]
        SmartStartHandler -->|owns/initializes| SmartStartAPI
    end
    
    subgraph NetworkMgmt["Network Management Component"]
        NetworkHandler[network_management_handler]
        NetworkAPI[NetworkManagementMqttApi]
        NetworkHandler -->|owns/initializes| NetworkAPI
    end
    
    subgraph DeviceInterviewer["Device Interviewer Component"]
        DeviceInterviewerMain[device_interviewer]
        DeviceInterviewerAPI[DeviceInterviewerMqttApi]
        DeviceInterviewerMain -->|owns/initializes| DeviceInterviewerAPI
    end
    
    subgraph Discovery["Discovery Component"]
        DiscoveryInit[DiscoveryMqttApiInitializer]
        DiscoveryAPI[DiscoveryMqttApi]
        DiscoveryInit -->|initializes| DiscoveryAPI
    end

    subgraph OTA["OTA Component"]
        UpdateManager[update_manager]
        OtaAPI[OTAMqttApi]
        UpdateManager -->|owns/initializes| OtaAPI
    end

    subgraph NetworkMonitor["Network Monitor Component"]
        NetMonitor[network_monitor]
        NetMonitorAPI[NetworkMonitorMqttApi]
        NetMonitor -->|owns/initializes| NetMonitorAPI
    end

    subgraph Base["MQTT API Base"]
        MqttApiBase[MqttApiBase<br/>Factory Pattern]
        MQTT[MQTT Handler]
        MqttApiBase -->|uses| MQTT
    end
    
    SmartStartAPI -->|inherits from| MqttApiBase
    NetworkAPI -->|inherits from| MqttApiBase
    DeviceInterviewerAPI -->|inherits from| MqttApiBase
    DiscoveryAPI -->|inherits from| MqttApiBase
    OtaAPI -->|inherits from| MqttApiBase
    NetMonitorAPI -->|inherits from| MqttApiBase

    style SmartStartHandler fill:#e1f5ff
    style NetworkHandler fill:#e1f5ff
    style DeviceInterviewerMain fill:#e1f5ff
    style DiscoveryInit fill:#e1f5ff
    style UpdateManager fill:#e1f5ff
    style NetMonitor fill:#e1f5ff
    style SmartStartAPI fill:#e8f5e9
    style NetworkAPI fill:#e8f5e9
    style DeviceInterviewerAPI fill:#e8f5e9
    style DiscoveryAPI fill:#e8f5e9
    style OtaAPI fill:#e8f5e9
    style NetMonitorAPI fill:#e8f5e9
    style MqttApiBase fill:#fff4e1
    style MQTT fill:#ffe1f5
```

## Initialization Locations

### SmartStartMqttApi

**Location:** `components/smartstart/include/smartstart.hpp` and `components/smartstart/src/smartstart.cpp`

The `SmartStartMqttApi` is initialized as part of the `smartstart_handler` class:

```cpp
class smartstart_handler : public threading::threading, public Initializable {
private:
    zwave_command_class::SmartStartMqttApi smartstart_mqtt_api_instance;
    // ...
};

sl_status_t smartstart_handler::initialize() {
    // ... other initialization ...
    smartstart_mqtt_api_instance.setup_mqtt_api();
    return SL_STATUS_OK;
}
```

**Initialization Order:** The `smartstart_handler` is initialized in `main.cpp` after the MQTT handler is ready, ensuring the MQTT API can subscribe to topics.

### NetworkManagementMqttApi

**Location:** `components/network_manager/include/network_management_handler.hpp` and `components/network_manager/src/zpc_network_management.cpp`

The `NetworkManagementMqttApi` is initialized as part of the `network_management_handler` class:

```cpp
class network_management_handler : public threading::threading, public Initializable {
private:
    zwave_command_class::NetworkManagementMqttApi network_management_mqtt_api_instance;
    // ...
};

sl_status_t network_management_handler::initialize() {
    network_management_mqtt_api_instance.setup_mqtt_api();
    return SL_STATUS_OK;
}
```

**Initialization Order:** The `network_management_handler` is initialized in `main.cpp` after the MQTT handler is ready.

### DeviceInterviewerMqttApi

**Location:** `components/device_interviewer/inc/device_interviewer.hpp` and `components/device_interviewer/src/device_interviewer.cpp`

The `DeviceInterviewerMqttApi` is initialized as part of the `device_interviewer` class:

```cpp
class device_interviewer : public threading::threading, public Initializable {
private:
    zwave_command_class::DeviceInterviewerMqttApi device_interviewer_mqtt_api;
    // ...
};

sl_status_t device_interviewer::initialize() {
    // ... other initialization ...
    device_interviewer_mqtt_api.setup_mqtt_api();
    return SL_STATUS_OK;
}
```

**Initialization Order:** The `device_interviewer` is initialized in `main.cpp` after the MQTT handler is ready. It publishes to `Interview/Report` when a device interview terminates (per node and per endpoint), including a `status` field for the interview result.

### DiscoveryMqttApi

**Location:** `components/discovery/include/discovery_mqtt_api_initializer.hpp` and `components/discovery/src/discovery_mqtt_api_initializer.cpp`

The `DiscoveryMqttApi` is initialized via a standalone `DiscoveryMqttApiInitializer` class:

```cpp
class DiscoveryMqttApiInitializer : public Initializable {
private:
    static DiscoveryMqttApi discovery_mqtt_api_instance;
    // ...
};

sl_status_t DiscoveryMqttApiInitializer::initialize() {
    discovery_mqtt_api_instance.setup_mqtt_api();
    return SL_STATUS_OK;
}
```

**Initialization Order:** The `DiscoveryMqttApiInitializer` is added to `main.cpp` after the MQTT handler is initialized.

### OTAMqttApi

**Location:** `components/ota/include/ota_mqtt_api.hpp` and `components/ota/src/ota_mqtt_api.cpp`

The `OTAMqttApi` is owned by the OTA `update_manager` component, which runs its own worker thread and state machine. Commands that need state-machine handling (`StartFirmwareUpload`, `Abort`, `Activate`, `Progress`) enqueue `ota_external_event_data` on the worker queue; `UploadImage`, `ListImages`, and `RemoveImage` are handled synchronously by the API. See [OTA Firmware Manager](../../ota/docs/ota.md) for the full state machine and MQTT topic list.

### NetworkMonitorMqttApi

**Location:** `components/network_monitor/`

The `NetworkMonitorMqttApi` is initialized as part of the `network_monitor` component. It publishes **unsolicited** `Network/Status/Report` messages that reflect node availability transitions (online / offline / unknown) for Always-Listening, FLiRS, and Non-Listening devices. See [Network Status](../../network_monitor/doc/network_status.md) for the payload and lifecycle details.

### Initialization Flow

```mermaid
sequenceDiagram
    participant Main as main.cpp
    participant InitBuilder
    participant MQTTHandler as MQTT Handler
    participant SmartStartHandler as smartstart_handler
    participant NetworkHandler as network_management_handler
    participant DeviceInterviewer as device_interviewer
    participant DiscoveryInit as DiscoveryMqttApiInitializer
    participant UpdateManager as ota::update_manager
    participant NetMonitor as network_monitor

    Main->>InitBuilder: add(MQTT Handler)
    InitBuilder->>MQTTHandler: initialize()

    Main->>InitBuilder: add(smartstart_handler)
    InitBuilder->>SmartStartHandler: initialize()
    SmartStartHandler->>SmartStartHandler: smartstart_mqtt_api.setup_mqtt_api()

    Main->>InitBuilder: add(network_management_handler)
    InitBuilder->>NetworkHandler: initialize()
    NetworkHandler->>NetworkHandler: network_management_mqtt_api.setup_mqtt_api()

    Main->>InitBuilder: add(device_interviewer)
    InitBuilder->>DeviceInterviewer: initialize()
    DeviceInterviewer->>DeviceInterviewer: device_interviewer_mqtt_api.setup_mqtt_api()

    Main->>InitBuilder: add(DiscoveryMqttApiInitializer)
    InitBuilder->>DiscoveryInit: initialize()
    DiscoveryInit->>DiscoveryInit: discovery_mqtt_api.setup_mqtt_api()

    Main->>InitBuilder: add(ota::update_manager)
    InitBuilder->>UpdateManager: initialize()
    UpdateManager->>UpdateManager: ota_mqtt_api.setup_mqtt_api()

    Main->>InitBuilder: add(network_monitor)
    InitBuilder->>NetMonitor: initialize()
    NetMonitor->>NetMonitor: network_monitor_mqtt_api.setup_mqtt_api()
```

## MqttApiBase - Factory Pattern

The `MqttApiBase` uses the **Factory Pattern** to provide a framework for creating MQTT API classes. All specialized API classes inherit from this base class, which acts as a factory that provides the common infrastructure needed to create functional MQTT API implementations.

### Architecture

```mermaid
graph TB
    Base[MqttApiBase<br/>Factory Pattern]
    MQTT[MQTT Handler<br/>subscribe/publish]
    
    Base -->|factory for| Discovery[DiscoveryMqttApi]
    Base -->|factory for| Network[NetworkManagementMqttApi]
    Base -->|factory for| SmartStart[SmartStartMqttApi]
    Base -->|factory for| DevInt[DeviceInterviewerMqttApi]
    Base -->|factory for| Ota[OTAMqttApi]
    Base -->|factory for| NetMon[NetworkMonitorMqttApi]

    Base -->|uses| MQTT

    style Base fill:#fff4e1
    style Discovery fill:#e8f5e9
    style Network fill:#e8f5e9
    style SmartStart fill:#e8f5e9
    style DevInt fill:#e8f5e9
    style Ota fill:#e8f5e9
    style NetMon fill:#e8f5e9
    style MQTT fill:#ffe1f5
```

### How It Works

The `MqttApiBase` acts as a factory that:
- Provides the common infrastructure (subscribe, publish, topic formatting) needed to create MQTT API classes
- Defines the interface (`setup_mqtt_api()`) that all MQTT API classes must implement
- Supplies protected helper methods (`subscribe_topic()`, `publish_report()`, `get_base_topic()`) for common MQTT operations
- Handles automatic base topic prefixing (`zpc/{home_id}/`)
- Encapsulates interaction with the MQTT Handler

### Topic Handling Flow

```mermaid
sequenceDiagram
    participant MQTT as MQTT Broker
    participant Handler as MQTT Handler
    participant API as MQTT API Class
    participant Base as MqttApiBase
    
    MQTT->>Handler: Message arrives
    Handler->>API: Callback invoked
    API->>API: Process message
    API->>Base: publish_report()
    Base->>Handler: Publish response
    Handler->>MQTT: Send report
```

## Creating MQTT API Classes

To create a new MQTT API class, inherit from `MqttApiBase` and implement the `setup_mqtt_api()` method. Then initialize it in the appropriate component location.

### Header Structure

```cpp
class MyFeatureMqttApi : public MqttApiBase {
public:
    void setup_mqtt_api() override;
private:
    inline static std::string MQTT_API_MY_FEATURE_TOPIC = "MyFeature/Command";
    void on_my_feature_command(const std::string &topic, const std::string &message);
};
```

### Implementation Pattern

```cpp
void MyFeatureMqttApi::setup_mqtt_api() {
    subscribe_topic(MQTT_API_MY_FEATURE_TOPIC, [this](auto& topic, auto& message) {
        this->on_my_feature_command(topic, message);
    });
}

void MyFeatureMqttApi::on_my_feature_command(const std::string &topic, const std::string &message) {
    // Process command
    publish_report("MyFeature/Report", response_json, false);
}
```

**Key Points:**
- Inherit from `MqttApiBase` to get common MQTT functionality
- Implement `setup_mqtt_api()` to set up topic subscriptions
- Use `subscribe_topic()` and `publish_report()` for MQTT operations
- Base topic (`zpc/{home_id}/`) is automatically prepended unless `add_base_topic=false`
- Initialize the API instance in the appropriate component handler or create a standalone initializer

**Reference Implementation:** See `components/discovery/include/discovery_mqtt_api.hpp` and `components/discovery/src/discovery_mqtt_api.cpp` for a complete example.

For detailed API reference, see [MQTT API Interface Documentation](mqtt_api_interface.md).
