from dataclasses import dataclass
from xml.etree.ElementTree import Element
from modules.zwave_xml_parser.xml_element_version_tracker import XMLElementVersionTracker


@dataclass
class Variant:
    param_offset: int | None
    size_mask: int | None
    size_change: int | None

    @classmethod
    def from_xml_element(cls, element: Element, version_tracker: XMLElementVersionTracker, cc_name: str, command_name: str, param_name: str, cc_version: int) -> 'Variant':
        param_offset = int(element.attrib.get("paramoffs", "0"))
        size_mask = int(element.attrib.get("sizemask", "0x00"), 16)
        size_change = int(element.attrib.get("sizechange", "0"))

        return cls(
            param_offset=param_offset,
            size_mask=size_mask,
            size_change=size_change,
        )
