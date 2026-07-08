from typing import Dict

class XMLElementVersionTracker:   
    def __init__(self):
        self._element_versions: Dict[str, int] = {}
    
    def track_element(self, element_type: str, element_id: int, version: int, parent_context: str = "") -> int:
        key = f"{parent_context}:{element_type}:{element_id:02X}"
        
        if key not in self._element_versions:
            self._element_versions[key] = version
            return version
        else:
            return self._element_versions[key]