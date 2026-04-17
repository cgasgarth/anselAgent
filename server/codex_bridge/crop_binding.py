from __future__ import annotations

from shared.canonical_plan import CanonicalEditAction
from shared.protocol import EditableSetting


def bind_crop_action(
    settings: list[EditableSetting],
    action: CanonicalEditAction,
    *,
    image_width: int | float | None = None,
    image_height: int | float | None = None,
) -> tuple[list[dict[str, object]], list[str]]:
    if action.action == "crop-normalized":
        return _bind_crop_normalized(settings, action)
    if action.action == "crop-to-bounding-box":
        return _bind_crop_to_bounding_box(
            settings,
            action,
            image_width=image_width,
            image_height=image_height,
        )
    return [], [f"unsupported canonical action {action.action}"]


def _bind_crop_normalized(
    settings: list[EditableSetting], action: CanonicalEditAction
) -> tuple[list[dict[str, object]], list[str]]:
    assert action.left is not None
    assert action.top is not None
    assert action.right is not None
    assert action.bottom is not None
    return _bind_crop_edges(
        settings,
        left=action.left,
        top=action.top,
        right=action.right,
        bottom=action.bottom,
        rationale=action.rationale,
    )


def _bind_crop_to_bounding_box(
    settings: list[EditableSetting],
    action: CanonicalEditAction,
    *,
    image_width: int | float | None = None,
    image_height: int | float | None = None,
) -> tuple[list[dict[str, object]], list[str]]:
    assert action.boxLeft is not None
    assert action.boxTop is not None
    assert action.boxWidth is not None
    assert action.boxHeight is not None
    padding_ratio = action.paddingRatio or 0.0
    pad_x = action.boxWidth * padding_ratio
    pad_y = action.boxHeight * padding_ratio
    left = max(0.0, action.boxLeft - pad_x)
    top = max(0.0, action.boxTop - pad_y)
    right = min(1.0, action.boxLeft + action.boxWidth + pad_x)
    bottom = min(1.0, action.boxTop + action.boxHeight + pad_y)

    if action.aspectRatioWidth is not None and action.aspectRatioHeight is not None:
        if not image_width or not image_height:
            return [], ["crop-to-bounding-box aspect ratio requires image dimensions"]
        fitted_edges, fit_error = _fit_crop_bounds_to_aspect_ratio(
            left=left,
            top=top,
            right=right,
            bottom=bottom,
            aspect_ratio_width=action.aspectRatioWidth,
            aspect_ratio_height=action.aspectRatioHeight,
            image_width=float(image_width),
            image_height=float(image_height),
        )
        if fit_error is not None:
            if padding_ratio > 0.0:
                fitted_edges, fit_error = _fit_crop_bounds_to_aspect_ratio(
                    left=action.boxLeft,
                    top=action.boxTop,
                    right=action.boxLeft + action.boxWidth,
                    bottom=action.boxTop + action.boxHeight,
                    aspect_ratio_width=action.aspectRatioWidth,
                    aspect_ratio_height=action.aspectRatioHeight,
                    image_width=float(image_width),
                    image_height=float(image_height),
                )
            if fit_error is not None:
                return [], [fit_error]
        assert fitted_edges is not None
        left, top, right, bottom = fitted_edges

    return _bind_crop_edges(
        settings,
        left=left,
        top=top,
        right=right,
        bottom=bottom,
        rationale=action.rationale,
    )


