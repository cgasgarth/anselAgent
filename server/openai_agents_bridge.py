from __future__ import annotations

# pyright: reportAttributeAccessIssue=false

import json
import threading
import time
from typing import Any, cast

from agents import Agent, ModelSettings, RunConfig, Runner, function_tool
from openai.types.shared import Reasoning

from shared.canonical_plan import CanonicalEditAction
from shared.protocol import AgentPlan, JsonObject, PlannedOperationDraft, RequestEnvelope

from .codex_bridge.canonical_binder import bind_canonical_plan
from .codex_bridge.config import (
    _DEFAULT_MAX_TOOL_CALLS_WITHOUT_APPLY,
    _DEFAULT_MODEL,
    _DEFAULT_REASONING_EFFORT,
    _THREAD_DEVELOPER_INSTRUCTIONS,
    _TOOL_APPLY_OPERATIONS,
    _TOOL_GET_IMAGE_STATE,
    _TOOL_GET_PLAYBOOK,
    _TOOL_GET_PREVIEW_IMAGE,
    logger,
)
from .codex_bridge.errors import CodexAppServerError
from .codex_bridge.intent_router import load_playbook
from .codex_bridge.models import ActiveRequestState, CodexTurnResult, TurnContext
from .codex_bridge.operations import OperationsMixin
from .codex_bridge.prompting import PromptingMixin
from .codex_bridge.request_state import RequestStateMixin
from .codex_bridge.tool_routing import ToolRoutingMixin
from .codex_bridge.turns import TurnsMixin
from .codex_bridge.verifier import VerifierMixin


