from dataclasses import dataclass
from xml.etree.ElementTree import Element
from modules.zwave_xml_parser.xml_element_version_tracker import XMLElementVersionTracker


@dataclass
class FieldEnumItem:
    value: str | None
    id: str | None

    @classmethod
    def from_xml_element(cls, element: Element, index: int) -> 'FieldEnumItem':
        value = element.attrib.get("value", "UNDEFINED_FIELDENUMITEM_VALUE")
        id = f"0x{index:02X}"

        return cls(
            value=value,
            id=id
        )