def _fit_crop_bounds_to_aspect_ratio(
    *,
    left: float,
    top: float,
    right: float,
    bottom: float,
    aspect_ratio_width: float,
    aspect_ratio_height: float,
    image_width: float,
    image_height: float,
) -> tuple[tuple[float, float, float, float] | None, str | None]:
    if image_width <= 0.0 or image_height <= 0.0:
        return None, "crop-to-bounding-box aspect ratio requires positive image dimensions"

    target_ratio = aspect_ratio_width / aspect_ratio_height
    if target_ratio <= 0.0:
        return None, "crop-to-bounding-box aspect ratio must be greater than 0"

    normalized_ratio = target_ratio * (image_height / image_width)
    if normalized_ratio <= 0.0:
        return None, "crop-to-bounding-box aspect ratio could not be normalized"

    box_width = right - left
    box_height = bottom - top
    if box_width <= 0.0 or box_height <= 0.0:
        return None, "crop bounds would collapse to an empty area"

    max_crop_height = min(1.0, 1.0 / normalized_ratio)
    required_crop_height = max(box_height, box_width / normalized_ratio)
    if required_crop_height > max_crop_height + 1e-9:
        return (
            None,
            "crop-to-bounding-box aspect ratio cannot fit the requested bounds inside the image",
        )

    crop_height = required_crop_height
    crop_width = normalized_ratio * crop_height
    center_x = (left + right) / 2.0
    center_y = (top + bottom) / 2.0
    crop_left = center_x - crop_width / 2.0
    crop_right = center_x + crop_width / 2.0
    crop_top = center_y - crop_height / 2.0
    crop_bottom = center_y + crop_height / 2.0

    if crop_left < 0.0:
        crop_right -= crop_left
        crop_left = 0.0
    if crop_right > 1.0:
        crop_left -= crop_right - 1.0
        crop_right = 1.0
    if crop_top < 0.0:
        crop_bottom -= crop_top
        crop_top = 0.0
    if crop_bottom > 1.0:
        crop_top -= crop_bottom - 1.0
        crop_bottom = 1.0

    crop_left = max(0.0, crop_left)
    crop_top = max(0.0, crop_top)
    crop_right = min(1.0, crop_right)
    crop_bottom = min(1.0, crop_bottom)
    return (crop_left, crop_top, crop_right, crop_bottom), None


def _bind_crop_edges(
    settings: list[EditableSetting],
    *,
    left: float,
    top: float,
    right: float,
    bottom: float,
    rationale: str | None,
) -> tuple[list[dict[str, object]], list[str]]:
    crop_settings = {
        "left": _find_crop_setting(
            settings,
            exact_action_paths=("iop/clipping/cx", "iop/crop/cx"),
            action_keywords=("cx",),
            label_keywords=("left", "cx"),
        ),
        "top": _find_crop_setting(
            settings,
            exact_action_paths=("iop/clipping/cy", "iop/crop/cy"),
            action_keywords=("cy",),
            label_keywords=("top", "cy"),
        ),
        "right": _find_crop_setting(
            settings,
            exact_action_paths=("iop/clipping/cw", "iop/crop/cw"),
            action_keywords=("cw",),
            label_keywords=("right", "cw"),
        ),
        "bottom": _find_crop_setting(
            settings,
            exact_action_paths=("iop/clipping/ch", "iop/crop/ch"),
            action_keywords=("ch",),
            label_keywords=("bottom", "ch"),
        ),
    }
    missing_edges = [
        edge for edge, setting in crop_settings.items() if setting is None
    ]
    if missing_edges:
        return [], ["crop controls are unavailable for " + ", ".join(missing_edges)]

    if left >= right or top >= bottom:
        return [], ["crop bounds would collapse to an empty area"]

    crop_settings = {edge: setting for edge, setting in crop_settings.items() if setting is not None}
    edge_values = {
        "left": left,
        "top": top,
        "right": right,
        "bottom": bottom,
    }
    operations = [
        _crop_operation(crop_settings[edge], edge_values[edge], rationale)
        for edge in ("left", "top", "right", "bottom")
    ]
    return operations, []


def _find_crop_setting(
    settings: list[EditableSetting],
    *,
    exact_action_paths: tuple[str, ...],
    action_keywords: tuple[str, ...],
    label_keywords: tuple[str, ...],
) -> EditableSetting | None:
    candidates: list[tuple[int, EditableSetting]] = []
    for setting in settings:
        if setting.kind != "set-float":
            continue
        score = 0
        action_path = setting.actionPath.lower()
        label = setting.label.lower()
        module_id = setting.moduleId.lower()
        if setting.actionPath in exact_action_paths:
            score += 100
        if module_id in {"clipping", "crop"}:
            score += 20
        score += sum(8 for keyword in action_keywords if keyword in action_path)
        score += sum(6 for keyword in label_keywords if keyword in label)
        if score > 0:
            candidates.append((score, setting))

    if not candidates:
        return None

    candidates.sort(
        key=lambda item: (
            -item[0],
            item[1].moduleId,
            item[1].actionPath,
            item[1].settingId,
        )
    )
    return candidates[0][1]


def _crop_operation(
    setting: EditableSetting,
    value: float,
    rationale: str | None,
) -> dict[str, object]:
    return {
        "operationId": f"bind-{setting.settingId}",
        "sequence": 1,
        "kind": "set-float",
        "target": {
            "type": "ansel-action",
            "actionPath": setting.actionPath,
            "settingId": setting.settingId,
        },
        "value": {"mode": "set", "number": value},
        "reason": rationale,
        "constraints": {"onOutOfRange": "clamp", "onRevisionMismatch": "fail"},
    }
