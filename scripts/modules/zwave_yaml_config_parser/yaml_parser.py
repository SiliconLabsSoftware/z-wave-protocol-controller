import yaml
from typing import List, IO, Optional, Dict, Any


class YAMLConfigParser:
    def __init__(self, yaml_file: IO[str]):
        self._yaml_file = yaml_file
        self._config: Optional[dict] = None
        self._opened = False

    def _open_yaml(self) -> None:
        if self._opened:
            return

        if not hasattr(self._yaml_file, 'read'):
            raise ValueError("YAML read failed")

        self._yaml_file.seek(0)
        self._config = yaml.safe_load(self._yaml_file)
        self._opened = True

    def get_supported_command_classes(self) -> List[Dict[str, Any]]:
        self._open_yaml()

        if not self._config:
            return []

        value = self._config.get('supported_command_classes', [])
        if not isinstance(value, list):
            return []
        return value

    def get_optional_param_overrides(self) -> List[Dict[str, Any]]:
        self._open_yaml()

        if not self._config:
            return []

        return self._config.get('optional_param_overrides', [])
