from __future__ import annotations

from collections import Counter
import json
from typing import cast

from shared.protocol import JsonObject


def coerce_validation_body_to_object(body: object) -> JsonObject | None:
    if isinstance(body, dict):
        return body
    if isinstance(body, (bytes, bytearray)):
        try:
            parsed = json.loads(body.decode("utf-8"))
        except (UnicodeDecodeError, json.JSONDecodeError):
            return None
        return parsed if isinstance(parsed, dict) else None
    if isinstance(body, str):
        try:
            parsed = json.loads(body)
        except json.JSONDecodeError:
            return None
        return parsed if isinstance(parsed, dict) else None
    return None


def extract_duplicate_capability_ids(body: object) -> list[str]:
    payload = coerce_validation_body_to_object(body)
    if payload is None:
        return []

    manifest = payload.get("capabilityManifest")
    if not isinstance(manifest, dict):
        return []
    manifest = cast(dict[str, object], manifest)

    targets = manifest.get("targets")
    if not isinstance(targets, list):
        return []

    capability_ids: list[str] = []
    for target in targets:
        if not isinstance(target, dict):
            continue
        target = cast(dict[str, object], target)
        capability_id = target.get("capabilityId")
        if isinstance(capability_id, str) and capability_id:
            capability_ids.append(capability_id)

    counts = Counter(capability_ids)
    return sorted(capability_id for capability_id, count in counts.items() if count > 1)
