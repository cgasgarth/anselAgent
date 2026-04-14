from __future__ import annotations

# pyright: reportAttributeAccessIssue=false

import copy
from collections.abc import Sequence
from typing import cast

from shared.protocol import AgentPlan, JsonObject

from .apply_batch import prepare_apply_batch
from .config import _TOOL_APPLY_OPERATIONS, logger
from .models import TurnContext
from .operation_support import (
    apply_operation_to_settings,
    extract_error_message,
    is_white_balance_action_path,
    normalize_graph_operation,
    setting_ids_for_action_path,
    white_balance_action_paths,
    white_balance_operation_rank,
)


class OperationsMixin:
    def _apply_operations_tool_call(
        self,
        context: TurnContext,
        arguments: JsonObject,
        *,
        thread_id: str | None = None,
        turn_id: str | None = None,
    ) -> JsonObject:
        if not context.live_run_enabled:
            return self._tool_error_response(
                "apply_operations is only available when live run mode is enabled."
            )

        prepared_batch, prepare_error = prepare_apply_batch(
            context,
            arguments,
            normalize_operation=lambda raw_operation,
            sequence_number: self._normalize_tool_operation(
                context,
                raw_operation,
                sequence_number=sequence_number,
            ),
        )
        if prepare_error:
            attempted = arguments.get("operations")
            if not isinstance(attempted, list):
                attempted = []
            self._log_white_balance_tool_call(
                context,
                attempted,
                [],
                success=False,
                error=prepare_error,
            )
            return self._tool_error_response(prepare_error)

        assert prepared_batch is not None
        ordered_batch = self._order_operations_for_apply(
            prepared_batch.normalized_batch
        )
        attempted_operations_for_logging = ordered_batch
        render_warnings = prepared_batch.render_warnings
        simulated_settings = copy.deepcopy(context.setting_by_id)
        for operation in ordered_batch:
            apply_error, _ = self._apply_operation_to_settings(
                simulated_settings,
                operation,
            )
            if apply_error:
                self._log_white_balance_tool_call(
                    context,
                    attempted_operations_for_logging,
                    [],
                    success=False,
                    error=apply_error,
                )
                return self._tool_error_response(apply_error)
        applied_batch: list[JsonObject] = []
        step_summaries: list[str] = []
        latest_preview_url: str | None = None
        latest_verifier_result: JsonObject | None = None

        for step_index, operation in enumerate(ordered_batch, start=1):
            apply_error = self._apply_live_operation_step(context, operation)
            if apply_error:
                self._log_white_balance_tool_call(
                    context,
                    attempted_operations_for_logging,
                    applied_batch,
                    success=False,
                    error=apply_error,
                )
                return self._tool_error_response(apply_error)

            applied_batch.append(operation)
            step_summary = self._summarize_live_operation(context, operation)
            step_summaries.append(step_summary)
            context.last_applied_summary = step_summary

            if thread_id and turn_id:
                self._set_active_request_status_for_turn_locked(
                    thread_id,
                    turn_id,
                    status="running",
                    message=(
                        f"Applied live edit step {step_index}/{len(ordered_batch)}: "
                        f"{step_summary}"
                    ),
                    last_tool_name=_TOOL_APPLY_OPERATIONS,
                )

            preview_url, verifier_result, warning = self._wait_for_live_render(context)
            if preview_url is not None:
                latest_preview_url = preview_url
            if verifier_result is not None:
                latest_verifier_result = verifier_result
            if warning is not None:
                render_warnings.append(warning)
        self._log_white_balance_tool_call(
            context,
            attempted_operations_for_logging,
            applied_batch,
            success=True,
        )

        content_items: list[JsonObject] = [
            {
                "type": "inputText",
                "text": (
                    f"Applied {len(applied_batch)} operations stepwise in this call; "
                    f"{len(context.applied_operations)} total live edits applied. "
                    f"Steps: {'; '.join(step_summaries)}. "
                    "Refreshed preview image included below."
                ),
            }
        ]
        if latest_preview_url is not None:
            content_items.append(
                {
                    "type": "inputImage",
                    "imageUrl": latest_preview_url,
                }
            )
        for warning in render_warnings:
            content_items.append({"type": "inputText", "text": warning})
        if latest_verifier_result is not None:
            content_items.append(
                {
                    "type": "inputText",
                    "text": self._verifier_feedback_text(latest_verifier_result),
                }
            )
        return {"success": True, "contentItems": content_items}

    def _apply_live_operation_step(
        self,
        context: TurnContext,
        operation: JsonObject,
    ) -> str | None:
        apply_error, _ = self._apply_operation_to_settings(
            context.setting_by_id,
            operation,
            graph_property_by_setting_id=context.graph_property_by_setting_id,
        )
        if apply_error:
            return apply_error

        context.applied_operations.append(operation)
        context.next_operation_sequence += 1
        context.last_applied_batch = [operation]
        image_snapshot = context.state_payload.get("imageSnapshot")
        if isinstance(image_snapshot, dict):
            image_snapshot["imageRevisionId"] = (
                f"{context.base_image_revision_id}:tool-{len(context.applied_operations)}"
            )
        self._refresh_preview_after_operations(context)
        context.requires_render_callback = True
        context.render_event.clear()
        context.rendered_preview_bytes = None
        return None

    def _wait_for_live_render(
        self, context: TurnContext
    ) -> tuple[str | None, JsonObject | None, str | None]:
        logger.info(
            "waiting_for_mid_turn_render",
            extra={
                "structured": {
                    "threadId": context.base_request.session.conversationId,
                    "turnId": context.base_request.session.turnId,
                }
            },
        )
        render_arrived = context.render_event.wait(timeout=15.0)
        context.requires_render_callback = False
        rendered_bytes = context.rendered_preview_bytes
        context.rendered_preview_bytes = None

        if render_arrived and rendered_bytes:
            context.preview_mime_type = "image/jpeg"
            context.current_preview_bytes = rendered_bytes
            context.preview_data_url = self._build_data_url(
                "image/jpeg",
                rendered_bytes,
                revision_token=str(len(context.applied_operations)),
            )
            verifier_result = self._build_live_verifier_feedback(context)
            return context.preview_data_url, verifier_result, None

        warning = "Warning: mid-turn render timed out. The preview image may be stale."
        logger.warning(
            "mid_turn_render_timeout",
            extra={
                "structured": {
                    "conversationId": context.base_request.session.conversationId,
                    "turnId": context.base_request.session.turnId,
                }
            },
        )
        return None, None, warning

    def _summarize_live_operation(
        self,
        context: TurnContext,
        operation: JsonObject,
    ) -> str:
        target = operation.get("target")
        target_dict: JsonObject = (
            cast(JsonObject, target) if isinstance(target, dict) else {}
        )
        action_path = str(target_dict.get("actionPath") or "unknown")
        setting_id = str(target_dict.get("settingId") or "")
        if (
            target_dict.get("type") == "graph-control"
            and isinstance(target_dict.get("nodeId"), str)
            and isinstance(target_dict.get("propertyPath"), str)
        ):
            graph_node_id = cast(str, target_dict["nodeId"])
            graph_property_path = cast(str, target_dict["propertyPath"])
            graph_ref = f"{graph_node_id}:{graph_property_path}"
            resolved_setting_id = context.graph_property_ref_to_setting_id.get(
                graph_ref
            )
            if resolved_setting_id:
                setting_id = resolved_setting_id
        setting = context.setting_by_id.get(setting_id) or {}
        module_label = str(setting.get("moduleLabel") or "")
        control_label = str(setting.get("label") or action_path.rsplit("/", 1)[-1])
        label = " / ".join(part for part in (module_label, control_label) if part)
        if not label:
            label = action_path

        value = operation.get("value")
        if not isinstance(value, dict):
            return label
        value_dict = cast(JsonObject, value)

        kind = operation.get("kind")
        if kind == "set-float":
            number = value_dict.get("number")
            mode = value_dict.get("mode")
            if isinstance(number, (int, float)):
                if mode == "delta":
                    return f"{label} {float(number):+0.3f}"
                return f"{label} = {float(number):0.3f}"
        if kind == "set-choice":
            choice_id = value_dict.get("choiceId")
            choice_value = value_dict.get("choiceValue")
            if isinstance(choice_id, str) and choice_id:
                return f"{label} -> {choice_id}"
            if isinstance(choice_value, int):
                return f"{label} -> choice {choice_value}"
        if kind == "set-bool":
            bool_value = value_dict.get("boolValue")
            if isinstance(bool_value, bool):
                return f"{label} -> {'on' if bool_value else 'off'}"
        return label

    @staticmethod
    def _clamp(value: float, minimum: float, maximum: float) -> float:
        return max(minimum, min(maximum, value))

    def _refresh_preview_after_operations(self, context: TurnContext) -> None:
        pass

    def _normalize_tool_operation(
        self,
        context: TurnContext,
        raw_operation: JsonObject,
        *,
        sequence_number: int,
    ) -> tuple[JsonObject, str | None]:
        for key in ("kind", "target", "value"):
            if key not in raw_operation:
                return {}, f"operation is missing required member '{key}'"

        target = raw_operation.get("target")
        if not isinstance(target, dict):
            return {}, "operation target must be an object"
        target_dict = cast(JsonObject, target)

        if target_dict.get("type") == "graph-control":
            normalized_graph_operation, graph_error = self._normalize_graph_operation(
                context,
                raw_operation,
                sequence_number=sequence_number,
            )
            if graph_error is not None:
                return {}, graph_error
            assert normalized_graph_operation is not None
            return normalized_graph_operation, None

        operation_id = raw_operation.get("operationId")
        if not isinstance(operation_id, str) or not operation_id:
            operation_id = f"tool-op-{sequence_number}"

        operation_candidate = {
            "operationId": operation_id,
            "sequence": sequence_number,
            "kind": raw_operation["kind"],
            "target": raw_operation["target"],
            "value": raw_operation["value"],
            "reason": raw_operation.get("reason"),
            "constraints": raw_operation.get(
                "constraints",
                {"onOutOfRange": "clamp", "onRevisionMismatch": "fail"},
            ),
        }

        try:
            validated = AgentPlan.model_validate(
                {
                    "assistantText": "tool staging",
                    "continueRefining": False,
                    "operations": [operation_candidate],
                }
            ).operations[0]
        except Exception as exc:
            return {}, f"operation failed schema validation: {exc}"

        operation = validated.model_dump(mode="json")
        target = operation.get("target")
        if not isinstance(target, dict):
            return {}, "operation target must be an object"

        setting_id = target.get("settingId")
        action_path = target.get("actionPath")
        if not isinstance(setting_id, str):
            return {}, f"operation targets unknown settingId '{setting_id}'"
        if setting_id not in context.setting_by_id:
            if isinstance(action_path, str):
                matches = self._setting_ids_for_action_path(
                    context.setting_by_id, action_path
                )
                if len(matches) == 1:
                    target["settingId"] = matches[0]
                    return operation, None
            return {}, f"operation targets unknown settingId '{setting_id}'"
        return operation, None

    def _normalize_graph_operation(
        self,
        context: TurnContext,
        raw_operation: JsonObject,
        *,
        sequence_number: int,
    ) -> tuple[JsonObject | None, str | None]:
        return normalize_graph_operation(
            context, raw_operation, sequence_number=sequence_number
        )

    @staticmethod
    def _setting_ids_for_action_path(
        setting_by_id: dict[str, JsonObject],
        action_path: str,
    ) -> list[str]:
        return setting_ids_for_action_path(setting_by_id, action_path)

    @classmethod
    def _white_balance_operation_rank(cls, operation: JsonObject) -> tuple[int, str]:
        return white_balance_operation_rank(operation)

    def _order_operations_for_apply(
        self, operations: list[JsonObject]
    ) -> list[JsonObject]:
        ordered = list(operations)
        wb_indexes = [
            index
            for index, operation in enumerate(operations)
            if is_white_balance_action_path(
                str(operation.get("target", {}).get("actionPath") or "")
            )
        ]
        if len(wb_indexes) < 2:
            return ordered
        wb_operations = [operations[index] for index in wb_indexes]
        wb_operations.sort(key=self._white_balance_operation_rank)
        for index, operation in zip(wb_indexes, wb_operations, strict=False):
            ordered[index] = operation
        return ordered

    def _log_white_balance_tool_call(
        self,
        context: TurnContext,
        attempted_operations: Sequence[object],
        applied_operations: Sequence[object],
        *,
        success: bool,
        error: str | None = None,
    ) -> None:
        attempted_paths = white_balance_action_paths(list(attempted_operations))
        applied_paths = white_balance_action_paths(list(applied_operations))
        if not attempted_paths and not applied_paths:
            return

        logger.info(
            "apply_operations_white_balance",
            extra={
                "structured": {
                    "requestId": context.base_request.requestId,
                    "conversationId": context.base_request.session.conversationId,
                    "tool": _TOOL_APPLY_OPERATIONS,
                    "success": success,
                    "attemptedWhiteBalanceActionPaths": attempted_paths,
                    "appliedWhiteBalanceActionPaths": applied_paths,
                    "error": error,
                }
            },
        )

    def _apply_operation_to_settings(
        self,
        setting_by_id: dict[str, JsonObject],
        operation: JsonObject,
        *,
        graph_property_by_setting_id: dict[str, JsonObject] | None = None,
    ) -> tuple[str | None, JsonObject | None]:
        return apply_operation_to_settings(
            setting_by_id,
            operation,
            graph_property_by_setting_id=graph_property_by_setting_id,
        )

    @staticmethod
    def _extract_error_message(message: str) -> str:
        return extract_error_message(message)
