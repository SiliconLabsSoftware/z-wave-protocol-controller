from pathlib import Path
import logging
import argparse
import sys
import os
from pprint import pprint


current_dir = os.path.dirname(os.path.abspath(__file__))
scripts_dir = os.path.abspath(os.path.join(current_dir, '..'))
if scripts_dir not in sys.path:
    sys.path.insert(0, scripts_dir)

logging.basicConfig(level=logging.ERROR, format='%(levelname)s: %(message)s')

try:
    from modules.zwave_xml_parser.command_class_factory import CommandClassFactory
    from modules.zwave_yaml_config_parser.yaml_parser import YAMLConfigParser
    from modules.zwave_xml_parser.xml_parser import XMLParser
    from modules.zwave_xml_parser.xml_element_param import Param
    from modules.zwave_command_class_generator.command_class_generator import CommandClassGenerator
except ImportError as e:
    print("Make sure you're running this script from the correct directory or install the dependencies")
    raise e

def main():
    parser = argparse.ArgumentParser(description='Z-Wave XML Validator')
    parser.add_argument('--xml', type=argparse.FileType('rb'),
                        required=True, help='XML file path')
    parser.add_argument('--xsd', type=argparse.FileType('rb'),
                        required=True, help='XSD file path')
    parser.add_argument('--cfg', type=argparse.FileType('r'),
                        required=True, help='YAML configuration file path')
    parser.add_argument('--templates', type=str, required=True,
                        help='Jinja2 templates directory path')
    parser.add_argument('--out', type=str, help='Output directory path')
    args = parser.parse_args()

    zwave_yaml_config = YAMLConfigParser(args.cfg)
    xml_parser = XMLParser(args.xml, args.xsd)
    zwave_xml = xml_parser.load_tree()

    supported_command_classes = zwave_yaml_config.get_supported_command_classes()
    command_classes = CommandClassFactory.from_xml_tree(
        zwave_xml, supported_command_classes)

    # Apply optional param overrides from config (for params missing optionaloffs/optionalmask in zwave.xml)
    for override in zwave_yaml_config.get_optional_param_overrides():
        cc_name = override.get("command_class", "").upper()
        cmd_name = override.get("command", "").upper()
        param_name = override.get("param", "")
        optionaloffs = override.get("optionaloffs")
        optionalmask = override.get("optionalmask")
        if not all([cc_name, cmd_name, param_name, optionaloffs is not None, optionalmask is not None]):
            continue
        for cc in command_classes:
            if cc.name and cc.name.upper() != cc_name:
                continue
            for cmd in cc.commands:
                if cmd.name and cmd.name.upper() != cmd_name:
                    continue
                for p in cmd.params:
                    if isinstance(p, Param) and p.name and p.name.upper() == param_name.upper():
                        p.optionaloffs = int(optionaloffs, 16)
                        p.optionalmask = int(optionalmask, 16)
                        break
                break
            break

    #########################################################
    # Generate command class types
    #########################################################
    if Path(args.templates).exists():
        generator = CommandClassGenerator()
        generator.load_templates(Path(args.templates))
        generator.load_command_classes(command_classes)

        if args.out:
            generator.output_dir = Path(args.out)

        generator.generate_sources()
    else:
        logging.error(f"Templates directory {args.templates} does not exist")
        sys.exit(1)


if __name__ == "__main__":
    main()
