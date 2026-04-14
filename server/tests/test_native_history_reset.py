from pathlib import Path
import re


REPO_ROOT = Path(__file__).resolve().parents[2]
HISTORY_C = REPO_ROOT / "ansel" / "src" / "libs" / "history.c"


def _gui_reset_body() -> str:
    source = HISTORY_C.read_text()
    match = re.search(
        r"void gui_reset\(dt_lib_module_t \*self\)\n\{(?P<body>.*?)\n\}", source, re.S
    )
    assert match, "gui_reset() not found in ansel/src/libs/history.c"
    return match.group("body")


def test_gui_reset_reloads_dev_history_after_delete() -> None:
    body = _gui_reset_body()

    delete_index = body.find("dt_history_delete_on_image_ext(imgid, FALSE);")
    reload_index = body.find("dt_dev_reload_history_items(darktable.develop, imgid);")
    gui_update_index = body.find("dt_dev_history_gui_update(darktable.develop);")
    notify_index = body.find("dt_dev_history_notify_change(darktable.develop, imgid);")
    signal_index = body.find(
        "DT_DEBUG_CONTROL_SIGNAL_RAISE(darktable.signals, DT_SIGNAL_DEVELOP_HISTORY_CHANGE);"
    )

    assert delete_index >= 0, "gui_reset() must delete persisted history"
    assert reload_index > delete_index, (
        "gui_reset() must reload in-memory dev history after deleting DB history"
    )
    assert gui_update_index > reload_index, (
        "gui_reset() must refresh the history GUI after reloading history"
    )
    assert notify_index > gui_update_index, (
        "gui_reset() must notify darkroom listeners after refreshing history"
    )
    assert signal_index > notify_index, (
        "gui_reset() must raise a history-change signal after notifying listeners"
    )


def test_gui_reset_refreshes_visible_thumbnails() -> None:
    body = _gui_reset_body()

    assert (
        "dt_thumbtable_refresh_thumbnail(darktable.gui->ui->thumbtable_lighttable, imgid, TRUE);"
        in body
    ), "gui_reset() must refresh the lighttable thumbnail for the reset image"
    assert (
        "dt_thumbtable_refresh_thumbnail(darktable.gui->ui->thumbtable_filmstrip, imgid, TRUE);"
        in body
    ), "gui_reset() must refresh the filmstrip thumbnail for the reset image"


def test_gui_reset_rebuilds_darkroom_pixelpipe() -> None:
    body = _gui_reset_body()

    assert "dt_dev_history_pixelpipe_update(darktable.develop, TRUE);" in body, (
        "gui_reset() must fully rebuild the darkroom pixelpipe after reloading history"
    )
