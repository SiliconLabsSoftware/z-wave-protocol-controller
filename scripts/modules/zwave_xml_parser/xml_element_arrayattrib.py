from dataclasses import dataclass
from xml.etree.ElementTree import Element
from modules.zwave_xml_parser.xml_element_version_tracker import XMLElementVersionTracker


@dataclass
class ArrayAttrib:
    id: int
    len: int
    min_version: int | None = None

    @classmethod
    def from_xml_element(cls, element: Element, version_tracker: XMLElementVersionTracker, cc_name: str, command_name: str, param_name: str, cc_version: int) -> 'ArrayAttrib':
        id = int(element.attrib.get("key", "0x00"), 16)
        len = int(element.attrib.get("len", "0x00"), 16)

        min_version = version_tracker.track_element(
            cls.__name__, id, cc_version, f"{cc_name}:{command_name}:{param_name}")

        return cls(
            id=id,
            len=len,
            min_version=min_version
        )
