from __future__ import annotations

import re
from typing import cast

from shared.protocol import EditableSetting, JsonObject, RequestEnvelope


_INSTANCE_PATTERN = re.compile(r"\.instance\.(\d+)$")


def _instance_index(setting_id: str) -> int:
    match = _INSTANCE_PATTERN.search(setting_id)
    return int(match.group(1)) if match else 0


def _module_node_id(module_id: str, instance_index: int) -> str:
    return f"module:{module_id}:{instance_index}"


def _group_node_id(module_id: str, instance_index: int, group_key: str) -> str:
    return f"group:{module_id}:{instance_index}:{group_key}"


def _raw_property_path(setting: EditableSetting) -> str:
    return setting.actionPath.rsplit("/", 1)[-1]


def _module_semantic_type(module_id: str) -> str:
    return {
        "clipping": "crop-module",
        "crop": "crop-module",
        "temperature": "white-balance-module",
        "colorbalancergb": "color-grading-module",
        "channelmixerrgb": "channel-mixer-module",
        "splittoningrgb": "split-toning-module",
        "toneequal": "tone-equalizer-module",
        "toneequalizer": "tone-equalizer-module",
        "colorequal": "color-equalizer-module",
        "denoiseprofile": "noise-reduction-module",
        "filmicrgb": "tone-mapping-module",
        "exposure": "exposure-module",
    }.get(module_id, "module-instance")


def _group_spec(
    module_id: str, raw_property_path: str
) -> tuple[str, str, str, str, str]:
    if module_id in {"clipping", "crop"}:
        if raw_property_path == "cx":
            return ("rect", "crop-rectangle", "crop.rect", "Crop rectangle", "left")
        if raw_property_path == "cy":
            return ("rect", "crop-rectangle", "crop.rect", "Crop rectangle", "top")
        if raw_property_path == "cw":
            return ("rect", "crop-rectangle", "crop.rect", "Crop rectangle", "right")
        if raw_property_path == "ch":
            return ("rect", "crop-rectangle", "crop.rect", "Crop rectangle", "bottom")
        if raw_property_path == "angle":
            return (
                "rotation",
                "rotation-control",
                "crop.rotation",
                "Rotation",
                "angle",
            )
    if module_id == "temperature":
        if raw_property_path == "preset":
            return (
                "preset",
                "white-balance-preset",
                "whiteBalance.preset",
                "Preset",
                "preset",
            )
        if raw_property_path in {"temperature", "tint", "finetune"}:
            return (
                "core",
                "white-balance-core",
                "whiteBalance.core",
                "Core white balance",
                raw_property_path,
            )
        return (
            "channels",
            "white-balance-channels",
            "whiteBalance.channels",
            "White balance channels",
            raw_property_path,
        )
    if module_id == "colorbalancergb":
        for prefix, semantic_type, group_path, label in (
            ("global_", "global-grade", "colorBalanceRgb.global", "Global grading"),
            ("shadows_", "shadows-grade", "colorBalanceRgb.shadows", "Shadows grading"),
            (
                "midtones_",
                "midtones-grade",
                "colorBalanceRgb.midtones",
                "Midtones grading",
            ),
            (
                "highlights_",
                "highlights-grade",
                "colorBalanceRgb.highlights",
                "Highlights grading",
            ),
        ):
            if raw_property_path.startswith(prefix):
                return (
                    prefix[:-1],
                    semantic_type,
                    group_path,
                    label,
                    raw_property_path[len(prefix) :],
                )
        return (
            "options",
            "grading-options",
            "colorBalanceRgb.options",
            "Grading options",
            raw_property_path,
        )
    if module_id == "colorequal":
        for prefix, semantic_type, group_path, label in (
            (
                "sat_",
                "saturation-zones",
                "colorEqualizer.saturation",
                "Saturation zones",
            ),
            ("hue_", "hue-zones", "colorEqualizer.hue", "Hue zones"),
            (
                "bright_",
                "brightness-zones",
                "colorEqualizer.brightness",
                "Brightness zones",
            ),
        ):
            if raw_property_path.startswith(prefix):
                return (
                    prefix[:-1],
                    semantic_type,
                    group_path,
                    label,
                    raw_property_path[len(prefix) :],
                )
        return (
            "options",
            "color-equalizer-options",
            "colorEqualizer.options",
            "Color equalizer options",
            raw_property_path,
        )
    if module_id == "denoiseprofile":
        return (
            "channels",
            "noise-channels",
            "noiseReduction.channels",
            "Noise channels",
            raw_property_path,
        )
    if module_id == "filmicrgb":
        if "white" in raw_property_path or "black" in raw_property_path:
            return (
                "scene-extremes",
                "scene-extremes",
                "filmic.sceneExtremes",
                "Scene extremes",
                raw_property_path,
            )
        if "preserve" in raw_property_path:
            return (
                "preservation",
                "preservation",
                "filmic.preservation",
                "Preservation",
                raw_property_path,
            )
        return (
            "contrast-shape",
            "contrast-shape",
            "filmic.contrastShape",
            "Contrast shape",
            raw_property_path,
        )
    if module_id in {"toneequal", "toneequalizer"}:
        tone_band_names = {
            "noise",
            "ultra_deep_blacks",
            "deep_blacks",
            "blacks",
            "shadows",
            "midtones",
            "highlights",
            "whites",
            "speculars",
        }
        if raw_property_path in tone_band_names:
            return (
                "bands",
                "tone-bands",
                "toneEqualizer.bands",
                "Tone bands",
                raw_property_path,
            )
        return (
            "options",
            "toneeq-options",
            "toneEqualizer.options",
            "Tone equalizer options",
            raw_property_path,
        )
    if module_id == "splittoningrgb":
        if raw_property_path.endswith("0"):
            return (
                "shadows",
                "shadows-toning",
                "splitToning.shadows",
                "Shadows toning",
                raw_property_path,
            )
        if raw_property_path.endswith("1"):
            return (
                "highlights",
                "highlights-toning",
                "splitToning.highlights",
                "Highlights toning",
                raw_property_path,
            )
        return (
            "balance",
            "balance",
            "splitToning.balance",
            "Split toning balance",
            raw_property_path,
        )
    if module_id == "channelmixerrgb":
        for row_name in ("red", "green", "blue"):
            if raw_property_path.startswith(f"{row_name}_"):
                return (
                    f"output-{row_name}",
                    "channel-row",
                    f"channelMixer.{row_name}",
                    f"{row_name.title()} output",
                    raw_property_path[len(row_name) + 1 :],
                )
        return (
            "options",
            "mixer-options",
            "channelMixer.options",
            "Channel mixer options",
            raw_property_path,
        )
    if module_id == "exposure":
        return (
            "primary",
            "exposure-primary",
            "exposure.primary",
            "Exposure",
            raw_property_path,
        )
    return (
        "main",
        "control-group",
        f"{module_id}.main",
        "Main controls",
        raw_property_path,
    )


