from dataclasses import dataclass, field
from typing import List
from xml.etree.ElementTree import Element
from modules.zwave_xml_parser.xml_element_version_tracker import XMLElementVersionTracker
from modules.zwave_xml_parser.xml_element_param import Param


@dataclass
class ParamGroup:
    name: str | None
    id: int | None
    param_offset: int | None
    size_mask: int | None
    size_offset: int | None
    min_version: int | None = None
    type: str | None = None
    params: List[Param] = field(default_factory=list)

    @classmethod
    def from_xml_element(cls, element: Element, version_tracker: XMLElementVersionTracker, cc_name: str, command_name: str, cc_version: int) -> 'ParamGroup':
        params = []
        id = int(element.attrib.get("key", "0x00"), 16)
        name = element.attrib.get("name", "UNDEFINED_PARAM_NAME")
        param_offset = int(element.attrib.get("paramOffs", "0x00"), 16)
        size_mask = int(element.attrib.get("sizemask", "0x00"), 16)
        size_offset = int(element.attrib.get("sizeoffs", "0x00"), 16)
        type = element.attrib.get("type", "VARIANT_GROUP")

        min_version = version_tracker.track_element(
            cls.__name__, id, cc_version, f"{cc_name}:{command_name}")

        for child in element:
            if child.tag == "param":
                params.append(Param.from_xml_element(
                    child, version_tracker, cc_name, name, cc_version))

        return cls(
            name=name,
            id=id,
            param_offset=param_offset,
            size_mask=size_mask,
            size_offset=size_offset,
            min_version=min_version,
            type=type,
            params=params
        )
