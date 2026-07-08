## COMMAND_CLASS_VERSION MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | true | true |

### Table of Contents
- [VERSION_COMMAND_CLASS_GET](#version_command_class_get)
- [VERSION_COMMAND_CLASS_REPORT](#version_command_class_report)
- [VERSION_GET](#version_get)
- [VERSION_REPORT](#version_report)
- [VERSION_CAPABILITIES_GET](#version_capabilities_get)
- [VERSION_CAPABILITIES_REPORT](#version_capabilities_report)
- [VERSION_ZWAVE_SOFTWARE_GET](#version_zwave_software_get)
- [VERSION_ZWAVE_SOFTWARE_REPORT](#version_zwave_software_report)
### VERSION_COMMAND_CLASS_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Command/VersionCommandClassGet
```

**Payload:**
```json
{
  "requested_command_class": "0x12"
}
```
### VERSION_COMMAND_CLASS_REPORT
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Report/VersionCommandClassReport
```

**Payload:**
```json
{
  "requested_command_class": "0x12",
  "command_class_version": "0x12"
}
```
### VERSION_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Command/VersionGet
```

**Payload:**
```json
{ }
```
### VERSION_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                            
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Report/VersionReport
```

**Payload:**
```json
{
  "z_wave_library_type": "0x12",
  "z_wave_protocol_version": "0x12",
  "z_wave_protocol_sub_version": "0x12",
  "firmware_0_version": "0x12",
  "firmware_0_sub_version": "0x12",
  "hardware_version": "0x12",
  "number_of_firmware_targets": "0x12",
  "vg": [
    {
      "firmware_version": "0x12",
      "firmware_sub_version": "0x12"
    }
  ]
}
```
### VERSION_CAPABILITIES_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Command/VersionCapabilitiesGet
```

**Payload:**
```json
{ }
```
### VERSION_CAPABILITIES_REPORT
                                                                                                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Report/VersionCapabilitiesReport
```

**Payload:**
```json
{
  "properties1": {
    "version": "0x05",
    "command_class": "0x05",
    "z_wave_software": "0x05",
    "reserved1": "0x05"
  }
}
```
### VERSION_ZWAVE_SOFTWARE_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Command/VersionZwaveSoftwareGet
```

**Payload:**
```json
{ }
```
### VERSION_ZWAVE_SOFTWARE_REPORT
                                                                                                                                                                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/Version/Report/VersionZwaveSoftwareReport
```

**Payload:**
```json
{
  "sdk_version": "0x123",
  "application_framework_api_version": "0x123",
  "application_framework_build_number": "0x2574",
  "host_interface_version": "0x123",
  "host_interface_build_number": "0x2574",
  "z_wave_protocol_version": "0x123",
  "z_wave_protocol_build_number": "0x2574",
  "application_version": "0x123",
  "application_build_number": "0x2574"
}
```
