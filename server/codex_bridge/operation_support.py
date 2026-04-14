from __future__ import annotations

import json
from typing import cast

from shared.protocol import AgentPlan, JsonObject

from .config import _WHITE_BALANCE_ACTION_PATH_PREFIXES
from .models import TurnContext


def normalize_graph_operation(
    context: TurnContext,
    raw_operation: JsonObject,
    *,
    sequence_number: int,
) -> tuple[JsonObject | None, str | None]:
    target = raw_operation.get("target")
    value = raw_operation.get("value")
    if not isinstance(target, dict):
        return None, "graph operation target must be an object"
    if not isinstance(value, dict):
        return None, "graph operation value must be an object"

    target_dict = cast(JsonObject, target)
    graph_id = target_dict.get("graphId")
    node_id = target_dict.get("nodeId")
    property_path = target_dict.get("propertyPath")
    if graph_id != "edit-pipeline":
        return None, f"unknown graphId '{graph_id}'"
    if not isinstance(node_id, str) or not node_id:
        return None, "graph operation target requires nodeId"
    if not isinstance(property_path, str) or not property_path:
        return None, "graph operation target requires propertyPath"

    graph_ref = f"{node_id}:{property_path}"
    setting_id = context.graph_property_ref_to_setting_id.get(graph_ref)
    if setting_id is None:
        return None, f"graph property '{graph_ref}' is not editable"

    setting = context.setting_by_id.get(setting_id)
    if not isinstance(setting, dict):
        return None, f"graph property '{graph_ref}' resolved to unknown setting"

    normalized_target: JsonObject = {
        "type": "ansel-action",
        "settingId": setting_id,
        "actionPath": setting.get("actionPath"),
    }
    normalized_operation = {
        "operationId": raw_operation.get("operationId")
        or f"graph-op-{sequence_number}",
        "sequence": sequence_number,
        "kind": setting.get("kind")
        if raw_operation.get("kind") == "set-graph-prop"
        else raw_operation.get("kind"),
        "target": normalized_target,
        "value": value,
        "reason": raw_operation.get("reason"),
        "constraints": raw_operation.get(
            "constraints",
            {"onOutOfRange": "clamp", "onRevisionMismatch": "fail"},
        ),
    }

    try:
        validated = AgentPlan.model_validate(
            {
                "assistantText": "graph tool staging",
                "continueRefining": False,
                "operations": [normalized_operation],
            }
        ).operations[0]
    except Exception as exc:
        return None, f"graph operation failed schema validation: {exc}"

    return validated.model_dump(mode="json"), None


def setting_ids_for_action_path(
    setting_by_id: dict[str, JsonObject], action_path: str
) -> list[str]:
    return [
        setting_id
        for setting_id, setting in setting_by_id.items()
        if setting.get("actionPath") == action_path
    ]


def choice_mapping(setting: JsonObject) -> dict[int, str]:
    choices = setting.get("choices")
    mapping: dict[int, str] = {}
    if not isinstance(choices, list):
        return mapping
    for choice in choices:
        if not isinstance(choice, dict):
            continue
        choice_dict = cast(JsonObject, choice)
        value = choice_dict.get("choiceValue")
        choice_id = choice_dict.get("choiceId")
        if isinstance(value, int) and isinstance(choice_id, str) and choice_id:
            mapping[value] = choice_id
    return mapping


def is_white_balance_action_path(action_path: str) -> bool:
    return any(
        action_path.startswith(prefix) for prefix in _WHITE_BALANCE_ACTION_PATH_PREFIXES
    )


def white_balance_operation_rank(operation: JsonObject) -> tuple[int, str]:
    target = operation.get("target")
    action_path = (
        cast(JsonObject, target).get("actionPath") if isinstance(target, dict) else None
    )
    if not isinstance(action_path, str):
        return (99, "")
    leaf = action_path.rsplit("/", 1)[-1].lower()
    kind = operation.get("kind")
    if kind == "set-bool":
        return (0, leaf)
    if kind == "set-choice":
        return (1, leaf)
    if leaf == "finetune":
        return (2, leaf)
    if leaf == "temperature":
        return (3, leaf)
    if leaf == "tint":
        return (4, leaf)
    channel_order = {
        "red": 5,
        "green": 6,
        "blue": 7,
        "emerald": 8,
        "yellow": 9,
        "various": 9,
    }
    return (channel_order.get(leaf, 99), leaf)


def white_balance_action_paths(operations: list[object]) -> list[str]:
    paths: list[str] = []
    for operation in operations:
        if not isinstance(operation, dict):
            continue
        operation_dict = cast(JsonObject, operation)
        target = operation_dict.get("target")
        if not isinstance(target, dict):
            continue
        action_path = cast(JsonObject, target).get("actionPath")
        if isinstance(action_path, str) and is_white_balance_action_path(action_path):
            paths.append(action_path)
    return paths


