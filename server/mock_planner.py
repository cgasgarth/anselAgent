from __future__ import annotations

import json
import os
import re

from server.bridge_types import RequestProgressPayload
from server.codex_app_server import CodexTurnResult
from shared.protocol import AgentPlan, RequestEnvelope

_EXACT_EV_RE = re.compile(r"([+-]?\d+(?:\.\d+)?)\s*ev\b", re.IGNORECASE)


def _pick_exposure_setting(request: RequestEnvelope) -> tuple[str, str] | None:
    for setting in request.imageSnapshot.editableSettings:
        if (
            setting.actionPath == "iop/exposure/exposure"
            and setting.kind == "set-float"
        ):
            return setting.actionPath, setting.settingId
    return None


def _infer_goal_delta(request: RequestEnvelope) -> float:
    text = f"{request.refinement.goalText}\n{request.message.text}"
    match = _EXACT_EV_RE.search(text)
    if match:
        return float(match.group(1))

    lowered = text.lower()
    if (
        "darken" in lowered
        or "lower exposure" in lowered
        or "reduce exposure" in lowered
    ):
        return -0.7

    return 0.7


def _configured_operations() -> list[dict]:
    raw = os.environ.get("ANSEL_AGENT_TEST_MOCK_OPERATIONS_JSON", "").strip()
    if not raw:
        return []

    payload = json.loads(raw)
    if not isinstance(payload, list):
        raise ValueError("ANSEL_AGENT_TEST_MOCK_OPERATIONS_JSON must be a JSON array")
    return [item for item in payload if isinstance(item, dict)]


def _matches_selector(setting, selector: dict) -> bool:
    module_ids = selector.get("moduleIds")
    if isinstance(module_ids, list) and module_ids:
        if setting.moduleId not in {
            value for value in module_ids if isinstance(value, str)
        }:
            return False

    module_label_contains = selector.get("moduleLabelContains")
    if isinstance(module_label_contains, str) and module_label_contains:
        if module_label_contains.lower() not in setting.moduleLabel.lower():
            return False

    label_contains = selector.get("labelContains")
    if isinstance(label_contains, str) and label_contains:
        if label_contains.lower() not in setting.label.lower():
            return False

    action_path = selector.get("actionPath")
    if isinstance(action_path, str) and action_path:
        if setting.actionPath != action_path:
            return False

    kind = selector.get("kind")
    if isinstance(kind, str) and kind:
        if setting.kind != kind:
            return False

    return True


def _find_setting(request: RequestEnvelope, selector: dict):
    for setting in request.imageSnapshot.editableSettings:
        if _matches_selector(setting, selector):
            return setting
    return None


def _operation_value(config: dict, *, kind: str) -> dict:
    value = config.get("value")
    if not isinstance(value, dict):
        raise ValueError("configured mock operation is missing a value object")

    normalized = dict(value)
    if "mode" not in normalized and kind in {"set-choice", "set-bool"}:
        normalized["mode"] = "set"
    return normalized


def _configured_plan(request: RequestEnvelope) -> AgentPlan | None:
    configured = _configured_operations()
    if not configured:
        return None

    if request.refinement.enabled and request.refinement.passIndex > 1:
        return AgentPlan.model_validate(
            {
                "assistantText": f"Mock pass {request.refinement.passIndex}: verified configured settings.",
                "continueRefining": False,
                "operations": [],
            }
        )

    operations: list[dict] = []
    for index, item in enumerate(configured, start=1):
        selector = item.get("selector")
        if not isinstance(selector, dict):
            raise ValueError("configured mock operation is missing a selector object")

        setting = _find_setting(request, selector)
        if setting is None:
            raise ValueError(
                f"configured mock operation selector did not match any editable setting: {selector}"
            )

        operations.append(
            {
                "operationId": f"mock-configured-{request.refinement.passIndex}-{index}",
                "sequence": index,
                "kind": setting.kind,
                "target": {
                    "type": "ansel-action",
                    "actionPath": setting.actionPath,
                    "settingId": setting.settingId,
                },
                "value": _operation_value(item, kind=setting.kind),
                "reason": item.get("reason")
                or "Deterministic configured smoke-test operation.",
                "constraints": {
                    "onOutOfRange": "clamp",
                    "onRevisionMismatch": "fail",
                },
            }
        )

    return AgentPlan.model_validate(
        {
            "assistantText": f"Mock configured edit: applying {len(operations)} settings.",
            "continueRefining": request.refinement.enabled
            and request.refinement.maxPasses > 1,
            "operations": operations,
        }
    )


