## COMMAND_CLASS_FIRMWARE_UPDATE_MD MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| false | true | true |

> When MQTT Support is `false`, only incoming (TX) commands are listed below. ZPC publishes these to MQTT but does not handle `Command/*` topics for this command class.

### Table of Contents
- [FIRMWARE_MD_REPORT](#firmware_md_report)
- [FIRMWARE_UPDATE_MD_GET](#firmware_update_md_get)
- [FIRMWARE_UPDATE_MD_REQUEST_REPORT](#firmware_update_md_request_report)
- [FIRMWARE_UPDATE_MD_STATUS_REPORT](#firmware_update_md_status_report)
- [FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT](#firmware_update_activation_status_report)
- [FIRMWARE_UPDATE_MD_PREPARE_REPORT](#firmware_update_md_prepare_report)
### FIRMWARE_MD_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/FirmwareUpdateMd/Report/FirmwareMdReport
```

**Payload:**
```json
{
  "manufacturer_id": "0x2574",
  "firmware_0_id": "0x2574",
  "firmware_0_checksum": "0x2574",
  "firmware_upgradable": "0x12",
  "number_of_firmware_targets": "0x12",
  "max_fragment_size": "0x2574",
  "vg1": [
    {
      "firmware_id": "0x2574"
    }
  ],
  "hardware_version": "0x12"
}
```
### FIRMWARE_UPDATE_MD_GET
> This command is received by ZPC but is **not** published to MQTT `Report/*` topics.

                                                                                                                                                                                          
### FIRMWARE_UPDATE_MD_REQUEST_REPORT
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/FirmwareUpdateMd/Report/FirmwareUpdateMdRequestReport
```

**Payload:**
```json
{
  "status": "0x01"
}
```
### FIRMWARE_UPDATE_MD_STATUS_REPORT
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/FirmwareUpdateMd/Report/FirmwareUpdateMdStatusReport
```

**Payload:**
```json
{
  "status": "0x01",
  "waittime": "0x2574"
}
```
### FIRMWARE_UPDATE_ACTIVATION_STATUS_REPORT
                                                                                                                                                                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/FirmwareUpdateMd/Report/FirmwareUpdateActivationStatusReport
```

**Payload:**
```json
{
  "manufacturer_id": "0x2574",
  "firmware_id": "0x2574",
  "checksum": "0x2574",
  "firmware_target": "0x12",
  "firmware_update_status": "0x01",
  "hardware_version": "0x12"
}
```
### FIRMWARE_UPDATE_MD_PREPARE_REPORT
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/FirmwareUpdateMd/Report/FirmwareUpdateMdPrepareReport
```

**Payload:**
```json
{
  "status": "0x01",
  "firmware_checksum": "0x2574"
}
```
