#!/usr/bin/env python3
"""
Prepare documentation for MkDocs: merge command-class docs into docs/api/command_classes/.
MQTT API, Network Manager, and Device Interviewer are included automatically via
mkdocs-monorepo-plugin (!include in root mkdocs.yml).

If GENERATED_CC_DOCS is set (e.g. to build/command_class_docs), generated command-class
docs from the command_class_generator are merged in (index + per-CC mqtt_interface.md).

Run from repository root:
    python scripts/mkdocs/prepare_docs_for_mkdocs.py
"""
import os
from pathlib import Path
import re
import shutil

REPO_ROOT = Path(__file__).resolve().parent.parent.parent
DOCS_API_CC = REPO_ROOT / "docs" / "api" / "command_classes"
MKDOCS_YML = REPO_ROOT / "mkdocs.yml"
GENERATED_CC_NAV_START = "    # GENERATED_CC_NAV_START"
GENERATED_CC_NAV_END = "    # GENERATED_CC_NAV_END"

# command_class_xxx/doc/generated/xxx.md -> command_class_xxx/xxx.md (for mqtt_interface.md)
CC_DOC_PATTERN = re.compile(
    r"command_class_([a-z0-9_]+)/doc/generated/(command_class_\1_mqtt_interface\.md)",
    re.IGNORECASE,
)
# generated/foo_mqtt_interface.md -> foo_mqtt_interface.md (flattened per-CC overview links)
GENERATED_RELATIVE_LINK_PATTERN = re.compile(
    r"\]\(generated/(command_class_[a-z0-9_]+_mqtt_interface\.md)\)",
    re.IGNORECASE,
)
SOURCE_LINK_REWRITES = {
    "../components/mqtt_api/doc/": "api/mqtt_api/",
    "../components/network_manager/doc/": "api/network_manager/",
    "../components/device_interviewer/docs/": "api/device_interviewer/",
    "../components/network_monitor/doc/": "api/network_monitor/",
    "../components/ota/docs/": "api/ota/",
    "../components/command_classes/doc/generated/": "api/command_classes/",
    "../../components/mqtt_api/doc/": "../api/mqtt_api/",
    "../../components/network_manager/doc/": "../api/network_manager/",
    "../../components/device_interviewer/docs/": "../api/device_interviewer/",
    "../../network_manager/doc/": "../network_manager/",
    "../../device_interviewer/docs/": "../device_interviewer/",
    "../../network_monitor/doc/": "../network_monitor/",
    "../../ota/docs/": "../ota/",
    "../../command_classes/doc/generated/": "../command_classes/",
    "../../../docs/sequences/": "../../sequences/",
}


def on_page_markdown(markdown: str, page, **kwargs) -> str:
    """Translate source-relative links to paths in the staged MkDocs tree."""
    for source, published in SOURCE_LINK_REWRITES.items():
        markdown = markdown.replace(f"]({source}", f"]({published}")
    return markdown


def copy_md(src_dir: Path, dst_dir: Path) -> None:
    """Copy all .md files from src_dir to dst_dir."""
    if not src_dir.is_dir():
        return
    dst_dir.mkdir(parents=True, exist_ok=True)
    for f in src_dir.glob("*.md"):
        shutil.copy2(f, dst_dir / f.name)


# Expand common abbreviations for human-friendly nav titles
_CC_WORD_EXPANSIONS = {"grp": "Group", "info": "Info", "zwaveplus": "Z-Wave Plus"}


def _cc_dir_to_nav_title(subdir_name: str) -> str:
    """Convert command_class_foo_bar to human-friendly 'Foo Bar' for nav."""
    if subdir_name.startswith("command_class_"):
        name = subdir_name[len("command_class_"):]
    else:
        name = subdir_name
    words = name.replace("_", " ").split()
    return " ".join(_CC_WORD_EXPANSIONS.get(w.lower(), w).title() for w in words)


def _collect_existing_cc_paths(text: str, start_pos: int, end_pos: int) -> set[str]:
    """Extract command_class directory names already listed in the nav (outside the generated block)."""
    nav_outside = text[:start_pos] + text[end_pos:]
    return set(re.findall(r"api/command_classes/(command_class_[a-z0-9_]+)/", nav_outside))