def _property_payload(
    setting: EditableSetting,
    *,
    node_id: str,
    property_path: str,
    raw_property_path: str,
    semantic_role: str,
) -> JsonObject:
    payload: JsonObject = {
        "controlId": setting.settingId,
        "settingId": setting.settingId,
        "capabilityId": setting.capabilityId,
        "propertyPath": property_path,
        "rawPropertyPath": raw_property_path,
        "nodeId": node_id,
        "label": setting.label,
        "kind": setting.kind,
        "supportedModes": list(setting.supportedModes),
        "actionPath": setting.actionPath,
        "semanticRole": semantic_role,
    }
    if setting.kind == "set-float":
        payload["currentNumber"] = setting.currentNumber
        payload["minNumber"] = setting.minNumber
        payload["maxNumber"] = setting.maxNumber
        payload["defaultNumber"] = setting.defaultNumber
        payload["stepNumber"] = setting.stepNumber
    elif setting.kind == "set-choice":
        payload["currentChoiceValue"] = setting.currentChoiceValue
        payload["currentChoiceId"] = setting.currentChoiceId
        payload["defaultChoiceValue"] = setting.defaultChoiceValue
        payload["choices"] = [
            choice.model_dump(mode="json") for choice in (setting.choices or [])
        ]
    elif setting.kind == "set-bool":
        payload["currentBool"] = setting.currentBool
        payload["defaultBool"] = setting.defaultBool
    return payload


def _graph_item_id(item: JsonObject) -> str:
    node_id = item.get("nodeId")
    if isinstance(node_id, str) and node_id:
        return node_id
    edge_id = item.get("edgeId")
    if isinstance(edge_id, str) and edge_id:
        return edge_id
    subgraph_id = item.get("subgraphId")
    if isinstance(subgraph_id, str) and subgraph_id:
        return subgraph_id
    return ""