class OpenAIAgentsBridge(
    OperationsMixin,
    PromptingMixin,
    VerifierMixin,
    ToolRoutingMixin,
    TurnsMixin,
    RequestStateMixin,
):
    def __init__(self, *, timeout_seconds: float = 600.0) -> None:
        self._timeout_seconds = timeout_seconds
        self._lock = threading.Lock()
        self._state_lock = threading.Lock()
        self._conversation_threads: dict[str, str] = {}
        self._conversation_turn_counts: dict[str, int] = {}
        self._conversation_histories: dict[str, list[str]] = {}
        self._active_requests = {}
        self._cancelled_requests = {}
        self._turn_contexts = {}

    def plan(self, request: RequestEnvelope) -> CodexTurnResult:
        request = self._sanitize_request_for_agent_safety(request)
        active_request = self._register_request(request)
        started_at = time.monotonic()
        try:
            with self._lock:
                self._raise_if_cancelled_locked(active_request)
                model = self._model_for_request(request)
                effort = self._effort_for_request(request)
                thread_id = self._get_or_create_agents_thread(
                    request.session.conversationId
                )
                active_request.thread_id = thread_id
                turn_id = f"agents-{request.requestId}"
                active_request.codex_turn_id = turn_id
                self._conversation_turn_counts[active_request.conversation_id] = (
                    self._conversation_turn_counts.get(
                        active_request.conversation_id, 0
                    )
                    + 1
                )
                turn_index = self._conversation_turn_counts[
                    active_request.conversation_id
                ]

                self._set_active_request_status_locked(
                    request.requestId,
                    status="running",
                    message="Running OpenAI Agents planner",
                )
                preview_data_url = self._preview_data_url(request)
                self._register_turn_context(thread_id, turn_id, request, preview_data_url)
                context = self._get_turn_context(thread_id, turn_id)
                if context is None:
                    raise CodexAppServerError(
                        "agents_context_unavailable",
                        "OpenAI Agents planner could not build image context",
                    )
                self._raise_if_cancelled_locked(active_request)

                turn_input = self._agents_input_from_turn_items(
                    self._build_turn_input(request, preview_data_url=preview_data_url)
                )
                prompt_text_chars = sum(
                    len(str(content_item.get("text", "")))
                    for item in turn_input
                    for content_item in cast(list[JsonObject], item.get("content", []))
                    if content_item.get("type") == "input_text"
                )
                image_input_chars = sum(
                    len(str(content_item.get("image_url", "")))
                    for item in turn_input
                    for content_item in cast(list[JsonObject], item.get("content", []))
                    if content_item.get("type") == "input_image"
                )

                agent = self._build_agent(
                    model=model,
                    effort=effort,
                    context=context,
                    thread_id=thread_id,
                    turn_id=turn_id,
                )
                run_config = self._build_run_config(model=model, effort=effort)
                result = Runner.run_sync(
                    agent,
                    cast(Any, turn_input),
                    run_config=run_config,
                    max_turns=max(context.max_tool_calls + 3, 10),
                )
                self._raise_if_cancelled_locked(active_request)
                plan = self._coerce_agent_plan(result.final_output)
                if request.refinement.enabled:
                    plan = bind_canonical_plan(request, plan)
                plan = self._finalize_plan_with_live_context(plan, context)
                raw_message = plan.model_dump_json()
                self._set_active_request_status_locked(
                    active_request.request_id,
                    status="completed",
                    message="OpenAI Agents plan completed",
                )
                self._remember_turn_summary(
                    active_request=active_request,
                    turn_index=turn_index,
                    plan=plan,
                )
                duration_ms = int((time.monotonic() - started_at) * 1000)
                logger.info(
                    "openai_agents_turn_completed",
                    extra={
                        "structured": {
                            "requestId": active_request.request_id,
                            "conversationId": active_request.conversation_id,
                            "threadId": thread_id,
                            "turnId": turn_id,
                            "turnIndexInConversation": turn_index,
                            "durationMs": duration_ms,
                            "promptTextChars": prompt_text_chars,
                            "imageInputChars": image_input_chars,
                            "model": model,
                            "effort": effort,
                        }
                    },
                )
                return CodexTurnResult(
                    plan=plan,
                    thread_id=thread_id,
                    turn_id=turn_id,
                    raw_message=raw_message,
                )
        except CodexAppServerError as exc:
            self._set_active_request_status_locked(
                request.requestId,
                status="failed",
                message=exc.message,
            )
            logger.error(
                "openai_agents_plan_failed",
                extra={
                    "structured": {
                        "requestId": request.requestId,
                        "conversationId": request.session.conversationId,
                        "threadId": active_request.thread_id,
                        "turnId": active_request.codex_turn_id,
                        "code": exc.code,
                        "message": exc.message,
                        "statusCode": exc.status_code,
                    }
                },
            )
            raise
        except Exception as exc:
            message = f"OpenAI Agents planner failed: {exc}"
            self._set_active_request_status_locked(
                request.requestId,
                status="failed",
                message=message,
            )
            logger.exception(
                "openai_agents_plan_unexpected_error",
                extra={
                    "structured": {
                        "requestId": request.requestId,
                        "conversationId": request.session.conversationId,
                        "threadId": active_request.thread_id,
                        "turnId": active_request.codex_turn_id,
                    }
                },
            )
            raise CodexAppServerError(
                "agents_run_failed", message, status_code=502
            ) from exc
        finally:
            if active_request.thread_id and active_request.codex_turn_id:
                self._clear_turn_context(
                    active_request.thread_id, active_request.codex_turn_id
                )
            self._unregister_request(request.requestId)

    def _get_or_create_agents_thread(self, conversation_id: str) -> str:
        existing = self._conversation_threads.get(conversation_id)
        if existing:
            return existing
        thread_id = f"agents-thread-{len(self._conversation_threads) + 1}"
        self._conversation_threads[conversation_id] = thread_id
        return thread_id

    @staticmethod
    def _agents_input_from_turn_items(items: list[JsonObject]) -> list[JsonObject]:
        content: list[JsonObject] = []
        for item in items:
            item_type = item.get("type")
            if item_type == "text":
                content.append({"type": "input_text", "text": str(item.get("text", ""))})
            elif item_type == "image":
                content.append(
                    {"type": "input_image", "image_url": str(item.get("url", ""))}
                )
        return [{"role": "user", "content": content}]

    def _build_agent(
        self,
        *,
        model: str | None,
        effort: str,
        context: TurnContext,
        thread_id: str,
        turn_id: str,
    ) -> Agent[Any]:
        return Agent(
            name="Ansel image editing planner",
            instructions=_THREAD_DEVELOPER_INSTRUCTIONS,
            model=model,
            model_settings=self._model_settings(effort),
            output_type=AgentPlan,
            tools=self._build_tools(context, thread_id=thread_id, turn_id=turn_id),
        )

    @staticmethod
    def _build_run_config(*, model: str | None, effort: str) -> RunConfig:
        return RunConfig(
            model=model,
            model_settings=OpenAIAgentsBridge._model_settings(effort),
            workflow_name="anselAgent",
            trace_metadata={"component": "ansel-agent-backend"},
        )

    @staticmethod
    def _model_settings(effort: str) -> ModelSettings:
        return ModelSettings(
            reasoning=Reasoning(effort=cast(Any, effort)), include_usage=True
        )

    def _build_tools(
        self, context: TurnContext, *, thread_id: str, turn_id: str
    ) -> list[Any]:
        bridge = self

        @function_tool(name_override=_TOOL_GET_IMAGE_STATE)
        def get_image_state() -> str:
            """Return current image state JSON for planning."""
            bridge._register_sdk_tool_call(context, _TOOL_GET_IMAGE_STATE, thread_id, turn_id)
            return json.dumps(context.state_payload, separators=(",", ":"))

        @function_tool(name_override=_TOOL_GET_PREVIEW_IMAGE)
        def get_preview_image() -> str:
            """Return the current rendered preview image as a data URL."""
            bridge._register_sdk_tool_call(
                context, _TOOL_GET_PREVIEW_IMAGE, thread_id, turn_id
            )
            return context.preview_data_url

        @function_tool(name_override=_TOOL_GET_PLAYBOOK)
        def get_playbook(playbookId: str) -> str:
            """Fetch one planning playbook by id."""
            bridge._register_sdk_tool_call(context, _TOOL_GET_PLAYBOOK, thread_id, turn_id)
            return json.dumps(load_playbook(playbookId), separators=(",", ":"))

        @function_tool(name_override=_TOOL_APPLY_OPERATIONS)
        def apply_operations(
            operations: list[PlannedOperationDraft] | None = None,
            canonicalActions: list[CanonicalEditAction] | None = None,
        ) -> str:
            """Apply Ansel edits in the live run and return render feedback."""
            bridge._register_sdk_tool_call(
                context, _TOOL_APPLY_OPERATIONS, thread_id, turn_id
            )
            arguments: JsonObject = {}
            if operations is not None:
                arguments["operations"] = [
                    operation.model_dump(mode="json") for operation in operations
                ]
            if canonicalActions is not None:
                arguments["canonicalActions"] = [
                    action.model_dump(mode="json") for action in canonicalActions
                ]
            response = bridge._apply_operations_tool_call(
                context, arguments, thread_id=thread_id, turn_id=turn_id
            )
            return json.dumps(response, separators=(",", ":"))

        return [get_image_state, get_preview_image, get_playbook, apply_operations]

    def _register_sdk_tool_call(
        self, context: TurnContext, tool_name: str, thread_id: str, turn_id: str
    ) -> None:
        with self._state_lock:
            guardrail_error = self._register_tool_call_progress_locked(
                context, tool_name
            )
            tool_calls_used = context.tool_calls_used
            max_tool_calls = context.max_tool_calls
        if guardrail_error is not None:
            raise CodexAppServerError(
                "agents_tool_guardrail", guardrail_error, status_code=422
            )
        self._set_active_request_status_for_turn_locked(
            thread_id,
            turn_id,
            status="running",
            message=f"Handled tool {tool_name} ({tool_calls_used}/{max_tool_calls})",
            last_tool_name=tool_name,
        )

    @staticmethod
    def _coerce_agent_plan(final_output: Any) -> AgentPlan:
        if isinstance(final_output, AgentPlan):
            return final_output
        if isinstance(final_output, str):
            return AgentPlan.model_validate_json(final_output)
        if isinstance(final_output, dict):
            return AgentPlan.model_validate(final_output)
        raise CodexAppServerError(
            "agents_invalid_response",
            "OpenAI Agents planner did not return an AgentPlan",
            status_code=502,
        )

    def _remember_turn_summary(
        self,
        *,
        active_request: ActiveRequestState,
        turn_index: int,
        plan: AgentPlan,
    ) -> None:
        summary_text = plan.assistantText or ""
        if len(summary_text) > 200:
            summary_text = summary_text[:200] + "..."
        turn_summary = f"Turn {turn_index}: {len(plan.operations)} operations. {summary_text}"
        conv_id = active_request.conversation_id
        self._conversation_histories.setdefault(conv_id, []).append(turn_summary)
        self._conversation_histories[conv_id] = self._conversation_histories[conv_id][
            -10:
        ]

    def _reset_process_locked(self) -> None:
        return None
