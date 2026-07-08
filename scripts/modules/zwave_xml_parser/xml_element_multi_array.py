from dataclasses import dataclass, field
from typing import List, Union
from xml.etree.ElementTree import Element
from modules.zwave_xml_parser.xml_element_version_tracker import XMLElementVersionTracker
from modules.zwave_xml_parser.xml_element_bitflag import BitFlag


@dataclass
class MultiArray:
    fields: List[Union[BitFlag]] = field(default_factory=list)

    @classmethod
    def from_xml_element(cls, element: Element, version_tracker: XMLElementVersionTracker, cc_name: str, command_name: str, param_name: str, cc_version: int) -> 'MultiArray':
        fields = []

        for child in element:
            if child.tag == "bitflag":
                fields.append(BitFlag.from_xml_element(
                    child, version_tracker, cc_name, command_name, None, cc_version))

        return cls(
            fields=fields
        )
