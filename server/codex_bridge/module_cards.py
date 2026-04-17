from __future__ import annotations

from functools import lru_cache
from pathlib import Path
from typing import Mapping, cast

import yaml

from shared.protocol import RequestEnvelope

_DATA_DIR = Path(__file__).resolve().parent / "data"
_MODULES_PATH = _DATA_DIR / "ansel_modules.yaml"
_RULES_PATH = _DATA_DIR / "module_selection_rules.yaml"


@lru_cache(maxsize=1)
def load_module_cards() -> dict[str, dict[str, object]]:
    payload = yaml.safe_load(_MODULES_PATH.read_text(encoding="utf-8"))
    modules = payload.get("modules", []) if isinstance(payload, dict) else []
    cards: dict[str, dict[str, object]] = {}
    for module in modules:
        if not isinstance(module, Mapping):
            continue
        module_dict = cast(dict[str, object], dict(module))
        module_id = module_dict.get("id")
        if isinstance(module_id, str) and module_id:
            cards[module_id] = module_dict
    return cards


@lru_cache(maxsize=1)
def load_module_selection_rules() -> dict[str, object]:
    payload = yaml.safe_load(_RULES_PATH.read_text(encoding="utf-8"))
    return payload if isinstance(payload, dict) else {}


def _canonical_module_id(module_id: str) -> str:
    rules = load_module_selection_rules()
    aliases = rules.get("module_aliases", [])
    if not isinstance(aliases, list):
        return module_id
    for alias in aliases:
        if not isinstance(alias, Mapping):
            continue
        alias_dict = cast(dict[str, object], dict(alias))
        canonical = alias_dict.get("canonical")
        if alias_dict.get("alias") == module_id and isinstance(canonical, str):
            return canonical
    return module_id


def _request_module_ids(request: RequestEnvelope) -> list[str]:
    module_ids: list[str] = []
    seen: set[str] = set()

    def add(module_id: str | None) -> None:
        if not module_id:
            return
        normalized = _canonical_module_id(module_id)
        if normalized in seen:
            return
        seen.add(normalized)
        module_ids.append(normalized)

    for capability in request.capabilityManifest.targets:
        add(capability.moduleId)
    for setting in request.imageSnapshot.editableSettings:
        add(setting.moduleId)
    for history_item in request.imageSnapshot.history:
        add(history_item.module)

    edit_graph = request.imageSnapshot.editGraph
    if isinstance(edit_graph, dict):
        nodes = edit_graph.get("nodes")
        if isinstance(nodes, list):
            for node in nodes:
                if isinstance(node, Mapping):
                    node_dict = cast(dict[str, object], dict(node))
                    module_id = node_dict.get("moduleId")
                    add(module_id if isinstance(module_id, str) else None)
    return module_ids


def render_module_policy_summary() -> str:
    rules = load_module_selection_rules()
    lines = ["Module selection policy:"]

    preferred_path = rules.get("preferred_scene_referred_path", [])
    if isinstance(preferred_path, list) and preferred_path:
        lines.append(
            "- Prefer modern scene-referred path: "
            + " -> ".join(str(item) for item in preferred_path)
            + "."
        )

    legacy_modules = rules.get("legacy_or_overlap_heavy", [])
    if isinstance(legacy_modules, list) and legacy_modules:
        lines.append(
            "- Treat these as legacy or overlap-heavy unless explicitly needed: "
            + ", ".join(str(item) for item in legacy_modules)
            + "."
        )

    diagnostic_modules = rules.get("diagnostic_modules", [])
    if isinstance(diagnostic_modules, list) and diagnostic_modules:
        lines.append(
            "- Diagnostic modules are analysis only, not edits: "
            + ", ".join(str(item) for item in diagnostic_modules)
            + "."
        )

    overlap_rules = rules.get("avoid_redundant_stacking", [])
    if isinstance(overlap_rules, list):
        rendered_overlap: list[str] = []
        for overlap_rule in overlap_rules[:4]:
            if not isinstance(overlap_rule, dict):
                continue
            modules = overlap_rule.get("modules")
            reason = overlap_rule.get("reason")
            if isinstance(modules, list) and modules and isinstance(reason, str):
                rendered_overlap.append(
                    f"{' + '.join(str(item) for item in modules)} ({reason})"
                )
        if rendered_overlap:
            lines.append(
                "- Avoid redundant stacks: " + "; ".join(rendered_overlap) + "."
            )

    return "\n".join(lines)


def render_relevant_module_cards(
    request: RequestEnvelope,
    *,
    max_cards: int = 18,
) -> str:
    cards = load_module_cards()
    lines = ["Relevant module cards:"]
    count = 0
    for module_id in _request_module_ids(request):
        card = cards.get(module_id)
        if card is None:
            continue
        display_name = str(card.get("display_name") or module_id)
        category = str(card.get("category") or "technical")
        stage = str(card.get("stage") or "scene_linear")
        output_type = str(card.get("output_type") or "technical")
        purpose = str(card.get("purpose") or "")
        use_when = card.get("when_to_use")
        avoid_when = card.get("avoid_when")
        guidance = str(card.get("agent_guidance") or "")
        use_summary = (
            ", ".join(str(item) for item in use_when[:2])
            if isinstance(use_when, list)
            else ""
        )
        avoid_summary = (
            ", ".join(str(item) for item in avoid_when[:2])
            if isinstance(avoid_when, list)
            else ""
        )
        line = f"- {module_id} / {display_name} [{category}, {stage}, {output_type}]: {purpose}"
        if use_summary:
            line += f" Use when: {use_summary}."
        if avoid_summary:
            line += f" Avoid when: {avoid_summary}."
        if guidance:
            line += f" Guidance: {guidance}"
        lines.append(line)
        count += 1
        if count >= max_cards:
            break
    if count == 0:
        return ""
    return "\n".join(lines)
