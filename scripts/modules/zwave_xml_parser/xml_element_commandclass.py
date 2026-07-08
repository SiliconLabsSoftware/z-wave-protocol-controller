from dataclasses import dataclass, field
from typing import List
from xml.etree.ElementTree import Element
from modules.zwave_xml_parser.xml_element_command import Command
from modules.zwave_xml_parser.xml_element_version_tracker import XMLElementVersionTracker


@dataclass
class CommandClass:
    name: str | None
    id: int | None
    supported_version: int | None
    mqtt_support: bool
    support: bool
    control: bool
    has_endpoints: bool
    minimal_scheme: str | None
    interview_attributes: List[str] = field(default_factory=list)
    commands: List[Command] = field(default_factory=list)

    @property
    def has_mqtt_interface_doc(self) -> bool:
        # Full MQTT docs when mqtt_support is enabled. Otherwise document only
        # when the CC publishes reports via mqtt_publish_report (TX commands).
        if self.mqtt_support:
            return True
        return any(command.is_tx() for command in self.commands)

    @classmethod
    def from_xml_element(cls, element: Element, version_tracker: XMLElementVersionTracker, supported_command_class: dict) -> 'CommandClass':
        commands = []
        name = element.attrib.get("name", "UNDEFINED_COMMANDCLASS_NAME")
        id = int(element.attrib.get("key", "0x00"), 16)
        version = int(element.attrib.get("version", "0"))
        support_mode = supported_command_class.get('support_mode', None)
        mqtt_support = supported_command_class.get('mqtt_support', False)
        minimal_scheme = supported_command_class.get('minimal_scheme', None)
        support = supported_command_class.get('support', False)
        control = supported_command_class.get('control', False)
        has_endpoints = supported_command_class.get('has_endpoints', False)
        interview_attributes = supported_command_class.get(
            'interview_attributes', [])

        for child in element:
            if child.tag == "cmd":
                commands.append(Command.from_xml_element(
                    child, version_tracker, name, version))

        return cls(
            name=name,
            id=id,
            supported_version=version,
            mqtt_support=mqtt_support,
            minimal_scheme=minimal_scheme,
            support=support,
            control=control,
            has_endpoints=has_endpoints,
            interview_attributes=interview_attributes,
            commands=commands
        )
