## COMMAND_CLASS_USER_CODE MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [USER_CODE_GET](#user_code_get)
- [USER_CODE_REPORT](#user_code_report)
- [USER_CODE_SET](#user_code_set)
- [USERS_NUMBER_GET](#users_number_get)
- [USERS_NUMBER_REPORT](#users_number_report)
- [EXTENDED_USER_CODE_SET](#extended_user_code_set)
- [EXTENDED_USER_CODE_GET](#extended_user_code_get)
- [EXTENDED_USER_CODE_REPORT](#extended_user_code_report)
- [USER_CODE_CAPABILITIES_GET](#user_code_capabilities_get)
- [USER_CODE_CAPABILITIES_REPORT](#user_code_capabilities_report)
- [USER_CODE_KEYPAD_MODE_SET](#user_code_keypad_mode_set)
- [USER_CODE_KEYPAD_MODE_GET](#user_code_keypad_mode_get)
- [USER_CODE_KEYPAD_MODE_REPORT](#user_code_keypad_mode_report)
- [ADMIN_CODE_SET](#admin_code_set)
- [ADMIN_CODE_GET](#admin_code_get)
- [ADMIN_CODE_REPORT](#admin_code_report)
- [USER_CODE_CHECKSUM_GET](#user_code_checksum_get)
- [USER_CODE_CHECKSUM_REPORT](#user_code_checksum_report)
### USER_CODE_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Command/UserCodeGet
```

**Payload:**
```json
{
  "user_identifier": "0x12"
}
```
### USER_CODE_REPORT
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Report/UserCodeReport
```

**Payload:**
```json
{
  "user_identifier": "0x12",
  "user_id_status": "0x01",
  "user_code": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### USER_CODE_SET
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Command/UserCodeSet
```

**Payload:**
```json
{
  "user_identifier": "0x12",
  "user_id_status": "0x01",
  "user_code": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### USERS_NUMBER_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Command/UsersNumberGet
```

**Payload:**
```json
{ }
```
### USERS_NUMBER_REPORT
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Report/UsersNumberReport
```

**Payload:**
```json
{
  "supported_users": "0x12",
  "extended_supported_users": "0x2574"
}
```
### EXTENDED_USER_CODE_SET
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Command/ExtendedUserCodeSet
```

**Payload:**
```json
{
  "number_of_user_codes": "0x12",
  "vg1": [
    {
      "user_identifier": "0x2574",
      "user_id_status": "0x01",
      "properties1": {
        "user_code_length": "0x05"
      },
      "user_code": [
        "0x01",
        "0x02",
        "0x03"
      ]
    }
  ]
}
```
### EXTENDED_USER_CODE_GET
                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Command/ExtendedUserCodeGet
```

**Payload:**
```json
{
  "user_identifier": "0x2574",
  "properties1": {
    "report_more": "0x05"
  }
}
```
### EXTENDED_USER_CODE_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Report/ExtendedUserCodeReport
```

**Payload:**
```json
{
  "number_of_user_codes": "0x12",
  "vg1": [
    {
      "user_identifier": "0x2574",
      "user_id_status": "0x01",
      "properties1": {
        "user_code_length": "0x05"
      },
      "user_code": [
        "0x01",
        "0x02",
        "0x03"
      ]
    }
  ],
  "next_user_identifier": "0x2574"
}
```
### USER_CODE_CAPABILITIES_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Command/UserCodeCapabilitiesGet
```

**Payload:**
```json
{ }
```
### USER_CODE_CAPABILITIES_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Report/UserCodeCapabilitiesReport
```

**Payload:**
```json
{
  "properties1": {
    "supported_user_id_status_bit_mask_length": "0x05",
    "reserved1": "0x05",
    "acd_support": "0x05",
    "ac_support": "0x05"
  },
  "supported_user_id_status_bit_mask": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "properties2": {
    "supported_keypad_modes_bit_mask_length": "0x05",
    "mucs_support": "0x05",
    "mucr_support": "0x05",
    "ucc_support": "0x05"
  },
  "supported_keypad_modes_bit_mask": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "properties3": {
    "supported_keys_bit_mask_length": "0x05",
    "reserved2": "0x05"
  },
  "supported_keys_bit_mask": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### USER_CODE_KEYPAD_MODE_SET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Command/UserCodeKeypadModeSet
```

**Payload:**
```json
{
  "keypad_mode": "0x01"
}
```
### USER_CODE_KEYPAD_MODE_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Command/UserCodeKeypadModeGet
```

**Payload:**
```json
{ }
```
### USER_CODE_KEYPAD_MODE_REPORT
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Report/UserCodeKeypadModeReport
```

**Payload:**
```json
{
  "keypad_mode": "0x01"
}
```
### ADMIN_CODE_SET
                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Command/AdminCodeSet
```

**Payload:**
```json
{
  "properties1": {
    "admin_code_length": "0x05"
  },
  "admin_code": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### ADMIN_CODE_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Command/AdminCodeGet
```

**Payload:**
```json
{ }
```
### ADMIN_CODE_REPORT
                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Report/AdminCodeReport
```

**Payload:**
```json
{
  "properties1": {
    "admin_code_length": "0x05"
  },
  "admin_code": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### USER_CODE_CHECKSUM_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Command/UserCodeChecksumGet
```

**Payload:**
```json
{ }
```
### USER_CODE_CHECKSUM_REPORT
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCode/Report/UserCodeChecksumReport
```

**Payload:**
```json
{
  "user_code_checksum": "0x2574"
}
```
