from dataclasses import dataclass, field
from typing import List
from xml.etree.ElementTree import Element
from modules.zwave_xml_parser.xml_element_version_tracker import XMLElementVersionTracker
from modules.zwave_xml_parser.xml_element_fieldenumitem import FieldEnumItem


@dataclass
class FieldEnum:
    name: str
    id: int
    mask: str
    mask_size: int
    shifter: str
    min_version: int | None = None
    enum_items: List[FieldEnumItem] = field(default_factory=list)

    @classmethod
    def from_xml_element(cls, element: Element, version_tracker: XMLElementVersionTracker, cc_name: str, command_name: str, param_name: str, cc_version: int) -> 'FieldEnum':
        name = element.attrib.get("fieldname", "UNDEFINED_FIELDENUM_NAME")
        id = int(element.attrib.get("key", "0x00"), 16)
        mask = element.attrib.get("fieldmask", "0x00")
        mask_size = bin(int(mask, 16)).count("1")
        shifter = element.attrib.get("shifter", "0")
        enum_items = []

        min_version = version_tracker.track_element(
            cls.__name__, id, cc_version, f"{cc_name}:{command_name}:{param_name}")

        for index, child in enumerate(element):
            if child.tag == "fieldenum":
                enum_items.append(
                    FieldEnumItem.from_xml_element(child, index))

        return cls(
            name=name,
            id=id,
            mask=mask,
            mask_size=mask_size,
            shifter=shifter,
            min_version=min_version,
            enum_items=enum_items
        )
