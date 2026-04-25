from __future__ import annotations

import json
from typing import Annotated, Any, Literal, cast

from pydantic import BaseModel, ConfigDict, Field, field_validator

from shared.canonical_plan import CanonicalEditAction
from shared.protocol import JsonObject, OperationConstraint, OperationTarget, OperationValue

from .intent_router import list_playbooks


class StrictToolModel(BaseModel):
    model_config = ConfigDict(extra="forbid")


class EmptyToolArguments(StrictToolModel):
    pass


class PlaybookToolArguments(StrictToolModel):
    playbookId: str = Field(min_length=1)


class GraphControlTarget(StrictToolModel):
    type: Literal["graph-control"]
    nodeId: str = Field(min_length=1)
    propertyPath: str = Field(min_length=1)


class RawToolOperation(StrictToolModel):
    operationId: str | None = Field(default=None, min_length=1)
    sequence: int | None = Field(default=None, ge=1)
    kind: Literal["set-float", "set-choice", "set-bool"]
    target: OperationTarget | GraphControlTarget
    value: OperationValue
    reason: str | None = None
    constraints: OperationConstraint | None = None


class ApplyOperationsToolArguments(StrictToolModel):
    operations: Annotated[list[RawToolOperation], Field(min_length=1)] | None = None
    canonicalActions: Annotated[list[CanonicalEditAction], Field(min_length=1)] | None = None

    @field_validator("operations", "canonicalActions", mode="before")
    @classmethod
    def _normalize_tool_list(cls, value: Any) -> Any:
        if value == []:
            return None
        if not isinstance(value, list):
            return value
        normalized: list[Any] = []
        for item in value:
            if isinstance(item, str):
                try:
                    normalized.append(json.loads(item))
                except json.JSONDecodeError:
                    normalized.append(item)
            else:
                normalized.append(item)
        return normalized


class ApplyOperationsToolSchema(StrictToolModel):
    operations: list[RawToolOperation] = Field(default_factory=list)
    canonicalActions: list[CanonicalEditAction] = Field(default_factory=list)


def strict_tool_schema(model: type[BaseModel], *, schema_model: type[BaseModel] | None = None) -> JsonObject:
    schema = (schema_model or model).model_json_schema()
    source_model = model
    if source_model is PlaybookToolArguments:
        properties = schema.get("properties")
        if isinstance(properties, dict):
            playbook = properties.get("playbookId")
            if isinstance(playbook, dict):
                playbook["enum"] = [entry.id for entry in list_playbooks()]
    _strip_unsupported_schema_keys(schema)
    return cast(JsonObject, schema)


def model_to_arguments(model: BaseModel) -> JsonObject:
    return cast(
        JsonObject,
        model.model_dump(mode="json", exclude_none=True, exclude_unset=True),
    )


def _strip_unsupported_schema_keys(node: Any) -> None:
    if isinstance(node, dict):
        node.pop("default", None)
        for value in node.values():
            _strip_unsupported_schema_keys(value)
    elif isinstance(node, list):
        for item in node:
            _strip_unsupported_schema_keys(item)