def apply_operation_to_settings(
    setting_by_id: dict[str, JsonObject],
    operation: JsonObject,
    *,
    graph_property_by_setting_id: dict[str, JsonObject] | None = None,
) -> tuple[str | None, JsonObject | None]:
    target = operation.get("target")
    if not isinstance(target, dict):
        return "operation target must be an object", None
    target_dict = cast(JsonObject, target)

    setting_id = target_dict.get("settingId")
    action_path = target_dict.get("actionPath")
    if not isinstance(setting_id, str) or not isinstance(action_path, str):
        return "operation target requires settingId and actionPath", None

    setting = setting_by_id.get(setting_id)
    if not isinstance(setting, dict):
        return f"unknown settingId '{setting_id}'", None
    if setting.get("actionPath") != action_path:
        return (
            f"actionPath mismatch for settingId '{setting_id}': expected {setting.get('actionPath')}, got {action_path}",
            None,
        )

    kind = operation.get("kind")
    if setting.get("kind") != kind:
        return f"kind mismatch for settingId '{setting_id}'", None
    value = operation.get("value")
    if not isinstance(value, dict):
        return "operation value must be an object", None
    value_dict = cast(JsonObject, value)

    mode = value_dict.get("mode")
    supported_modes = setting.get("supportedModes")
    if not isinstance(mode, str):
        return "operation value requires mode", None
    if isinstance(supported_modes, list) and mode not in supported_modes:
        return f"mode '{mode}' is not supported by settingId '{setting_id}'", None

    graph_property = (
        graph_property_by_setting_id.get(setting_id)
        if graph_property_by_setting_id is not None
        else None
    )

    if kind == "set-float":
        number_value = value_dict.get("number")
        if not isinstance(number_value, (int, float)):
            return (
                f"set-float operation requires numeric value.number for '{setting_id}'",
                None,
            )
        current = setting.get("currentNumber")
        if not isinstance(current, (int, float)):
            current = setting.get("defaultNumber")
        if not isinstance(current, (int, float)):
            current = 0.0
        requested_number = float(number_value)
        resolved_number = (
            float(current) + requested_number if mode == "delta" else requested_number
        )
        next_value = resolved_number
        min_number = setting.get("minNumber")
        max_number = setting.get("maxNumber")
        if isinstance(min_number, (int, float)):
            next_value = max(next_value, float(min_number))
        if isinstance(max_number, (int, float)):
            next_value = min(next_value, float(max_number))
        setting["currentNumber"] = next_value
        if isinstance(graph_property, dict):
            graph_property["currentNumber"] = next_value
        return None, {
            "actionPath": action_path,
            "settingId": setting_id,
            "kind": kind,
            "mode": mode,
            "requestedNumber": requested_number,
            "resolvedNumber": resolved_number,
            "appliedNumber": next_value,
            "wasClamped": abs(next_value - resolved_number) > 1e-12,
        }

    if kind == "set-choice":
        choice_value = value_dict.get("choiceValue")
        if not isinstance(choice_value, int):
            return (
                f"set-choice operation requires integer value.choiceValue for '{setting_id}'",
                None,
            )
        mapping = choice_mapping(setting)
        if mapping and choice_value not in mapping:
            return f"choiceValue {choice_value} is not valid for '{setting_id}'", None
        choice_id = value_dict.get("choiceId")
        if isinstance(choice_id, str) and mapping.get(choice_value) not in {
            None,
            choice_id,
        }:
            expected_choice_id = mapping.get(choice_value)
            return (
                f"choiceId mismatch for '{setting_id}': expected {expected_choice_id}, got {choice_id}",
                None,
            )
        setting["currentChoiceValue"] = choice_value
        if choice_value in mapping:
            setting["currentChoiceId"] = mapping[choice_value]
        if isinstance(graph_property, dict):
            graph_property["currentChoiceValue"] = choice_value
            graph_property["currentChoiceId"] = setting.get("currentChoiceId")
        return None, {
            "actionPath": action_path,
            "settingId": setting_id,
            "kind": kind,
            "mode": mode,
            "requestedChoiceValue": choice_value,
            "appliedChoiceValue": choice_value,
            "appliedChoiceId": setting.get("currentChoiceId"),
        }

    if kind == "set-bool":
        bool_value = value_dict.get("boolValue")
        if not isinstance(bool_value, bool):
            return (
                f"set-bool operation requires boolean value.boolValue for '{setting_id}'",
                None,
            )
        setting["currentBool"] = bool_value
        if isinstance(graph_property, dict):
            graph_property["currentBool"] = bool_value
        return None, {
            "actionPath": action_path,
            "settingId": setting_id,
            "kind": kind,
            "mode": mode,
            "appliedBoolValue": bool_value,
        }

    return f"unsupported operation kind '{kind}'", None


def extract_error_message(message: str) -> str:
    try:
        payload = json.loads(message)
    except (TypeError, json.JSONDecodeError):
        return message
    if isinstance(payload, dict):
        error = payload.get("error")
        if isinstance(error, dict):
            nested = error.get("message")
            if isinstance(nested, str) and nested:
                return nested
    return message
