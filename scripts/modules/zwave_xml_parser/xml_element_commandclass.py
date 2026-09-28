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
    manual_security_validation: bool = False
    commands: List[Command] = field(default_factory=list)
    protocol: bool = False
    generate_commands: list[str] | None = None

    @property
    def has_mqtt_interface_doc(self) -> bool:
        if self.mqtt_support:
            return True
        return any(command.is_tx() for command in self.commands)

    @classmethod
    def from_xml_element(cls, element: Element, version_tracker: XMLElementVersionTracker, supported_command_class: dict) -> 'CommandClass':
        commands = []
        name = element.attrib.get("name", "UNDEFINED_COMMANDCLASS_NAME")
        if not name.startswith("COMMAND_CLASS_"):
            name = f"COMMAND_CLASS_{name}"
        id = int(element.attrib.get("key", "0x00"), 16)
        version = int(element.attrib.get("version", "0"))
        mqtt_support = supported_command_class.get('mqtt_support', False)
        minimal_scheme = supported_command_class.get('minimal_scheme', None)
        manual_security_validation = supported_command_class.get('manual_security_validation', False)
        support = supported_command_class.get('support', False)
        control = supported_command_class.get('control', False)
        has_endpoints = supported_command_class.get('has_endpoints', False)
        protocol = bool(supported_command_class.get('protocol', False))

        if protocol and 'generate_commands' not in supported_command_class:
            raise ValueError(
                f"{name}: protocol command classes must set generate_commands (use [] for a shell-only CC)")

        generate_commands = supported_command_class.get('generate_commands')

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
            manual_security_validation=manual_security_validation,
            support=support,
            control=control,
            has_endpoints=has_endpoints,
            commands=commands,
            protocol=protocol,
            generate_commands=generate_commands,
        )
