from __future__ import annotations

from collections.abc import Callable, Sequence
from dataclasses import dataclass
from typing import cast
from shared.canonical_plan import CanonicalEditAction
from shared.protocol import JsonObject

from .canonical_binder import bind_canonical_actions
from .models import TurnContext


@dataclass(frozen=True, slots=True)
class PreparedApplyBatch:
    normalized_batch: list[JsonObject]
    render_warnings: list[str]


def prepare_apply_batch(
    context: TurnContext,
    arguments: JsonObject,
    *,
    normalize_operation: Callable[[JsonObject, int], tuple[JsonObject, str | None]],
) -> tuple[PreparedApplyBatch | None, str | None]:
    raw_operations = arguments.get("operations")
    raw_canonical_actions = arguments.get("canonicalActions")
    if raw_operations is None:
        raw_operations = []
    if raw_canonical_actions is None:
        raw_canonical_actions = []
    if not isinstance(raw_operations, list):
        return None, "apply_operations operations must be an array."
    if not isinstance(raw_canonical_actions, list):
        return None, "apply_operations canonicalActions must be an array."
    if not raw_operations and not raw_canonical_actions:
        return None, "apply_operations requires operations and/or canonicalActions."

    warnings: list[str] = []
    combined_operations: list[JsonObject] = []
    if raw_canonical_actions:
        canonical_batch, canonical_error = _prepare_canonical_batch(
            context, raw_canonical_actions
        )
        if canonical_error:
            return None, canonical_error
        assert canonical_batch is not None
        combined_operations.extend(canonical_batch.normalized_batch)
        warnings.extend(canonical_batch.render_warnings)
    if raw_operations:
        raw_batch, raw_error = _prepare_raw_batch(
            context,
            raw_operations,
            normalize_operation,
            sequence_base=context.next_operation_sequence + len(combined_operations),
        )
        if raw_error:
            return None, raw_error
        assert raw_batch is not None
        combined_operations.extend(raw_batch.normalized_batch)
        warnings.extend(raw_batch.render_warnings)
    _renumber_batch_operations(combined_operations, context.next_operation_sequence)
    return PreparedApplyBatch(combined_operations, warnings), None


def _prepare_canonical_batch(
    context: TurnContext, raw_canonical_actions: Sequence[object]
) -> tuple[PreparedApplyBatch | None, str | None]:
    canonical_actions: list[CanonicalEditAction] = []
    for raw_action in raw_canonical_actions:
        try:
            canonical_actions.append(CanonicalEditAction.model_validate(raw_action))
        except Exception as exc:
            return None, f"canonicalActions entry failed schema validation: {exc}"

    binding_result = bind_canonical_actions(
        list(context.base_request.imageSnapshot.editableSettings),
        canonical_actions,
    )
    if binding_result.failures and not binding_result.operations:
        return None, "; ".join(binding_result.failures)
    if not binding_result.operations:
        return (
            None,
            "apply_operations could not bind any supported operations from canonicalActions.",
        )
    warnings = []
    if binding_result.failures:
        warnings.append("Binding notes: " + "; ".join(binding_result.failures))
    return PreparedApplyBatch(binding_result.operations, warnings), None


def _prepare_raw_batch(
    context: TurnContext,
    raw_operations: Sequence[object],
    normalize_operation: Callable[[JsonObject, int], tuple[JsonObject, str | None]],
    *,
    sequence_base: int,
) -> tuple[PreparedApplyBatch | None, str | None]:
    normalized_batch: list[JsonObject] = []
    for index, raw_operation in enumerate(raw_operations):
        if not isinstance(raw_operation, dict):
            return None, "Every apply_operations entry must be an object."
        normalized_operation, error = normalize_operation(
            cast(JsonObject, raw_operation),
            sequence_base + index,
        )
        if error:
            return None, error
        normalized_batch.append(normalized_operation)
    return PreparedApplyBatch(normalized_batch, []), None


def _renumber_batch_operations(
    operations: list[JsonObject], start_sequence: int
) -> None:
    seen_operation_ids: set[str] = set()
    for index, operation in enumerate(operations):
        candidate_operation_id = str(
            operation.get("operationId") or f"apply-op-{start_sequence + index}"
        )
        operation_id = candidate_operation_id
        duplicate_index = 2
        while operation_id in seen_operation_ids:
            operation_id = f"{candidate_operation_id}-{duplicate_index}"
            duplicate_index += 1
        seen_operation_ids.add(operation_id)
        operation["operationId"] = operation_id
        operation["sequence"] = start_sequence + index
