from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
SIGNAL_C = REPO_ROOT / "ansel" / "src" / "control" / "signal.c"
FILTER_C = REPO_ROOT / "ansel" / "src" / "libs" / "tools" / "filter.c"
GTK_C = REPO_ROOT / "ansel" / "src" / "gui" / "gtk.c"


def test_signal_emitter_supports_int_arguments() -> None:
    source = SIGNAL_C.read_text()

    assert "case G_TYPE_INT:" in source
    assert "g_value_set_int(&instance_and_params[i], va_arg(extra_args, gint));" in source


def test_filter_popup_uses_action_shortcuts_not_menu_widget_shortcuts() -> None:
    source = FILTER_C.read_text()

    assert "dt_accels_new_lighttable_action(_refresh_collection_action" in source
    assert "dt_accels_new_lighttable_action(_toggle_culling_mode_action" in source
    assert "dt_accels_new_lighttable_action(_select_all_filters_action" in source
    assert "dt_accels_new_lighttable_action(_select_none_filters_action" in source
    assert "dt_accels_new_widget_shortcut(" not in source


def test_macos_menu_bar_uses_real_ui_menu_bar() -> None:
    source = GTK_C.read_text()

    assert "osx_menu_bar = dt_ui_get_menu_bar(gui->ui);" in source
    assert "GTK_IS_MENU_SHELL(osx_menu_bar)" in source
    assert "gtk_menu_bar_new())); // needed for default entries to show up" not in source
