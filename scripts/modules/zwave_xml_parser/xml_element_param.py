from dataclasses import dataclass, field
from typing import List, Union
from xml.etree.ElementTree import Element
from modules.zwave_xml_parser.xml_element_version_tracker import XMLElementVersionTracker
from modules.zwave_xml_parser.xml_element_bitfield import BitField
from modules.zwave_xml_parser.xml_element_bitflag import BitFlag
from modules.zwave_xml_parser.xml_element_bitmask import BitMask
from modules.zwave_xml_parser.xml_element_fieldenum import FieldEnum
from modules.zwave_xml_parser.xml_element_const import Const
from modules.zwave_xml_parser.xml_element_variant import Variant
from modules.zwave_xml_parser.xml_element_multi_array import MultiArray
from modules.zwave_xml_parser.xml_element_arrayattrib import ArrayAttrib


@dataclass
class Param:
    name: str | None
    id: int | None
    type: str | None
    min_version: int | None = None
    optionaloffs: int | None = None
    optionalmask: int | None = None
    fields: List[Union[Const, BitFlag, BitField, FieldEnum, Variant, MultiArray, ArrayAttrib]] = field(
        default_factory=list)

    @classmethod
    def from_xml_element(cls, element: Element, version_tracker: XMLElementVersionTracker, cc_name: str, command_name: str, cc_version: int) -> 'Param':
        fields = []
        id = int(element.attrib.get("key", "0x00"), 16)
        name = element.attrib.get("name", "UNDEFINED_PARAM_NAME")
        type = element.attrib.get("type", "UNDEFINED_TYPE")

        min_version = version_tracker.track_element(
            cls.__name__, id, cc_version, f"{cc_name}:{command_name}")

        optionaloffs = None
        optionalmask = None
        if element.attrib.get("optionaloffs"):
            optionaloffs = int(element.attrib.get("optionaloffs"), 16)
        if element.attrib.get("optionalmask"):
            optionalmask = int(element.attrib.get("optionalmask"), 16)

        for child in element:
            if child.tag == "const":
                fields.append(Const.from_xml_element(
                    child, version_tracker, cc_name, command_name, name, cc_version))
            elif child.tag == "bitflag":
                fields.append(BitFlag.from_xml_element(
                    child, version_tracker, cc_name, command_name, name, cc_version))
            elif child.tag == "bitfield":
                fields.append(BitField.from_xml_element(
                    child, version_tracker, cc_name, command_name, name, cc_version))
            elif child.tag == "bitmask":
                fields.append(BitMask.from_xml_element(
                    child, version_tracker, cc_name, command_name, name, cc_version))
            elif child.tag == "fieldenum":
                fields.append(FieldEnum.from_xml_element(
                    child, version_tracker, cc_name, command_name, name, cc_version))
            elif child.tag == "variant":
                fields.append(Variant.from_xml_element(
                    child, version_tracker, cc_name, command_name, name, cc_version))
            elif child.tag == "multi_array":
                fields.append(MultiArray.from_xml_element(
                    child, version_tracker, cc_name, command_name, name, cc_version))
            elif child.tag == "arrayattrib":
                fields.append(ArrayAttrib.from_xml_element(
                    child, version_tracker, cc_name, command_name, name, cc_version))

        return cls(
            name=name,
            id=id,
            type=type,
            min_version=min_version,
            optionaloffs=optionaloffs,
            optionalmask=optionalmask,
            fields=fields
        )
