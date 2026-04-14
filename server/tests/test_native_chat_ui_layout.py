from pathlib import Path


REPO_ROOT = Path(__file__).resolve().parents[2]
DARKROOM_C = REPO_ROOT / "ansel" / "src" / "views" / "darkroom.c"


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