def _generate_cc_nav_entries(skip_dirs: set[str] | None = None) -> str:
    """Build YAML nav lines for command classes not already in the nav."""
    lines = []
    for subdir in sorted(DOCS_API_CC.iterdir()):
        if not subdir.is_dir() or not subdir.name.startswith("command_class_"):
            continue
        if skip_dirs and subdir.name in skip_dirs:
            continue
        mqtt_doc = next(subdir.glob("*_mqtt_interface.md"), None)
        if mqtt_doc is None:
            continue
        rel_path = f"api/command_classes/{subdir.name}/{mqtt_doc.name}"
        title = _cc_dir_to_nav_title(subdir.name)
        lines.append(f'    - "{title}": {rel_path}')
    return "\n".join(lines) if lines else ""


def update_mkdocs_nav() -> None:
    """Replace content between GENERATED_CC_NAV markers (idempotent)."""
    if not MKDOCS_YML.exists():
        return
    text = MKDOCS_YML.read_text(encoding="utf-8")
    if GENERATED_CC_NAV_START not in text or GENERATED_CC_NAV_END not in text:
        return

    start_idx = text.index(GENERATED_CC_NAV_START)
    end_idx = text.index(GENERATED_CC_NAV_END)
    existing = _collect_existing_cc_paths(text, start_idx, end_idx)
    nav_block = _generate_cc_nav_entries(skip_dirs=existing)

    pattern = re.compile(
        re.escape(GENERATED_CC_NAV_START) + r".*?" + re.escape(GENERATED_CC_NAV_END),
        re.DOTALL,
    )
    if nav_block:
        replacement = f"{GENERATED_CC_NAV_START}\n{nav_block}\n{GENERATED_CC_NAV_END}"
    else:
        replacement = f"{GENERATED_CC_NAV_START}\n{GENERATED_CC_NAV_END}"
    new_text = pattern.sub(replacement, text)
    MKDOCS_YML.write_text(new_text, encoding="utf-8")


def rewrite_cc_doc_links() -> None:
    """Flatten doc/generated/ link targets in the MkDocs command-class tree."""
    if not DOCS_API_CC.is_dir():
        return
    for path in DOCS_API_CC.rglob("*.md"):
        text = path.read_text(encoding="utf-8")
        updated = CC_DOC_PATTERN.sub(r"command_class_\1/\2", text)
        updated = GENERATED_RELATIVE_LINK_PATTERN.sub(r"](\1)", updated)
        if updated != text:
            path.write_text(updated, encoding="utf-8")


def publish_mqtt_interface_index(index: Path) -> None:
    """Publish mqtt_interface.md at the flat MkDocs nav path."""
    shutil.copy2(index, DOCS_API_CC / "mqtt_interface.md")


def merge_generated_cc_docs(generated_dir: Path) -> None:
    """Copy generated command-class docs (index + per-CC mqtt_interface) into DOCS_API_CC."""
    if not generated_dir.is_dir():
        return
    index = generated_dir / "doc/generated/mqtt_interface.md"
    if index.exists():
        publish_mqtt_interface_index(index)
    for path in generated_dir.glob("command_class_*/doc/generated/*_mqtt_interface.md"):
        cc_name = path.relative_to(generated_dir).parts[0]
        dst = DOCS_API_CC / cc_name
        dst.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, dst / path.name)


def main() -> None:
    # Command classes: single merged tree (plugin can't merge many dirs into one)
    if DOCS_API_CC.exists():
        shutil.rmtree(DOCS_API_CC)
    DOCS_API_CC.mkdir(parents=True)

    cc_dir = REPO_ROOT / "components" / "command_classes"
    index = cc_dir / "doc/generated/mqtt_interface.md"
    if index.exists():
        publish_mqtt_interface_index(index)
    for subdir in sorted(cc_dir.glob("command_class_*")):
        doc_dir = subdir / "doc"
        if not doc_dir.is_dir():
            continue
        dst = DOCS_API_CC / subdir.name
        copy_md(doc_dir, dst)
        copy_md(doc_dir / "generated", dst)

    # Overlay generated command-class docs if present (e.g. from doc build CI)
    generated_cc = os.environ.get("GENERATED_CC_DOCS")
    if generated_cc:
        merge_generated_cc_docs(REPO_ROOT / generated_cc)

    # Rewrite links (doc/generated/ and generated/ -> flat MkDocs paths)
    rewrite_cc_doc_links()

    # Inject generated command class nav into mkdocs.yml
    update_mkdocs_nav()

    print(
        "Documentation prepared (command_classes + guide). "
        "MQTT API / Network Manager / Device Interviewer use monorepo plugin."
    )


if __name__ == "__main__":
    main()
