from __future__ import annotations

from dataclasses import dataclass
from typing import Any

from agents import Agent, RunConfig

from server.openai_agents_bridge import OpenAIAgentsBridge
from server.tests.test_api import _sample_request_payload
from shared.protocol import AgentPlan, RequestEnvelope


def _sample_request_payload_with_preview() -> dict[str, object]:
    payload = _sample_request_payload()
    payload["imageSnapshot"]["preview"] = {
        "previewId": "preview-1",
        "mimeType": "image/png",
        "width": 1,
        "height": 1,
        "base64Data": "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mP8z8BQDwAFgwJ/lzQzWQAAAABJRU5ErkJggg==",
    }
    return payload


def _sample_request() -> RequestEnvelope:
    return RequestEnvelope.model_validate(_sample_request_payload_with_preview())


@dataclass
class _FakeRunResult:
    final_output: Any


def _sample_plan() -> AgentPlan:
    return AgentPlan.model_validate(
        {
            "assistantText": "Increasing exposure by +0.7 EV.",
            "continueRefining": False,
            "operations": [
                {
                    "operationId": "op-exposure-plus-0.7",
                    "sequence": 1,
                    "kind": "set-float",
                    "target": {
                        "type": "ansel-action",
                        "actionPath": "iop/exposure/exposure",
                        "settingId": "setting.exposure.primary",
                    },
                    "value": {"mode": "delta", "number": 0.7},
                    "reason": None,
                    "constraints": {
                        "onOutOfRange": "clamp",
                        "onRevisionMismatch": "fail",
                    },
                }
            ],
        }
    )


def test_openai_agents_bridge_runs_agent_with_structured_output(
    monkeypatch,
) -> None:
    captured: dict[str, Any] = {}
    expected_plan = _sample_plan()

    def fake_run_sync(
        agent: Agent[Any],
        input: list[dict[str, object]],
        *,
        run_config: RunConfig,
        max_turns: int,
    ) -> _FakeRunResult:
        captured["agent"] = agent
        captured["input"] = input
        captured["run_config"] = run_config
        captured["max_turns"] = max_turns
        return _FakeRunResult(final_output=expected_plan)

    monkeypatch.setattr("server.openai_agents_bridge.Runner.run_sync", fake_run_sync)

    bridge = OpenAIAgentsBridge()
    result = bridge.plan(_sample_request())

    assert result.plan == expected_plan
    assert result.thread_id.startswith("agents-thread-")
    assert result.turn_id == "agents-req-1"
    agent = captured["agent"]
    assert agent.model == "gpt-5.5"
    assert agent.output_type is AgentPlan
    assert [tool.name for tool in agent.tools] == [
        "get_image_state",
        "get_preview_image",
        "get_playbook",
        "apply_operations",
    ]
    assert captured["max_turns"] >= 10
    assert captured["input"][0]["role"] == "user"
    content = captured["input"][0]["content"]
    assert any(item["type"] == "input_text" for item in content)


def test_openai_agents_bridge_reuses_conversation_thread(monkeypatch) -> None:
    def fake_run_sync(
        agent: Agent[Any],
        input: list[dict[str, object]],
        *,
        run_config: RunConfig,
        max_turns: int,
    ) -> _FakeRunResult:
        del agent, input, run_config, max_turns
        return _FakeRunResult(final_output=_sample_plan())

    monkeypatch.setattr("server.openai_agents_bridge.Runner.run_sync", fake_run_sync)

    bridge = OpenAIAgentsBridge()
    first = bridge.plan(_sample_request())
    second_payload = _sample_request_payload_with_preview()
    second_payload["requestId"] = "req-2"
    second_payload["session"]["turnId"] = "turn-2"
    second = bridge.plan(RequestEnvelope.model_validate(second_payload))

    assert second.thread_id == first.thread_id
    assert second.turn_id == "agents-req-2"
