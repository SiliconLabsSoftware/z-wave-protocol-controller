from modules.zwave_xml_parser.xml_element_param import Param
from modules.zwave_xml_parser.xml_element_paramgroup import ParamGroup
from modules.zwave_xml_parser.xml_element_variant import Variant

class XmlElementUtils:
    @staticmethod
    def get_type_size(param_type: str | None) -> int:
        if param_type is None:
            return 0

        type_sizes = {
            'BYTE': 1,
            'WORD': 2,
            'DWORD': 4,
            'BIT_24': 3,
            'STRUCT_BYTE': 1,
            'CONST': 1,
            'BITMASK': 255,
            'VARIANT': 0,  # Variable size, will be calculated separately `by calculate_param_max_size`
        }
        return type_sizes.get(param_type, 0)

    @staticmethod
    def calculate_param_max_size(param: Param) -> int:
        if param.type == 'VARIANT':
            max_size = 0
            for field in param.fields:
                if isinstance(field, Variant) and field.size_mask is not None:
                    max_size = field.size_mask
                    if field.size_mask == 0: # no mask
                        if field.size_change != 0:
                            max_size = (field.param_offset or 0) + (field.size_change or 0)
                        else:
                            max_size = field.param_offset or 0
            return max_size
        else:
            return XmlElementUtils.get_type_size(param.type)

    @staticmethod
    def calculate_param_min_size(param: Param) -> int:
        if param.type == 'VARIANT':
            # For minimum size calculation, VARIANT type should be counted as 1
            return 0
        elif param.type == 'BITMASK':
            return 0
        else:
            return XmlElementUtils.get_type_size(param.type)

    @staticmethod
    # TODO: Check this with COMMAND_CLASS_FIRMWARE_UPDATE_MD
    def calculate_paramgroup_max_size(param_group: ParamGroup) -> int:
        """Calculate the maximum size for a parameter group."""
        if param_group.size_mask is None or param_group.size_mask == 0:
            # No size information, calculate from individual params
            total_size = 0
            for param in param_group.params:
                total_size += XmlElementUtils.calculate_param_max_size(param)
            return total_size
        else:
            # The size_mask is applied as logical AND over the byte at paramOffs
            # The result represents the number of repetitions of the variable data
            size_mask = param_group.size_mask
            
            # Calculate the maximum possible value after applying the mask
            # For a full mask (0xFF), the maximum value is 255
            # For a partial mask, we need to find the maximum value that can pass through
            max_repetitions = 0
            for i in range(256):  # Check all possible byte values (0-255)
                masked_value = i & size_mask
                max_repetitions = max(max_repetitions, masked_value)
            
            # Calculate total size: number of repetitions * size per repetition
            size_per_repetition = 0
            for param in param_group.params:
                size_per_repetition += XmlElementUtils.calculate_param_max_size(param)
            
            return max_repetitions * size_per_repetition

    @staticmethod
    def calculate_paramgroup_min_size(param_group: ParamGroup) -> int:
        """Calculate the minimum size for a parameter group."""
        if param_group.size_mask is None or param_group.size_mask == 0:
            # No size information, calculate from individual params
            total_size = 0
            for param in param_group.params:
                total_size += XmlElementUtils.calculate_param_min_size(param)
            return total_size
        else:
            # The size_mask is applied as logical AND over the byte at paramOffs
            # The result represents the number of repetitions of the variable data
            size_mask = param_group.size_mask

            # Calculate the minimum possible value after applying the mask
            # The minimum value is always 0 (when the masked byte is 0)
            min_repetitions = 0

            # Calculate total size: number of repetitions * size per repetition
            size_per_repetition = 0
            for param in param_group.params:
                size_per_repetition += XmlElementUtils.calculate_param_min_size(param)

            return min_repetitions * size_per_repetition