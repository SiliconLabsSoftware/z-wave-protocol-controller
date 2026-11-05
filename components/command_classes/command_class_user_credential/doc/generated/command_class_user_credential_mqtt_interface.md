## COMMAND_CLASS_USER_CREDENTIAL MQTT API

| MQTT Support | Support | Control |
|--------------|---------|---------|
| true | false | true |

### Table of Contents
- [USER_CAPABILITIES_GET](#user_capabilities_get)
- [USER_CAPABILITIES_REPORT](#user_capabilities_report)
- [CREDENTIAL_CAPABILITIES_GET](#credential_capabilities_get)
- [CREDENTIAL_CAPABILITIES_REPORT](#credential_capabilities_report)
- [USER_SET](#user_set)
- [USER_GET](#user_get)
- [USER_REPORT](#user_report)
- [CREDENTIAL_SET](#credential_set)
- [CREDENTIAL_GET](#credential_get)
- [CREDENTIAL_REPORT](#credential_report)
- [CREDENTIAL_LEARN_START](#credential_learn_start)
- [CREDENTIAL_LEARN_CANCEL](#credential_learn_cancel)
- [CREDENTIAL_LEARN_REPORT](#credential_learn_report)
- [USER_CREDENTIAL_ASSOCIATION_SET](#user_credential_association_set)
- [USER_CREDENTIAL_ASSOCIATION_REPORT](#user_credential_association_report)
- [ALL_USERS_CHECKSUM_GET](#all_users_checksum_get)
- [ALL_USERS_CHECKSUM_REPORT](#all_users_checksum_report)
- [USER_CHECKSUM_GET](#user_checksum_get)
- [USER_CHECKSUM_REPORT](#user_checksum_report)
- [CREDENTIAL_CHECKSUM_GET](#credential_checksum_get)
- [CREDENTIAL_CHECKSUM_REPORT](#credential_checksum_report)
- [ADMIN_PIN_CODE_SET](#admin_pin_code_set)
- [ADMIN_PIN_CODE_GET](#admin_pin_code_get)
- [ADMIN_PIN_CODE_REPORT](#admin_pin_code_report)
### USER_CAPABILITIES_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Command/UserCapabilitiesGet
```

**Payload:**
```json
{ }
```
### USER_CAPABILITIES_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Report/UserCapabilitiesReport
```

**Payload:**
```json
{
  "number_of_supported_user_unique_identifiers": "0x2574",
  "max_length_of_user_name": "0x12",
  "properties1": {
    "ascii_encoding_support": "0x05",
    "extended_ascii_encoding_support": "0x05",
    "utf_16_encoding_support": "0x05",
    "user_checksum_support": "0x05",
    "all_users_checksum_support": "0x05",
    "user_schedule_support": "0x05"
  },
  "supported_user_types_bit_mask_length": "0x12"
}
```
### CREDENTIAL_CAPABILITIES_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Command/CredentialCapabilitiesGet
```

**Payload:**
```json
{ }
```
### CREDENTIAL_CAPABILITIES_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Report/CredentialCapabilitiesReport
```

**Payload:**
```json
{
  "properties1": {
    "admin_code_deactivation_support": "0x05",
    "admin_code_support": "0x05",
    "credential_checksum_support": "0x05"
  },
  "number_of_supported_credential_types": "0x12",
  "vg1 credential type": [
    {
      "credential_type": "0x01"
    }
  ],
  "vg2 properties2 per credential type": [
    {
      "properties2": {
        "reserved2": "0x05",
        "cl_support": "0x05"
      }
    }
  ],
  "vg3 number of supported credential slots per credential type": [
    {
      "number_of_supported_credential_slots": "0x2574"
    }
  ],
  "vg4 min length of credential data per credential type": [
    {
      "min_length_of_credential_data": "0x12"
    }
  ],
  "vg5 max length of credential data per credential type": [
    {
      "max_length_of_credential_data": "0x12"
    }
  ],
  "vg6 cl recommended timeout per credential type": [
    {
      "cl_recommended_timeout": "0x12"
    }
  ],
  "vg7 cl number of steps per credential type": [
    {
      "cl_number_of_steps": "0x12"
    }
  ],
  "vg8 maximum credential hash length per credential type": [
    {
      "maximum_credential_hash_length": "0x12"
    }
  ]
}
```
### USER_SET
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Command/UserSet
```

**Payload:**
```json
{
  "properties1": {
    "operation_type": "0x05"
  },
  "user_unique_identifier": "0x2574",
  "user_type": "0x01",
  "properties2": {
    "user_active_state": "0x05",
    "reserved2": "0x05"
  },
  "credential_rule": "0x12",
  "expiring_timeout_minutes": "0x2574",
  "properties3": {
    "user_name_encoding": "0x05",
    "reserved3": "0x05"
  },
  "user_name_length": "0x12",
  "user_name": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### USER_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Command/UserGet
```

**Payload:**
```json
{
  "user_unique_identifier": "0x2574"
}
```
### USER_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Report/UserReport
```

**Payload:**
```json
{
  "user_report_type": "0x01",
  "next_user_unique_identifier": "0x2574",
  "user_modifier_type": "0x01",
  "user_modifier_node_id": "0x2574",
  "user_unique_identifier": "0x2574",
  "user_type": "0x01",
  "properties1": {
    "user_active_state": "0x05"
  },
  "credential_rule": "0x12",
  "expiring_timeout_minutes": "0x2574",
  "properties2": {
    "user_name_encoding": "0x05",
    "reserved2": "0x05"
  },
  "user_name_length": "0x12",
  "user_name": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### CREDENTIAL_SET
                                                                                                                                                                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Command/CredentialSet
```

**Payload:**
```json
{
  "user_unique_identifier": "0x2574",
  "credential_type": "0x01",
  "credential_slot": "0x2574",
  "properties1": {
    "operation_type": "0x05"
  },
  "credential_length": "0x12",
  "credential_data": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### CREDENTIAL_GET
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Command/CredentialGet
```

**Payload:**
```json
{
  "user_unique_identifier": "0x2574",
  "credential_type": "0x01",
  "credential_slot": "0x2574"
}
```
### CREDENTIAL_REPORT
                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Report/CredentialReport
```

**Payload:**
```json
{
  "credential_report_type": "0x01",
  "user_unique_identifier": "0x2574",
  "credential_type": "0x01",
  "credential_slot": "0x2574",
  "properties1": {
    "crb": "0x05"
  },
  "credential_length": "0x12",
  "credential_data": [
    "0x01",
    "0x02",
    "0x03"
  ],
  "credential_modifier_type": "0x01",
  "credential_modifier_node_id": "0x2574",
  "next_credential_type": "0x01",
  "next_credential_slot": "0x2574"
}
```
### CREDENTIAL_LEARN_START
                                                                                                                                                                                                                                                
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Command/CredentialLearnStart
```

**Payload:**
```json
{
  "user_unique_identifier": "0x2574",
  "credential_type": "0x01",
  "credential_slot": "0x2574",
  "properties1": {
    "operation_type": "0x05"
  },
  "credential_learn_timeout": "0x12"
}
```
### CREDENTIAL_LEARN_CANCEL
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Command/CredentialLearnCancel
```

**Payload:**
```json
{ }
```
### CREDENTIAL_LEARN_REPORT
                                                                                                                                                                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Report/CredentialLearnReport
```

**Payload:**
```json
{
  "credential_learn_status": "0x01",
  "user_unique_identifier": "0x2574",
  "credential_type": "0x01",
  "credential_slot": "0x2574",
  "credential_learn_steps_remaining": "0x12"
}
```
### USER_CREDENTIAL_ASSOCIATION_SET
                                                                                                                  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Command/UserCredentialAssociationSet
```

**Payload:**
```json
{
  "credential_type": "0x01",
  "credential_slot": "0x2574",
  "destination_user_unique_identifier": "0x2574"
}
```
### USER_CREDENTIAL_ASSOCIATION_REPORT
                                                                                                                                                      
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Report/UserCredentialAssociationReport
```

**Payload:**
```json
{
  "credential_type": "0x01",
  "credential_slot": "0x2574",
  "destination_user_unique_identifier": "0x2574",
  "user_credential_association_status": "0x01"
}
```
### ALL_USERS_CHECKSUM_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Command/AllUsersChecksumGet
```

**Payload:**
```json
{ }
```
### ALL_USERS_CHECKSUM_REPORT
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Report/AllUsersChecksumReport
```

**Payload:**
```json
{
  "all_users_checksum": "0x2574"
}
```
### USER_CHECKSUM_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Command/UserChecksumGet
```

**Payload:**
```json
{
  "user_unique_identifier": "0x2574"
}
```
### USER_CHECKSUM_REPORT
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Report/UserChecksumReport
```

**Payload:**
```json
{
  "user_unique_identifier": "0x2574",
  "user_checksum": "0x2574"
}
```
### CREDENTIAL_CHECKSUM_GET
                                          
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Command/CredentialChecksumGet
```

**Payload:**
```json
{
  "credential_type": "0x01"
}
```
### CREDENTIAL_CHECKSUM_REPORT
                                                                              
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Report/CredentialChecksumReport
```

**Payload:**
```json
{
  "credential_type": "0x01",
  "credential_checksum": "0x2574"
}
```
### ADMIN_PIN_CODE_SET
                                                                                                                                    
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Command/AdminPinCodeSet
```

**Payload:**
```json
{
  "properties1": {
    "admin_pin_code_length": "0x05"
  },
  "admin_code": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
### ADMIN_PIN_CODE_GET
  
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Command/AdminPinCodeGet
```

**Payload:**
```json
{ }
```
### ADMIN_PIN_CODE_REPORT
                                                                                                                                                      
**Command:**
```sh
zpc/<home_id>/<node_id>/ep<endpoint_id>/UserCredential/Report/AdminPinCodeReport
```

**Payload:**
```json
{
  "properties1": {
    "admin_pin_code_length": "0x05",
    "admin_pin_code_operation_result": "0x05"
  },
  "admin_code": [
    "0x01",
    "0x02",
    "0x03"
  ]
}
```