class MockPlannerBridge:
    def plan(self, request: RequestEnvelope) -> CodexTurnResult:
        configured_plan = _configured_plan(request)
        if configured_plan is not None:
            return CodexTurnResult(
                plan=configured_plan,
                thread_id=f"mock-thread-{request.session.conversationId}",
                turn_id=f"mock-turn-{request.session.turnId}",
                raw_message=configured_plan.model_dump_json(),
            )

        exposure_target = _pick_exposure_setting(request)
        if not exposure_target:
            plan = AgentPlan.model_validate(
                {
                    "assistantText": "Mock planner could not find an exposure control for this image.",
                    "continueRefining": False,
                    "operations": [],
                }
            )
            return CodexTurnResult(
                plan=plan,
                thread_id=f"mock-thread-{request.session.conversationId}",
                turn_id=f"mock-turn-{request.session.turnId}",
                raw_message=plan.model_dump_json(),
            )

        action_path, setting_id = exposure_target
        total_delta = _infer_goal_delta(request)

        if request.refinement.enabled:
            if request.refinement.passIndex == 1:
                delta = round(total_delta * 0.6, 2)
                continue_refining = request.refinement.maxPasses > 1
                assistant_text = f"Mock pass 1: starting with {delta:+.2f} EV."
            else:
                delta = round(total_delta - round(total_delta * 0.6, 2), 2)
                continue_refining = False
                assistant_text = f"Mock pass {request.refinement.passIndex}: finishing with {delta:+.2f} EV."
        else:
            delta = round(total_delta, 2)
            continue_refining = False
            assistant_text = f"Mock single-turn edit: applying {delta:+.2f} EV."

        plan = AgentPlan.model_validate(
            {
                "assistantText": assistant_text,
                "continueRefining": continue_refining,
                "operations": [
                    {
                        "operationId": f"mock-exposure-{request.refinement.passIndex}",
                        "sequence": 1,
                        "kind": "set-float",
                        "target": {
                            "type": "ansel-action",
                            "actionPath": action_path,
                            "settingId": setting_id,
                        },
                        "value": {"mode": "delta", "number": delta},
                        "reason": "Deterministic smoke-test mock response.",
                        "constraints": {
                            "onOutOfRange": "clamp",
                            "onRevisionMismatch": "fail",
                        },
                    }
                ],
            }
        )
        return CodexTurnResult(
            plan=plan,
            thread_id=f"mock-thread-{request.session.conversationId}",
            turn_id=f"mock-turn-{request.session.turnId}",
            raw_message=plan.model_dump_json(),
        )

    def cancel_request(
        self,
        *,
        request_id: str,
        app_session_id: str,
        image_session_id: str,
        conversation_id: str,
        turn_id: str,
        reason: str | None = None,
    ) -> bool:
        del request_id
        del app_session_id
        del image_session_id
        del conversation_id
        del turn_id
        del reason
        return False

    def get_request_progress(
        self,
        *,
        request_id: str,
        app_session_id: str,
        image_session_id: str,
        conversation_id: str,
        turn_id: str,
    ) -> RequestProgressPayload:
        del request_id
        del app_session_id
        del image_session_id
        del conversation_id
        del turn_id
        return {
            "found": False,
            "status": "not_found",
            "message": "No active request found for that requestId.",
            "toolCallsUsed": 0,
            "maxToolCalls": 0,
            "appliedOperationCount": 0,
            "operations": [],
            "lastToolName": None,
            "progressVersion": 0,
            "requiresRenderCallback": False,
        }

    def provide_render_callback(
        self,
        *,
        image_session_id: str,
        turn_id: str,
        image_bytes: bytes,
    ) -> bool:
        _, _, _ = image_session_id, turn_id, image_bytes
        return False
