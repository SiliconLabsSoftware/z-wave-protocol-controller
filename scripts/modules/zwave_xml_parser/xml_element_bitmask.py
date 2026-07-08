from dataclasses import dataclass
from xml.etree.ElementTree import Element
from modules.zwave_xml_parser.xml_element_version_tracker import XMLElementVersionTracker


@dataclass
class BitMask:
    id: int
    paramoffs: int
    len_mask: str
    min_version: int | None = None

    @classmethod
    def from_xml_element(cls, element: Element, version_tracker: XMLElementVersionTracker, cc_name: str, command_name: str, param_name: str, cc_version: int) -> 'BitMask':
        id = int(element.attrib.get("key", "0x00"), 16)
        paramoffs = int(element.attrib.get("paramoffs", "0"))
        len_mask = element.attrib.get("lenmask", "0x00")

        min_version = version_tracker.track_element(
            cls.__name__, id, cc_version, f"{cc_name}:{command_name}:{param_name}")

        return cls(
            id=id,
            paramoffs=paramoffs,
            len_mask=len_mask,
            min_version=min_version
        )
