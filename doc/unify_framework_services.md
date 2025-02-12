# Framework Services


Note: For reference documentation please refer to
[UnifySDK documentation](
https://SiliconLabs.github.io/UnifySDK/doc/unify_framework_services
)

WARNING: The following chapters may be oudated,
they are still here until the new release of UnifySDK.
In the future, Z-Wave parts will be more isolated and the rest deduplicated.


The following services facilitate different functionalities in cooperation with
the Protocol Controller(s).

- [The UPVL](https://siliconlabs.github.io/UnifySDK/applications/upvl/readme_user) (Unify Provisioning List) serves the SmartStart Provisioning
list to perform SmartStart Security 2 (S2) inclusions and maintains the
ucl/SmartStart MQTT topic.

- [GMS](https://siliconlabs.github.io/UnifySDK/applications/gms/readme_user) (Group Manager) manages groups and bookkeeping between
protocol controllers and also publishes group state changes to the ucl/by-group
MQTT topic.

- [NAL](https://siliconlabs.github.io/UnifySDK/applications/nal/readme_user) (Name and Location service) is a helper MQTT component that allows for book-keeping of text names and locations that have been assigned. This functionality allows IoT Services to assign and read back a Name and a Location for each node/endpoint.

- [OTA Image Provider](https://siliconlabs.github.io/UnifySDK/applications/image_provider/readme_user) announces OTA
images available in OTA storage and publishes OTA binary on request.


- [UPTICap (upti_cap)](https://siliconlabs.github.io/UnifySDK/applications/upti_cap/readme_user) is an application to communicate with Silicon Labs WSTK adapters. The adapters capture data on the debug channel and publish the captured data as MQTT messages. Application provided strictly for
test and demonstration purposes only and is not suitable for production.

- [UPTIWriter (upti_writer)](https://siliconlabs.github.io/UnifySDK/applications/upti_writer/readme_user) is an application to receive trace packages captured
with _UPTI_CAP_ application and save them to a file in [Network Analyzer](https://docs.silabs.com/simplicity-studio-5-users-guide/latest/ss-5-users-guide-tools-network-analyzer/) compatible format. Application provided strictly for test and demonstration purposes only
and is not suitable for production.

