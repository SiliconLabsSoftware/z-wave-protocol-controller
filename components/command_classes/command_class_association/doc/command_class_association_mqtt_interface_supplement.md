## COMMAND_CLASS_ASSOCIATION MQTT API — Supplement

### ZPC Association Support Behavior

The ZPC dynamically reports the number of supported association groups based on the
`AGI_ZPC_GROUP` nodes present in the attribute store under the home ID.
Currently, only the **Lifeline group (Group 1)** is created, so the
`ASSOCIATION_GROUPINGS_REPORT` will advertise `supported_groupings = 1`.

Users cannot create associations targeting groups other than the Lifeline on the ZPC.
`Association Set` commands with a `grouping_identifier` exceeding the supported count
are ignored per Z-Wave specification compliance.
