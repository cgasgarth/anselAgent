from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
DARKROOM_C = REPO_ROOT / "ansel" / "src" / "views" / "darkroom.c"
DARKTABLE_C = REPO_ROOT / "ansel" / "src" / "common" / "darktable.c"


def test_chat_window_has_minimum_size_and_bottom_input_row() -> None:
    source = DARKROOM_C.read_text()

    assert "gtk_widget_set_size_request(dev->agent_chat.floating_window," in source
    assert (
        "GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_PIXEL_APPLY_DPI(6));"
        in source
    )
    assert "gtk_box_pack_start(GTK_BOX(outer), content, TRUE, TRUE, 0);" in source
    assert (
        "gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), DT_PIXEL_APPLY_DPI(320));"
        in source
    )
    assert "gtk_box_pack_start(GTK_BOX(content), scroll, TRUE, TRUE, 0);" in source
    assert "gtk_box_pack_start(GTK_BOX(content), input_row, FALSE, FALSE, 0);" in source
    assert 'dev->agent_chat.conversation_view = gtk_label_new("");' in source
    assert (
        "gtk_label_set_line_wrap(GTK_LABEL(dev->agent_chat.conversation_view), TRUE);"
        in source
    )
    assert "gtk_widget_set_hexpand(dev->agent_chat.input_entry, TRUE);" in source
    assert (
        "static gboolean _agent_chat_window_reflow_idle(gpointer user_data);" in source
    )
    assert (
        "g_idle_add(_agent_chat_window_reflow_idle, dev->agent_chat.floating_window);"
        in source
    )
    assert "static gboolean _agent_chat_focus_input_idle(gpointer user_data);" in source
    assert "gtk_window_set_focus(GTK_WINDOW(dev->agent_chat.floating_window), dev->agent_chat.input_entry);" in source
    assert "gtk_editable_set_position(GTK_EDITABLE(dev->agent_chat.input_entry), -1);" in source
    assert "g_idle_add(_agent_chat_focus_input_idle, dev);" in source


def test_chat_progress_ignores_not_found_status_updates() -> None:
    source = DARKROOM_C.read_text()

    assert "if(progress->status && progress->status[0] != '\\0')" in source
    assert "if(progress->found)" in source
    assert "_agent_chat_set_status(dev, progress->status);" in source
    assert "_agent_chat_set_status(dev, progress->message);" in source


def test_agent_smoke_autorun_hook_is_wired() -> None:
    source = DARKROOM_C.read_text()

    assert 'g_getenv("ANSEL_AGENT_TEST_AUTORUN_PROMPT")' in source
    assert 'g_getenv("ANSEL_AGENT_TEST_RESULT_FILE")' in source
    assert "g_idle_add(_agent_chat_test_autorun_idle, dev);" in source
    assert "_agent_chat_test_write_report(dev, \"ok\"" in source
    assert "dt_control_quit();" in source


def test_agent_smoke_loads_cli_asset_when_launcher_passes_options() -> None:
    source = DARKTABLE_C.read_text()

    assert 'g_getenv("ANSEL_AGENT_TEST_AUTORUN_PROMPT")' in source
    assert "for(int arg = argc - 1; arg >= 1; arg--)" in source
    assert "argv[arg][0] != '-'" in source
    assert "g_file_test(argv[arg], G_FILE_TEST_EXISTS)" in source
    assert "dt_load_from_string(argv[arg], TRUE, NULL);" in source


def test_agent_chat_final_response_preserves_live_applied_count() -> None:
    source = DARKROOM_C.read_text()

    assert "const guint already_live_applied_count = active_request" in source
    assert "_agent_chat_clear_active_request(dev);" in source
    assert "_agent_chat_apply_operation_range(result->response.operations," in source
    assert "already_live_applied_count," in source


def test_agent_smoke_report_does_not_recollect_full_state_after_apply() -> None:
    source = DARKROOM_C.read_text()

    assert "static double _agent_chat_test_exposure_after_response" in source
    assert "static double _agent_chat_test_exposure_after = NAN;" in source
    assert 'g_strcmp0(operation->action_path, "iop/exposure/exposure")' in source
    assert "exposure += operation->number;" in source
    assert "const double exposure_fallback = isnan(_agent_chat_test_exposure_after)" in source
    assert "_agent_chat_test_exposure_after = current_exposure;" in source
    assert (
        "const double current_exposure = _agent_chat_test_exposure_after_response("
        in source
    )


def test_agent_smoke_reuses_initial_request_context_for_refinement() -> None:
    source = DARKROOM_C.read_text()

    assert "static dt_agent_chat_request_t _agent_chat_test_cached_request;" in source
    assert "_agent_chat_test_cache_request_context(request);" in source
    assert "static gboolean _agent_chat_test_copy_cached_request" in source
    assert "const gboolean use_cached_test_context = refinement_pass_index > 1u" in source
    assert (
        "_agent_chat_build_request(dev, message_text, &request, use_cached_test_context, &error)"
        in source
    )
