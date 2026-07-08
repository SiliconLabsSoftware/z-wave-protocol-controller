import xml.etree.ElementTree as ET
from typing import IO, Optional, Any
from lxml import etree


class XMLParser:
    def __init__(self, xml_file: IO[str], xsd_file: IO[str]):
        self._xml_file = xml_file
        self._xsd_file = xsd_file
        self._xsd_schema: Optional[etree.XMLSchema] = None
        self._xml_doc: Optional[etree.ElementTree] = None
        self._tree: Optional[Any] = None
        self._loaded = False

    def _open_xsd(self) -> etree.XMLSchema:
        if hasattr(self._xsd_file, 'read'):
            self._xsd_file.seek(0)
            schema_root = etree.XML(self._xsd_file.read())
            return etree.XMLSchema(schema_root)
        raise ValueError("XSD read failed")

    def _open_xml(self) -> etree._ElementTree:
        if hasattr(self._xml_file, 'read'):
            self._xml_file.seek(0)
            xml_doc = etree.parse(self._xml_file)
            return xml_doc
        raise ValueError("XML read failed")

    def _load_xml(self) -> None:
        if self._loaded:
            return

        try:
            self._xsd_schema = self._open_xsd()
            self._xml_doc = self._open_xml()

            if self._xsd_schema is None or self._xml_doc is None or not self._xsd_schema.validate(self._xml_doc):
                raise ValueError("XML validation failed")

            self._xml_file.seek(0)
            self._tree = ET.parse(self._xml_file)

            if self._tree is None or self._tree.getroot() is None:
                raise ValueError("XML root element not found")

            self._loaded = True

        except Exception:
            raise

    def load_tree(self) -> ET.ElementTree:
        self._load_xml()
        if self._tree is None:
            raise RuntimeError("XML tree not found")
        return self._tree
