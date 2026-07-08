from dataclasses import dataclass, field
from typing import List, Union
from xml.etree.ElementTree import Element
from modules.zwave_xml_parser.xml_element_version_tracker import XMLElementVersionTracker
from modules.zwave_xml_parser.xml_element_param import Param
from modules.zwave_xml_parser.xml_element_paramgroup import ParamGroup
from modules.zwave_xml_parser.xml_element_utils import XmlElementUtils


@dataclass
class Command:
    name: str | None
    id: int | None
    min_version: int | None = None
    max_frame_sizes: List[int] = field(default_factory=list)
    min_frame_sizes: List[int] = field(default_factory=list)
    params: List[Union[Param, ParamGroup]] = field(default_factory=list)
    support_mode: str | None = None

    def is_tx(self) -> bool:
        """True when the node sends this command (received by ZPC).

        Mirrors utils.is_tx in the Jinja templates. Determines whether a command
        is incoming TX for parsing, doc generation, and has_mqtt_interface_doc.

        Runtime MQTT publish is separately gated by the Jinja macro
        should_mqtt_publish_report, which composes is_tx with the
        suppressed_mqtt_report_commands denylist in config.j2.
        """
        if self.support_mode in ("TX", "TX_RX"):
            return True
        if self.support_mode is not None:
            return False
        if not self.name:
            return False
        name_lower = self.name.lower()
        return name_lower.endswith(("_report", "_notification"))

    @classmethod
    def from_xml_element(cls, element: Element, version_tracker: XMLElementVersionTracker, cc_name: str, cc_version: int) -> 'Command':
        params = []
        max_frame_sizes = []
        id = int(element.attrib.get("key", "0x00"), 16)
        name = element.attrib.get("name", "UNDEFINED_CMD_NAME")
        support_mode = element.attrib.get("support_mode", None)

        min_version = version_tracker.track_element(
            cls.__name__, id, cc_version, cc_name)

        for child in element:
            if child.tag == "param":
                params.append(Param.from_xml_element(
                    child, version_tracker, cc_name, name, cc_version))
            elif child.tag == "variant_group":
                params.append(ParamGroup.from_xml_element(
                    child, version_tracker, cc_name, name, cc_version))

        return cls(
            name=name,
            id=id,
            min_version=min_version,
            max_frame_sizes=max_frame_sizes,
            params=params,
            support_mode=support_mode
        )

    def calculate_frame_size_for_version(self) -> int:
        total_size = 0
        current_frame_size = 0
        has_variant_group = False

        for param in self.params:
            if isinstance(param, Param):
                param_size = XmlElementUtils.calculate_param_max_size(param)
                if param_size == 0:
                    if current_frame_size > 0:
                        total_size += current_frame_size
                        current_frame_size = 0
                    total_size += param_size
                else:
                    current_frame_size += param_size
            elif isinstance(param, ParamGroup):
                has_variant_group = True
                if current_frame_size > 0:
                    total_size += current_frame_size
                    current_frame_size = 0
                group_size = XmlElementUtils.calculate_paramgroup_max_size(param)
                total_size += group_size

        if current_frame_size > 0:
            total_size += current_frame_size

        total_size += 2 # CommandClass.ID + Command.ID

        # Add 255 to the maximum size if there is a variant_group
        if has_variant_group:
            total_size += 255

        # Cap the total size at 255
        return min(total_size, 255)

    def calculate_frame_min_size_for_version(self) -> int:
        total_size = 0
        current_frame_size = 0

        for param in self.params:
            if isinstance(param, Param):
                param_size = XmlElementUtils.calculate_param_min_size(param)
                if param_size == 0:
                    if current_frame_size > 0:
                        total_size += current_frame_size
                        current_frame_size = 0
                    total_size += param_size
                else:
                    current_frame_size += param_size
            elif isinstance(param, ParamGroup):
                if current_frame_size > 0:
                    total_size += current_frame_size
                    current_frame_size = 0
                group_size = XmlElementUtils.calculate_paramgroup_min_size(param)
                total_size += group_size

        if current_frame_size > 0:
            total_size += current_frame_size

        total_size += 2 # CommandClass.ID + Command.ID
        return total_size