def build_edit_graph(
    request: RequestEnvelope,
) -> tuple[JsonObject, dict[str, str], dict[str, JsonObject]]:
    history_by_module_instance: dict[tuple[str, int], JsonObject] = {}
    for item in request.imageSnapshot.history:
        module_id = item.module or item.instanceName or "unknown"
        history_by_module_instance[(module_id, item.multiPriority)] = {
            "historyIndex": item.num,
            "enabled": item.enabled,
            "instanceName": item.instanceName,
            "iopOrder": item.iopOrder,
        }

    module_nodes_by_key: dict[tuple[str, int], JsonObject] = {}
    group_nodes_by_id: dict[str, JsonObject] = {}
    property_ref_to_setting_id: dict[str, str] = {}
    property_by_setting_id: dict[str, JsonObject] = {}
    edges: list[JsonObject] = []

    for setting in request.imageSnapshot.editableSettings:
        instance_index = _instance_index(setting.settingId)
        module_key = (setting.moduleId, instance_index)
        module_node = module_nodes_by_key.get(module_key)
        if module_node is None:
            module_node = {
                "nodeId": _module_node_id(setting.moduleId, instance_index),
                "nodeType": "module-instance",
                "semanticType": _module_semantic_type(setting.moduleId),
                "moduleId": setting.moduleId,
                "moduleLabel": setting.moduleLabel,
                "instanceIndex": instance_index,
                "history": history_by_module_instance.get(module_key, {}),
                "groupNodeIds": [],
                "properties": [],
            }
            module_nodes_by_key[module_key] = module_node

        module_node_id = cast(str, module_node["nodeId"])
        raw_property_path = _raw_property_path(setting)
        group_key, semantic_type, group_path, group_label, semantic_property_path = (
            _group_spec(setting.moduleId, raw_property_path)
        )
        semantic_role = f"{group_path}.{semantic_property_path}"

        module_property = _property_payload(
            setting,
            node_id=module_node_id,
            property_path=raw_property_path,
            raw_property_path=raw_property_path,
            semantic_role=semantic_role,
        )
        cast(list[JsonObject], module_node["properties"]).append(module_property)

        module_graph_refs = {
            f"{module_node_id}:{raw_property_path}",
            f"{module_node_id}:{semantic_property_path}",
        }
        for graph_ref in module_graph_refs:
            property_ref_to_setting_id[graph_ref] = setting.settingId

        group_node_id = _group_node_id(setting.moduleId, instance_index, group_key)
        group_node = group_nodes_by_id.get(group_node_id)
        if group_node is None:
            group_node = {
                "nodeId": group_node_id,
                "nodeType": "control-group",
                "semanticType": semantic_type,
                "moduleNodeId": module_node_id,
                "groupKey": group_key,
                "groupPath": group_path,
                "label": group_label,
                "properties": [],
            }
            group_nodes_by_id[group_node_id] = group_node
            cast(list[str], module_node["groupNodeIds"]).append(group_node_id)
            edges.append(
                {
                    "edgeType": "contains",
                    "fromNodeId": module_node_id,
                    "toNodeId": group_node_id,
                }
            )

        group_property = _property_payload(
            setting,
            node_id=group_node_id,
            property_path=semantic_property_path,
            raw_property_path=raw_property_path,
            semantic_role=semantic_role,
        )
        cast(list[JsonObject], group_node["properties"]).append(group_property)
        property_ref_to_setting_id[f"{group_node_id}:{semantic_property_path}"] = (
            setting.settingId
        )
        property_by_setting_id[setting.settingId] = group_property

    module_nodes = list(module_nodes_by_key.values())
    module_nodes.sort(
        key=lambda node: (
            cast(JsonObject, node.get("history") or {}).get("iopOrder", 10_000),
            cast(str, node["moduleId"]),
            cast(int, node["instanceIndex"]),
        )
    )

    for previous, current in zip(module_nodes, module_nodes[1:], strict=False):
        edges.append(
            {
                "edgeType": "pipeline-order",
                "fromNodeId": previous["nodeId"],
                "toNodeId": current["nodeId"],
            }
        )

    subgraphs: list[JsonObject] = []
    for module_node in module_nodes:
        subgraphs.append(
            {
                "subgraphId": f"subgraph:{module_node['moduleId']}:{module_node['instanceIndex']}",
                "subgraphType": "module-cluster",
                "semanticType": module_node["semanticType"],
                "rootNodeId": module_node["nodeId"],
                "nodeIds": [
                    module_node["nodeId"],
                    *cast(list[str], module_node["groupNodeIds"]),
                ],
            }
        )

    graph_nodes = [*module_nodes, *group_nodes_by_id.values()]
    graph_payload: JsonObject = {
        "graphId": "edit-pipeline",
        "graphType": "module-property-graph",
        "schemaVersion": "2",
        "authoritative": True,
        "nodeCount": len(graph_nodes),
        "edgeCount": len(edges),
        "subgraphCount": len(subgraphs),
        "nodes": graph_nodes,
        "edges": edges,
        "subgraphs": subgraphs,
    }

    native_graph = request.imageSnapshot.editGraph
    if (
        isinstance(native_graph, dict)
        and native_graph.get("graphId") == "edit-pipeline"
    ):
        for collection_key in ("nodes", "edges", "subgraphs"):
            native_items = native_graph.get(collection_key)
            if not isinstance(native_items, list):
                continue
            destination = cast(list[JsonObject], graph_payload[collection_key])
            existing_ids = {
                _graph_item_id(item) for item in destination if isinstance(item, dict)
            }
            for item in native_items:
                if not isinstance(item, dict):
                    continue
                item_id = _graph_item_id(cast(JsonObject, item))
                if item_id and item_id in existing_ids:
                    continue
                destination.append(cast(JsonObject, item))
                if item_id:
                    existing_ids.add(item_id)

        graph_payload["nodeCount"] = len(graph_payload["nodes"])
        graph_payload["edgeCount"] = len(graph_payload["edges"])
        graph_payload["subgraphCount"] = len(graph_payload["subgraphs"])

    return graph_payload, property_ref_to_setting_id, property_by_setting_id
