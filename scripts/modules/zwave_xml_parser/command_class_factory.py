import logging
from typing import List, Dict, Any
from modules.zwave_xml_parser.xml_element_version_tracker import XMLElementVersionTracker
from modules.zwave_xml_parser.xml_element_commandclass import CommandClass
from collections import defaultdict


class CommandClassFactory:
    @staticmethod
    def from_xml_tree(tree, supported_command_classes: List[Dict[str, Any]]) -> List[CommandClass]:
        xml_elements = list(tree.iter())

        # - Create a map of supported command classes by name for fast lookup
        #   Also map deprecated names so renamed CCs (e.g. ALARM -> NOTIFICATION)
        #   have their older XML versions included in the frame size arrays.
        supported_command_class_map = {}
        for cc in supported_command_classes:
            supported_command_class_map[cc["name"]] = cc
            for dep_name in cc.get("deprecated_names", []):
                supported_command_class_map[dep_name] = cc

        # - Group all command classes by name and filter out the ones that are not supported
        # - Each group contains all versions of the same command class
        supported_command_class_groups = defaultdict(list)
        for elm in xml_elements:
            if elm.tag == "cmd_class":
                id = elm.attrib.get("key")
                name = elm.attrib.get("name")
                if name in supported_command_class_map:
                    supported_command_class_groups[id].append(elm)

        # - Parse the XML tree to get the command classes and filter out the ones that are higher than the supported version
        # - Calculate frame sizes for each command in a given command class
        # - Pick the latest version of a command class in a given command class group
        command_classes = {}
        for cc_group in supported_command_class_groups.values():
            cc_group.sort(key=lambda x: int(x.attrib.get("version")))

            version_tracker = XMLElementVersionTracker()
            cc_name = cc_group[0].attrib.get("name")
            supported_version = supported_command_class_map[cc_name]["version"]

            command_versions = {}
            for cc in cc_group:
                current_cc_version = int(cc.attrib.get("version"))
                if current_cc_version > supported_version:
                    break

                command_class = CommandClass.from_xml_element(
                    cc, version_tracker, supported_command_class_map[cc_name])

                for command in command_class.commands:
                    if command.id not in command_versions:
                        command_versions[command.id] = {}
                    command_versions[command.id][current_cc_version] = command

                    max_frame_sizes = []
                    min_frame_sizes = []
                    for version in range(1, supported_version + 1):
                        if version in command_versions[command.id]:
                            max_frame_size = command_versions[command.id][version].calculate_frame_size_for_version()
                            min_frame_size = command_versions[command.id][version].calculate_frame_min_size_for_version()
                            max_frame_sizes.append(max_frame_size)
                            min_frame_sizes.append(min_frame_size)
                        else:
                            # Mark as 0 if command doesn't exist in this version
                            max_frame_sizes.append(0)
                            min_frame_sizes.append(0)
                    command.max_frame_sizes = max_frame_sizes
                    command.min_frame_sizes = min_frame_sizes

                # Keep track of the latest version for each command class ID
                cc_id = command_class.id
                if cc_id not in command_classes:
                    # Add the command class to the list if it is not already in the list
                    command_classes[cc_id] = command_class
                else:
                    # Update if this version is newer
                    current_latest_version = int(
                        command_classes[cc_id].supported_version or 0)
                    new_version = int(command_class.supported_version or 0)
                    if new_version > current_latest_version:
                        command_classes[cc_id] = command_class

        return list(command_classes.values())
