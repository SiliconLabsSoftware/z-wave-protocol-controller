from dataclasses import dataclass
from xml.etree.ElementTree import Element
from modules.zwave_xml_parser.xml_element_version_tracker import XMLElementVersionTracker


@dataclass
class BitFlag:
    name: str
    id: int
    mask: str
    mask_size: int
    min_version: int | None = None

    @classmethod
    def from_xml_element(cls, element: Element, version_tracker: XMLElementVersionTracker, cc_name: str, command_name: str, param_name: str, cc_version: int) -> 'BitFlag':
        name = element.attrib.get("flagname", "UNDEFINED_BITFLAG_NAME")
        id = int(element.attrib.get("key", "0x00"), 16)
        mask = element.attrib.get("flagmask", "0x00")
        mask_size = bin(int(mask, 16)).count("1")

        min_version = version_tracker.track_element(
            cls.__name__, id, cc_version, f"{cc_name}:{command_name}:{param_name}")

        return cls(
            name=name,
            id=id,
            mask=mask,
            mask_size=mask_size,
            min_version=min_version
        )
