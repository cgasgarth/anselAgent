/*
    This file is part of darktable,
    Copyright (C) 2009-2014, 2018 johannes hanika.
    Copyright (C) 2010-2012 Henrik Andersson.
    Copyright (C) 2010 Stuart Henderson.
    Copyright (C) 2010-2018, 2020 Tobias Ellinghaus.
    Copyright (C) 2011-2012 Antony Dovgal.
    Copyright (C) 2011 Edouard Gomez.
    Copyright (C) 2011, 2013-2015 Jérémy Rosen.
    Copyright (C) 2011 Karl Mikaelsson.
    Copyright (C) 2011 Omari Stephens.
    Copyright (C) 2011 Robert Bieber.
    Copyright (C) 2011 Steven Carter.
    Copyright (C) 2012-2016, 2019-2022 Aldric Renaudin.
    Copyright (C) 2012 Christian Tellefsen.
    Copyright (C) 2012 Frédéric Grollier.
    Copyright (C) 2012-2013 José Carlos García Sogo.
    Copyright (C) 2012-2022 Pascal Obry.
    Copyright (C) 2012 Richard Wonka.
    Copyright (C) 2012-2014 Ulrich Pegelow.
    Copyright (C) 2013 Dennis Gnad.
    Copyright (C) 2013 Pascal de Bruijn.
    Copyright (C) 2013-2017, 2020 Roman Lebedev.
    Copyright (C) 2014, 2017, 2020-2021 Dan Torop.
    Copyright (C) 2014, 2017, 2019 parafin.
    Copyright (C) 2014 Pedro Côrte-Real.
    Copyright (C) 2014 Stéphane Gimenez.
    Copyright (C) 2015 Guillaume Subiron.
    Copyright (C) 2016 Asma.
    Copyright (C) 2016 itinerarium.
    Copyright (C) 2017-2019 Edgardo Hoszowski.
    Copyright (C) 2017 Matthieu Moy.
    Copyright (C) 2017-2019 Peter Budai.
    Copyright (C) 2017 Žilvinas Žaltiena.
    Copyright (C) 2018 Hans Rosenfeld.
    Copyright (C) 2018 rawfiner.
    Copyright (C) 2018 Rikard Öxler.
    Copyright (C) 2019 Alexander Blinne.
    Copyright (C) 2019 Alexis Mousset.
    Copyright (C) 2019-2020, 2022-2026 Aurélien PIERRE.
    Copyright (C) 2019, 2021 Bill Ferguson.
    Copyright (C) 2019-2022 Diederik Ter Rahe.
    Copyright (C) 2019-2022 Hanno Schwalm.
    Copyright (C) 2019-2020 Heiko Bauke.
    Copyright (C) 2019 jakubfi.
    Copyright (C) 2019, 2021 luzpaz.
    Copyright (C) 2019-2020 Philippe Weyland.
    Copyright (C) 2020-2022 Chris Elston.
    Copyright (C) 2020 GrahamByrnes.
    Copyright (C) 2020-2021 Hubert Kowalski.
    Copyright (C) 2020-2021 Marco.
    Copyright (C) 2020-2021 Mark-64.
    Copyright (C) 2020 Miloš Komarčević.
    Copyright (C) 2020 Reinout Nonhebel.
    Copyright (C) 2020 U-DESKTOP-HQME86J\marco.
    Copyright (C) 2021 darkelectron.
    Copyright (C) 2021 lhietal.
    Copyright (C) 2021-2022 Nicolas Auffray.
    Copyright (C) 2021-2022 Ralf Brown.
    Copyright (C) 2021-2022 Sakari Kapanen.
    Copyright (C) 2021 Victor Forsiuk.
    Copyright (C) 2022 Martin Bařinka.
    Copyright (C) 2023 Alynx Zhou.
    Copyright (C) 2023 Luca Zulberti.
    Copyright (C) 2023 Maurizio Paglia.
    Copyright (C) 2023 Ricky Moon.
    Copyright (C) 2025-2026 Guillaume Stutin.
    
    darktable is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    darktable is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with darktable.  If not, see <http://www.gnu.org/licenses/>.
*/
/** this is the view for the darkroom module.  */

#include "common/extra_optimizations.h"

#include "bauhaus/bauhaus.h"
#if defined(__has_include)
#if __has_include("common/agent_capabilities.h") && __has_include("common/agent_client.h") && __has_include("common/agent_execute.h") && __has_include("common/agent_protocol.h") && __has_include("common/agent_state.h")
#define HAVE_ANSEL_AGENT_CHAT_BACKEND 1
#include "common/agent_capabilities.h"
#include "common/agent_client.h"
#include "common/agent_execute.h"
#include "common/agent_protocol.h"
#include "common/agent_state.h"
#else
#define HAVE_ANSEL_AGENT_CHAT_BACKEND 0
#endif
#else
#define HAVE_ANSEL_AGENT_CHAT_BACKEND 0
#endif
#include "common/collection.h"
#include "common/colorspaces.h"
#include "common/curl_tools.h"
#include "common/darktable.h"
#include "common/debug.h"
#include "common/file_location.h"
#include "common/history.h"
#include "common/image_cache.h"
#include "common/imageio.h"
#include "common/imageio_module.h"
#include "common/mipmap_cache.h"
#include "common/selection.h"
#include "common/tags.h"
#include "common/undo.h"
#include "control/conf.h"
#include "control/control.h"
#include "control/jobs.h"
#include "develop/blend.h"
#include "develop/dev_pixelpipe.h"
#include "develop/develop.h"
#include "develop/imageop.h"
#include "develop/masks.h"
#include "dtgtk/button.h"
#include "dtgtk/thumbtable.h"

#include "gui/color_picker_proxy.h"
#include "gui/draw.h"
#include "gui/gtk.h"
#include "gui/gui_throttle.h"
#include "gui/guides.h"
#include "gui/presets.h"
#include "libs/colorpicker.h"
#include "libs/lib.h"
#include "libs/modulegroups.h"
#include "views/view.h"
#include "views/view_api.h"
#ifdef GDK_WINDOWING_QUARTZ
#include "osx/osx.h"
#endif

#ifdef USE_LUA
#include "lua/image.h"
#endif

#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <curl/curl.h>

#ifndef G_SOURCE_FUNC // Defined for glib >= 2.58
#define G_SOURCE_FUNC(f) ((GSourceFunc) (void (*)(void)) (f))
#endif

DT_MODULE(1)

typedef struct coords_t
{
  double roi_coord[2];  // coordinates in ROI space
  double roi_delta[2];  // delta in ROI space
  double full_delta[2]; // delta in image space
  double full[2];       // coordinates in image space
}coords_t;

static void _update_softproof_gamut_checking(dt_develop_t *d);

/* signal handler for filmstrip image switching */
static void _view_darkroom_filmstrip_activate_callback(gpointer instance, int32_t imgid, gpointer user_data);
static void _darkroom_image_loaded_callback(gpointer instance, guint request_id, guint result, gpointer user_data);
static void _darkroom_ui_pipe_finish_signal_callback(gpointer instance, gpointer user_data);

static void _dev_change_image(dt_view_t *self, const int32_t imgid);

static int _change_scaling(dt_develop_t *dev, const float point[2], const float new_scaling);
static void _release_expose_source_caches(void);
static void _agent_chat_window_write_config(GtkWidget *widget);
static gboolean _agent_chat_window_reflow_idle(gpointer user_data);
static gboolean _agent_chat_focus_input_idle(gpointer user_data);
static void _agent_chat_update_sensitivity(dt_develop_t *dev);
static void _agent_chat_set_status(dt_develop_t *dev, const char *status);
static void _agent_chat_set_error(dt_develop_t *dev, const char *error);
static void _agent_chat_set_loading(dt_develop_t *dev, gboolean is_loading);
static void _agent_chat_cancel_active_request(dt_develop_t *dev, const char *status_text);
static void _agent_chat_reset_for_current_image(dt_develop_t *dev, gboolean force_new_chat);
static void _agent_chat_image_changed_callback(gpointer instance, gpointer user_data);
static gboolean _agent_chat_window_delete_callback(GtkWidget *widget, GdkEvent *event, gpointer user_data);
static void _agent_chat_toggle_callback(GtkToggleButton *button, gpointer user_data);
static void _agent_chat_send_clicked(GtkButton *button, gpointer user_data);
static void _agent_chat_cancel_clicked(GtkButton *button, gpointer user_data);
static void _agent_chat_new_chat_clicked(GtkButton *button, gpointer user_data);
static void _agent_chat_entry_activate(GtkEntry *entry, gpointer user_data);

#if HAVE_ANSEL_AGENT_CHAT_BACKEND
typedef struct dt_agent_chat_submission_t
{
  gchar *request_id;
  gchar *prompt_text;
  gchar *conversation_id;
  gchar *image_session_id;
  int32_t image_id;
  guint refinement_pass_index;
  guint refinement_max_passes;
} dt_agent_chat_submission_t;

typedef struct dt_agent_chat_render_delivery_t
{
  gchar *image_session_id;
  gchar *turn_id;
  guchar *jpeg_bytes;
  gsize jpeg_len;
  gchar *endpoint;
} dt_agent_chat_render_delivery_t;

static gboolean _agent_chat_test_autorun_started = FALSE;
static guint _agent_chat_test_refinement_pass_count = 0;
static double _agent_chat_test_exposure_before = NAN;
static double _agent_chat_test_exposure_after = NAN;
static gboolean _agent_chat_test_cached_request_valid = FALSE;
static dt_agent_chat_request_t _agent_chat_test_cached_request;

static gboolean _agent_chat_submit_request(dt_develop_t *dev,
                                           const char *message_text,
                                           guint refinement_pass_index,
                                           guint refinement_max_passes,
                                           gboolean append_user_message);
static void _agent_chat_encode_and_post_preview(dt_develop_t *dev);
static gboolean _agent_chat_apply_operation_range(const GPtrArray *operations,
                                                  guint start_index,
                                                  dt_agent_execution_report_t *execution_report,
                                                  GError **error);
static void _agent_chat_progress_finished(const dt_agent_client_progress_t *progress,
                                          gpointer user_data);
static void _agent_chat_request_finished(const dt_agent_client_result_t *result,
                                         gpointer user_data);
static gboolean _agent_chat_test_autorun_idle(gpointer user_data);
static void _agent_chat_test_write_report(dt_develop_t *dev,
                                          const char *status,
                                          const char *error_message,
                                          const dt_agent_chat_response_t *response,
                                          const guint operation_count,
                                          const guint execution_blocked_count,
                                          const guint execution_failed_count);
static void _agent_chat_test_cache_request_context(const dt_agent_chat_request_t *request);
static gboolean _agent_chat_test_copy_cached_request(dt_develop_t *dev,
                                                     const char *message_text,
                                                     dt_agent_chat_request_t *request);
static void _agent_chat_test_schedule_quit(void);
#endif

static int32_t _darkroom_pending_imgid = UNKNOWN_IMAGE;
static dt_iop_module_t *_darkroom_pending_focus_module = NULL;
static GtkWidget *_darkroom_ioporder_button = NULL;

static void _darkroom_ioporder_quickbutton_clicked(GtkButton *button, gpointer user_data)
{
  dt_lib_module_t *module = dt_lib_get_module("ioporder");
  if(module && module->show_popup)
    module->show_popup(module);

  (void)button;
  (void)user_data;
}

const char *name(const dt_view_t *self)
{
  return _("Darkroom");
}


void init(dt_view_t *self)
{
  self->data = malloc(sizeof(dt_develop_t));
  dt_dev_init((dt_develop_t *)self->data, 1);
  darktable.develop = (dt_develop_t *)self->data;
  darktable.view_manager->proxy.darkroom.view = self;

#ifdef USE_LUA
  lua_State *L = darktable.lua_state.state;
  const int my_type = dt_lua_module_entry_get_type(L, "view", self->module_name);
  lua_pushlightuserdata(L, self);
  lua_pushcclosure(L, display_image_cb, 1);
  dt_lua_gtk_wrap(L);
  lua_pushcclosure(L, dt_lua_type_member_common, 1);
  dt_lua_type_register_const_type(L, my_type, "display_image");

  lua_pushcfunction(L, dt_lua_event_multiinstance_register);
  lua_pushcfunction(L, dt_lua_event_multiinstance_destroy);
  lua_pushcfunction(L, dt_lua_event_multiinstance_trigger);
  dt_lua_event_add(L, "darkroom-image-loaded");
#endif
}

uint32_t view(const dt_view_t *self)
{
  return DT_VIEW_DARKROOM;
}

void cleanup(dt_view_t *self)
{
  dt_develop_t *dev = (dt_develop_t *)self->data;
  _release_expose_source_caches();
  dt_gui_throttle_cancel(dev);

  // unref the grid lines popover if needed
  if(darktable.view_manager->guides_popover) g_object_unref(darktable.view_manager->guides_popover);
  _darkroom_pending_imgid = UNKNOWN_IMAGE;
  _darkroom_pending_focus_module = NULL;
  _agent_chat_cancel_active_request(dev, NULL);
  if(dev->agent_chat.floating_window)
  {
    _agent_chat_window_write_config(dev->agent_chat.floating_window);
    gtk_window_close(GTK_WINDOW(dev->agent_chat.floating_window));
    dev->agent_chat.floating_window = NULL;
  }
  g_free(dev->agent_chat.app_session_id);
  g_free(dev->agent_chat.image_session_id);
  g_free(dev->agent_chat.conversation_id);
  g_free(dev->agent_chat.active_request_id);
  dt_dev_cleanup(dev);
  dt_free(dev);
}

static cairo_status_t _write_snapshot_data(void *closure, const unsigned char *data, unsigned int length)
{
  const int fd = GPOINTER_TO_INT(closure);
  ssize_t res = write(fd, data, length);
  if(res != length)
    return CAIRO_STATUS_WRITE_ERROR;
  return CAIRO_STATUS_SUCCESS;
}

static dt_darkroom_layout_t _lib_darkroom_get_layout(dt_view_t *self)
{
  return DT_DARKROOM_LAYOUT_EDITING;
}

static gboolean _darkroom_is_only_selected_sample(gboolean is_primary_sample, dt_colorpicker_sample_t *selected_sample, gboolean display_samples)
{
  return !is_primary_sample && selected_sample && !display_samples;
}

/**
 * @brief Draw colorpicker samples overlays in darkroom view
 * 
 * @param self actual view
 * @param cri cairo context
 * @param width width of the widget
 * @param height height of the widget
 * @param pozx x pointer
 * @param pozy y pointer
 * @param samples list of samples to draw
 * @param is_primary_sample whether we are drawing the primary sample or live samples
 */
static void _darkroom_pickers_draw(dt_view_t *self, cairo_t *cri,
                                   int32_t width, int32_t height, int32_t pozx, int32_t pozy,
                                   GSList *samples, gboolean is_primary_sample)
{
  if(IS_NULL_PTR(samples)) return;

  dt_develop_t *dev = (dt_develop_t *)self->data;

  cairo_save(cri);
  // The colorpicker samples bounding rectangle should only be displayed inside the visible image

  const double wd = dev->roi.preview_width;
  const double ht = dev->roi.preview_height;
  const double scale = dt_dev_get_fit_scale(dev);
  const double lw = 1.0 / scale;
  const double dashes[1] = { lw * 4.0 };

  dt_dev_rescale_roi(dev, cri, width, height);

  // makes point sample crosshair gap look nicer
  cairo_set_line_cap(cri, CAIRO_LINE_CAP_SQUARE);

  dt_colorpicker_sample_t *selected_sample = darktable.develop->color_picker.selected_sample;
  const gboolean only_selected_sample
      = _darkroom_is_only_selected_sample(is_primary_sample, selected_sample,
                                          darktable.develop->color_picker.display_samples);
  
  for( ; samples; samples = g_slist_next(samples))
  {
    dt_colorpicker_sample_t *sample = samples->data;
    if(only_selected_sample && (sample != selected_sample))
      continue;

    // The picker is at the resolution of the preview pixelpipe. This
    // is width/2 of a preview-pipe pixel in (scaled) user space
    // coordinates. Use half pixel width so rounding to nearest device
    // pixel doesn't make uneven centering.
    double half_px = 0.5;
    const double min_half_px_device = 4.0;
    // FIXME: instead of going to all this effort to show how error-prone a preview pipe sample can be, just produce a better point sample
    gboolean show_preview_pixel_scale = TRUE;

    // overlays are aligned with pixels for a clean look
    if(sample->size == DT_LIB_COLORPICKER_SIZE_BOX)
    {
      double x = sample->box[0] * wd, y = sample->box[1] * ht,
        w = sample->box[2] * wd, h = sample->box[3] * ht;
      cairo_user_to_device(cri, &x, &y);
      cairo_user_to_device(cri, &w, &h);
      x=round(x+0.5)-0.5;
      y=round(y+0.5)-0.5;
      w=round(w+0.5)-0.5;
      h=round(h+0.5)-0.5;
      cairo_device_to_user(cri, &x, &y);
      cairo_device_to_user(cri, &w, &h);
      cairo_rectangle(cri, x, y, w - x, h - y);
      if(is_primary_sample)
      {
        // handles
        const double hw = 5. / scale;
        cairo_rectangle(cri, x - hw, y - hw, 2. * hw, 2. * hw);
        cairo_rectangle(cri, x - hw, h - hw, 2. * hw, 2. * hw);
        cairo_rectangle(cri, w - hw, y - hw, 2. * hw, 2. * hw);
        cairo_rectangle(cri, w - hw, h - hw, 2. * hw, 2. * hw);
      }
    }
    else if(sample->size == DT_LIB_COLORPICKER_SIZE_POINT)
    {
      // FIXME: to be really accurate, the colorpicker should render precisely over the nearest pixelpipe pixel, but this gets particularly tricky to do with iop pickers with transformations after them in the pipeline
      double x = sample->point[0] * wd;
      double y = sample->point[1] * ht;
      cairo_user_to_device(cri, &x, &y);
      x=round(x+0.5)-0.5;
      y=round(y+0.5)-0.5;
      // render picker center a reasonable size in device pixels
      half_px = round(half_px * scale);
      if(half_px < min_half_px_device)
      {
        half_px = min_half_px_device;
        show_preview_pixel_scale = FALSE;
      }
      // crosshair radius
      double crosshair = (is_primary_sample ? 4. : 5.) * half_px;
      if(sample == selected_sample) crosshair *= 2;
      cairo_device_to_user(cri, &x, &y);
      cairo_device_to_user_distance(cri, &crosshair, &half_px);

      // "handles"
      if(is_primary_sample)
        cairo_arc(cri, x, y, crosshair, 0., 2. * M_PI);
      // crosshair
      cairo_move_to(cri, x - crosshair, y);
      cairo_line_to(cri, x + crosshair, y);
      cairo_move_to(cri, x, y - crosshair);
      cairo_line_to(cri, x, y + crosshair);
    }

    // default is to draw 1 (logical) pixel light lines with 1
    // (logical) pixel dark outline for legibility
    const double line_scale = (sample == selected_sample ? 2.0 : 1.0);
    cairo_set_line_width(cri, lw * 3.0 * line_scale);
    cairo_set_source_rgba(cri, 0.0, 0.0, 0.0, 0.4);
    cairo_stroke_preserve(cri);

    const gboolean draw_dashed = !is_primary_sample
                   && sample != selected_sample
                   && sample->size == DT_LIB_COLORPICKER_SIZE_BOX;
    cairo_set_line_width(cri, lw * line_scale);
    cairo_set_dash(cri, dashes, draw_dashed, 0.0);

    cairo_set_source_rgba(cri, 1.0, 1.0, 1.0, 0.8);
    cairo_stroke(cri);

    // draw the actual color sampled
    // FIXME: if an area sample is selected, when selected should fill it with colorpicker color?
    // NOTE: The sample may be based on outdated data, but still
    // display as it will update eventually. If we only drew on valid
    // data, swatches on point live samples would flicker when the
    // primary sample was drawn, and the primary sample swatch would
    // flicker when an iop is adjusted.
    if(sample->size == DT_LIB_COLORPICKER_SIZE_POINT)
    {
      if(sample == selected_sample)
        cairo_arc(cri, sample->point[0] * wd, sample->point[1] * ht, half_px * 2., 0., 2. * M_PI);
      else if(show_preview_pixel_scale)
        cairo_rectangle(cri, sample->point[0] * wd - half_px, sample->point[1] * ht - half_px, half_px * 2., half_px * 2.);
      else
        cairo_arc(cri, sample->point[0] * wd, sample->point[1] * ht, half_px, 0., 2. * M_PI);

      set_color(cri, sample->swatch);
      cairo_fill(cri);
    }
  }

  cairo_restore(cri);
}

void _colormanage_ui_color(const float L, const float a, const float b, dt_aligned_pixel_t RGB)
{
  dt_aligned_pixel_t Lab = { L, a, b, 1.f };
  dt_aligned_pixel_t XYZ = { 0.f, 0.f, 0.f, 1.f };
  dt_Lab_to_XYZ(Lab, XYZ);
  cmsDoTransform(darktable.color_profiles->transform_xyz_to_display, XYZ, RGB, 1);
}

static void _render_iso12646(cairo_t *cr, int width, int height, int border)
{
  // draw the white frame around picture
  cairo_rectangle(cr, -border * .5f, -border * .5f, width + border, height + border);
  cairo_set_source_rgb(cr, 1., 1., 1.);
  cairo_fill(cr);
}

/* Debug-only darkroom expose mode:
 * - no persistent surface locks,
 * - no fallback cache,
 * - no preview substitution,
 * - only main backbuffer if currently available.
 *
 * Set to 1 for baseline debugging when troubleshooting display regressions. */
#ifndef DARKROOM_EXPOSE_DUMB_DEBUG
#define DARKROOM_EXPOSE_DUMB_DEBUG 0
#endif

#if DARKROOM_EXPOSE_DUMB_DEBUG
static gboolean _render_main_direct_debug(cairo_t *cr, dt_develop_t *dev, const int width, const int height,
                                          const int border, const dt_aligned_pixel_t bg_color)
{
  if(IS_NULL_PTR(cr) || IS_NULL_PTR(dev) || IS_NULL_PTR(dev->pipe)) return FALSE;

  cairo_set_source_rgb(cr, bg_color[0], bg_color[1], bg_color[2]);
  cairo_paint(cr);

  if(!dt_dev_pixelpipe_is_backbufer_valid(dev->pipe, dev)) return FALSE;
  const uint64_t hash = dt_dev_backbuf_get_hash(&dev->pipe->backbuf);
  if(hash == (uint64_t)-1) return FALSE;

  dt_pixel_cache_entry_t *entry = NULL;
  void *data = NULL;
  if(!dt_dev_pixelpipe_cache_peek_gui(dev->pipe, NULL, &data, &entry, NULL, NULL, NULL))
    return FALSE;

  dt_dev_pixelpipe_cache_rdlock_entry(darktable.pixelpipe_cache, TRUE, entry);

  const int bw = (int)dev->pipe->backbuf.width;
  const int bh = (int)dev->pipe->backbuf.height;
  if(bw <= 0 || bh <= 0)
  {
    dt_dev_pixelpipe_cache_rdlock_entry(darktable.pixelpipe_cache, FALSE, entry);
    return FALSE;
  }

  const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, bw);
  const size_t required_size = (size_t)stride * (size_t)bh;
  const size_t entry_size = dt_pixel_cache_entry_get_size(entry);
  if(entry_size < required_size || dt_pixel_cache_entry_get_data(entry) != data)
  {
    dt_dev_pixelpipe_cache_rdlock_entry(darktable.pixelpipe_cache, FALSE, entry);
    return FALSE;
  }

  cairo_surface_t *surface = cairo_image_surface_create_for_data((unsigned char *)data, CAIRO_FORMAT_RGB24,
                                                                  bw, bh, stride);
  if(IS_NULL_PTR(surface) || cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
  {
    if(surface) cairo_surface_destroy(surface);
    dt_dev_pixelpipe_cache_rdlock_entry(darktable.pixelpipe_cache, FALSE, entry);
    return FALSE;
  }

  float image_box[4] = { 0.0f };
  dt_dev_get_image_box_in_widget(dev, width, height, image_box);
  const int wd = image_box[2];
  const int ht = image_box[3];
  cairo_translate(cr, image_box[0], image_box[1]);
  if(dev->iso_12646.enabled) _render_iso12646(cr, wd, ht, border);
  cairo_surface_set_device_scale(surface, darktable.gui->ppd, darktable.gui->ppd);
  cairo_rectangle(cr, 0, 0, wd, ht);
  cairo_set_source_surface(cr, surface, 0, 0);
  cairo_fill(cr);

  cairo_surface_destroy(surface);
  dt_dev_pixelpipe_cache_rdlock_entry(darktable.pixelpipe_cache, FALSE, entry);
  return TRUE;
}
#endif

typedef struct darkroom_locked_surface_t
{
  uint64_t hash;
  int width;
  int height;
  void *data;
  struct dt_pixel_cache_entry_t *entry;
  cairo_surface_t *surface;
} darkroom_locked_surface_t;

static darkroom_locked_surface_t _darkroom_main_locked = { .hash = (uint64_t)-1 };
static darkroom_locked_surface_t _darkroom_preview_locked = { .hash = (uint64_t)-1 };
static cairo_surface_t *_darkroom_preview_fallback_surface = NULL;
static int32_t _darkroom_preview_fallback_imgid = UNKNOWN_IMAGE;
static uint64_t _darkroom_preview_fallback_zoom_hash = 0;
static uint64_t _darkroom_preview_fallback_backbuf_hash = 0;
static int _darkroom_preview_fallback_width = 0;
static int _darkroom_preview_fallback_height = 0;

static void _release_locked_surface(darkroom_locked_surface_t *locked)
{
  if(IS_NULL_PTR(locked)) return;

  if(locked->surface)
  {
    cairo_surface_destroy(locked->surface);
    locked->surface = NULL;
  }

  /* These cairo views only mirror whatever cacheline the pipeline currently exposes as backbuffer.
   * They never own the backbuffer keepalive ref themselves: `pixelpipe_hb.c` swaps that ownership
   * when publishing a new backbuffer. Releasing the surface must therefore only drop the GUI view. */
  locked->entry = NULL;
  locked->data = NULL;
  locked->hash = (uint64_t)-1;
  locked->width = 0;
  locked->height = 0;
}

static void _release_preview_fallback_surface(void)
{
  if(_darkroom_preview_fallback_surface)
  {
    cairo_surface_destroy(_darkroom_preview_fallback_surface);
    _darkroom_preview_fallback_surface = NULL;
  }

  _darkroom_preview_fallback_imgid = UNKNOWN_IMAGE;
  _darkroom_preview_fallback_zoom_hash = 0;
  _darkroom_preview_fallback_backbuf_hash = 0;
  _darkroom_preview_fallback_width = 0;
  _darkroom_preview_fallback_height = 0;
}

static void _release_expose_source_caches(void)
{
  _release_locked_surface(&_darkroom_main_locked);
  _release_locked_surface(&_darkroom_preview_locked);
  _release_preview_fallback_surface();
}

static gboolean _lock_pipe_surface(dt_develop_t *dev, dt_dev_pixelpipe_t *pipe, darkroom_locked_surface_t *locked,
                                   const gboolean keep_previous_on_fail, const gboolean lock_read)
{
  if(IS_NULL_PTR(dev) || IS_NULL_PTR(pipe) || !locked) return FALSE;
  (void)lock_read;

  const uint64_t hash = dt_dev_backbuf_get_hash(&pipe->backbuf);
  if(hash == (uint64_t)-1) return keep_previous_on_fail && (!IS_NULL_PTR(locked->surface));

  /* Fast-path reuse is only valid if the cacheline identity/data pointer are
   * still the same behind the hash key. This avoids reusing stale cairo views
   * when an entry is recreated or replaced under the same key. */
  dt_pixel_cache_entry_t *live_entry = NULL;
  void *live_data = NULL;
  if(locked->surface && locked->hash == hash
     && dt_dev_pixelpipe_cache_peek_gui(pipe, NULL, &live_data, &live_entry, NULL, NULL, NULL)
     && live_entry == locked->entry && live_data == locked->data)
  {
    locked->width = pipe->backbuf.width;
    locked->height = pipe->backbuf.height;
    return TRUE;
  }

  struct dt_pixel_cache_entry_t *entry = NULL;
  /* GUI surfaces only borrow the currently published backbuffer. They rely on the backbuffer keepalive ref
   * owned by `pixelpipe_hb.c`, so they must not take or drop their own cache refs here. */
  void *data = NULL;
  if(!dt_dev_pixelpipe_cache_peek_gui(pipe, NULL, &data, &entry, NULL, NULL, NULL))
    data = NULL;
  if(IS_NULL_PTR(data))
  {
    /* Keep previous frame only while waiting for a *different* target hash.
     * If requested hash equals the currently locked one but cache lookup fails,
     * the cached line was likely flushed/invalidated: drop stale lock so the
     * line can be recreated and displayed again. */
    if(keep_previous_on_fail && locked->surface && locked->hash != hash) return TRUE;
    _release_locked_surface(locked);
    return FALSE;
  }

  const int width = pipe->backbuf.width;
  const int height = pipe->backbuf.height;
  const int stride = cairo_format_stride_for_width(CAIRO_FORMAT_RGB24, width);
  const size_t required_size = (size_t)stride * (size_t)height;
  const size_t entry_size = dt_pixel_cache_entry_get_size(entry);
  if(width <= 0 || height <= 0 || entry_size < required_size || dt_pixel_cache_entry_get_data(entry) != data)
  {
    if(keep_previous_on_fail && locked->surface && locked->hash != hash) return TRUE;
    _release_locked_surface(locked);
    return FALSE;
  }

  if(locked->surface && locked->data == data && locked->width == width && locked->height == height)
  {
    locked->hash = hash;
    locked->entry = entry;
    locked->data = data;
    return TRUE;
  }

  cairo_surface_t *surface = cairo_image_surface_create_for_data(data, CAIRO_FORMAT_RGB24, width, height, stride);
  if(IS_NULL_PTR(surface) || cairo_surface_status(surface) != CAIRO_STATUS_SUCCESS)
  {
    if(surface) cairo_surface_destroy(surface);
    if(keep_previous_on_fail && locked->surface) return TRUE;
    _release_locked_surface(locked);
    return FALSE;
  }

  _release_locked_surface(locked);
  locked->hash = hash;
  locked->width = width;
  locked->height = height;
  locked->data = data;
  locked->entry = entry;
  locked->surface = surface;
  return TRUE;
}

static gboolean _render_main_locked_surface(cairo_t *cr, dt_develop_t *dev, darkroom_locked_surface_t *locked,
                                            const int width, const int height, const int border,
                                            const dt_aligned_pixel_t bg_color)
{
  if(IS_NULL_PTR(cr) || IS_NULL_PTR(dev) || !locked || !locked->surface) return FALSE;
  if(!locked->entry || locked->hash == (uint64_t)-1) return FALSE;

  cairo_set_source_rgb(cr, bg_color[0], bg_color[1], bg_color[2]);
  cairo_paint(cr);

  int wd = locked->width;
  int ht = locked->height;
  if(wd <= 0 || ht <= 0) return FALSE;

  wd /= darktable.gui->ppd;
  ht /= darktable.gui->ppd;
  cairo_translate(cr, .5f * (width - wd), .5f * (height - ht));

  if(dev->iso_12646.enabled) _render_iso12646(cr, wd, ht, border);

  dt_dev_pixelpipe_cache_rdlock_entry(darktable.pixelpipe_cache, TRUE, locked->entry);
  cairo_surface_set_device_scale(locked->surface, darktable.gui->ppd, darktable.gui->ppd);
  cairo_rectangle(cr, 0, 0, wd, ht);
  cairo_set_source_surface(cr, locked->surface, 0, 0);
  cairo_fill(cr);
  dt_dev_pixelpipe_cache_rdlock_entry(darktable.pixelpipe_cache, FALSE, locked->entry);

  return TRUE;
}

static gboolean _build_preview_fallback_surface(dt_develop_t *dev, const int width, const int height, const int border,
                                                const dt_aligned_pixel_t bg_color, const uint64_t zoom_hash)
{
  if(IS_NULL_PTR(_darkroom_preview_locked.surface)) return FALSE;
  if(!_darkroom_preview_locked.entry || _darkroom_preview_locked.hash == (uint64_t)-1) return FALSE;
  if(width <= 0 || height <= 0) return FALSE;

  if(!_darkroom_preview_fallback_surface
     || _darkroom_preview_fallback_width != width
     || _darkroom_preview_fallback_height != height)
  {
    _release_preview_fallback_surface();
    _darkroom_preview_fallback_surface = dt_cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
    if(IS_NULL_PTR(_darkroom_preview_fallback_surface)) return FALSE;
    _darkroom_preview_fallback_width = width;
    _darkroom_preview_fallback_height = height;
  }

  cairo_t *cr = cairo_create(_darkroom_preview_fallback_surface);
  cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);

  cairo_set_source_rgb(cr, bg_color[0], bg_color[1], bg_color[2]);
  cairo_paint(cr);

  const int wd = _darkroom_preview_locked.width;
  const int ht = _darkroom_preview_locked.height;
  const float ppd = darktable.gui->ppd;
  const float preview_wd = wd / ppd;
  const float preview_ht = ht / ppd;
  const float preview_scale = dev->roi.scaling;

  if(dev->iso_12646.enabled)
  {
    // The preview backbuffer is already a full-image fit render. Reprojecting it
    // for temporary zoom/pan feedback therefore only needs the extra darkroom
    // zoom factor, not another full processed-image rescale.
    const float roi_wd = fminf(preview_wd * preview_scale, width);
    const float roi_ht = fminf(preview_ht * preview_scale, height);

    cairo_save(cr);
    cairo_translate(cr, .5f * (width - roi_wd), .5f * (height - roi_ht));
    _render_iso12646(cr, roi_wd, roi_ht, border);
    cairo_restore(cr);
  }

  dt_dev_pixelpipe_cache_rdlock_entry(darktable.pixelpipe_cache, TRUE, _darkroom_preview_locked.entry);
  cairo_surface_set_device_scale(_darkroom_preview_locked.surface, ppd, ppd);

  // The preview surface already embeds the fit-to-window scale. To emulate the
  // main pipe while it catches up, we only apply the additional darkroom zoom
  // and pan around the preview image center in GUI logical coordinates.
  const float roi_width = fminf(width, preview_wd * preview_scale);
  const float roi_height = fminf(height, preview_ht * preview_scale);
  const float rec_x = fmaxf(border, (width - roi_width) * 0.5f);
  const float rec_y = fmaxf(border, (height - roi_height) * 0.5f);
  const float rec_w = fminf(width - 2 * border, roi_width);
  const float rec_h = fminf(height - 2 * border, roi_height);
  cairo_rectangle(cr, rec_x, rec_y, rec_w, rec_h);
  cairo_clip(cr);

  const float tx = 0.5f * width - dev->roi.x * preview_wd * preview_scale;
  const float ty = 0.5f * height - dev->roi.y * preview_ht * preview_scale;
  cairo_translate(cr, tx, ty);
  cairo_scale(cr, preview_scale, preview_scale);
  cairo_rectangle(cr, 0, 0, preview_wd, preview_ht);
  cairo_set_source_surface(cr, _darkroom_preview_locked.surface, 0, 0);
  cairo_fill(cr);
  dt_dev_pixelpipe_cache_rdlock_entry(darktable.pixelpipe_cache, FALSE, _darkroom_preview_locked.entry);
  cairo_destroy(cr);

  _darkroom_preview_fallback_imgid = dev->image_storage.id;
  _darkroom_preview_fallback_zoom_hash = zoom_hash;
  _darkroom_preview_fallback_backbuf_hash = dt_dev_backbuf_get_hash(&dev->preview_pipe->backbuf);
  return TRUE;
}

static gboolean _render_preview_fallback_surface(cairo_t *cr)
{
  if(IS_NULL_PTR(cr) || !_darkroom_preview_fallback_surface) return FALSE;
  cairo_set_source_surface(cr, _darkroom_preview_fallback_surface, 0, 0);
  cairo_paint(cr);
  return TRUE;
}

static inline gboolean _darkroom_gui_module_requests_uncropped_full_image(const dt_develop_t *dev)
{
  return dev && dev->gui_attached && dev->gui_module && dt_iop_get_cache_bypass(dev->gui_module);
}

static void _paint_all(cairo_t *cri, cairo_t *cr, cairo_surface_t *image_surface)
{
  cairo_destroy(cr);
  if(IS_NULL_PTR(image_surface)) return;
  cairo_set_source_surface(cri, image_surface, 0, 0);
  cairo_paint(cri);
}

typedef struct darkroom_expose_state_t
{
  int image_surface_width;
  int image_surface_height;
  int32_t image_surface_imgid;
  gboolean image_surface_has_main;
  uint64_t main_zoom_hash;
  uint64_t main_hash;
} darkroom_expose_state_t;

static inline gboolean _darkroom_preview_fallback_valid(const dt_develop_t *dev, const int width,
                                                        const int height, const uint64_t zoom_hash)
{
  return dev
         && _darkroom_preview_fallback_surface
         && _darkroom_preview_fallback_imgid == dev->image_storage.id
         && _darkroom_preview_fallback_zoom_hash == zoom_hash
         && _darkroom_preview_fallback_width == width
         && _darkroom_preview_fallback_height == height;
}

static inline gboolean _darkroom_locked_main_valid_for_zoom(const darkroom_expose_state_t *state,
                                                            const uint64_t zoom_hash)
{
  return state
         && _darkroom_main_locked.surface
         && _darkroom_main_locked.hash != (uint64_t)-1
         && state->main_zoom_hash == zoom_hash;
}

static inline void _darkroom_reset_expose_state(darkroom_expose_state_t *state)
{
  if(IS_NULL_PTR(state)) return;
  state->image_surface_imgid = UNKNOWN_IMAGE;
  state->image_surface_has_main = FALSE;
  state->main_zoom_hash = 0;
  state->main_hash = 0;
}

static void _darkroom_prepare_image_surface(dt_develop_t *dev, const int width, const int height,
                                            darkroom_expose_state_t *state)
{
  if(IS_NULL_PTR(dev) || !state) return;

  if(state->image_surface_imgid != dev->image_storage.id
     && state->image_surface_imgid != UNKNOWN_IMAGE)
  {
    _release_expose_source_caches();
    _darkroom_reset_expose_state(state);
  }

  if(state->image_surface_width == width
     && state->image_surface_height == height
     && !IS_NULL_PTR(dev->image_surface))
    return;

  state->image_surface_width = width;
  state->image_surface_height = height;
  if(dev->image_surface) cairo_surface_destroy(dev->image_surface);
  dev->image_surface = dt_cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
  state->image_surface_imgid = UNKNOWN_IMAGE;
  state->image_surface_has_main = FALSE;
  _release_preview_fallback_surface();
}

void expose(
    dt_view_t *self,
    cairo_t *cri,
    int32_t width,
    int32_t height,
    int32_t pointerx,
    int32_t pointery)
{
  dt_times_t start;
  dt_get_times(&start);

  cairo_save(cri);

  dt_develop_t *dev = (dt_develop_t *)self->data;

  const int32_t border = dev->roi.border_size;

#if DARKROOM_EXPOSE_DUMB_DEBUG
  dt_aligned_pixel_t bg_color_dbg;
  if(dev->iso_12646.enabled)
    _colormanage_ui_color(50., 0., 0., bg_color_dbg);
  else
    _colormanage_ui_color((float)dt_conf_get_int("display/brightness"), 0., 0., bg_color_dbg);
  _render_main_direct_debug(cri, dev, width, height, border, bg_color_dbg);
  cairo_restore(cri);
  return;
#endif
  static darkroom_expose_state_t expose_state = {
    .image_surface_width = 0,
    .image_surface_height = 0,
    .image_surface_imgid = UNKNOWN_IMAGE,
    .image_surface_has_main = FALSE,
    .main_zoom_hash = 0,
    .main_hash = 0,
  };
  const uint64_t zoom_hash = dt_hash(5381, (char *)&dev->roi, sizeof(dev->roi));
  const gboolean allow_uncropped_full_image = _darkroom_gui_module_requests_uncropped_full_image(dev);
  const gboolean roi_changed = !_darkroom_locked_main_valid_for_zoom(&expose_state, zoom_hash);

  _darkroom_prepare_image_surface(dev, width, height, &expose_state);

  cairo_t *cr = cairo_create(dev->image_surface);
  const int full_width = dev->roi.preview_width;
  const int full_height = dev->roi.preview_height;
  const uint64_t main_backbuf_hash = dt_dev_backbuf_get_hash(&dev->pipe->backbuf);
  const uint64_t preview_backbuf_hash = dt_dev_backbuf_get_hash(&dev->preview_pipe->backbuf);
  const gboolean main_has_backbuf = main_backbuf_hash != DT_PIXELPIPE_CACHE_HASH_INVALID;
  const gboolean preview_has_backbuf = preview_backbuf_hash != DT_PIXELPIPE_CACHE_HASH_INVALID;
  const gboolean main_backbuf_is_newer = main_has_backbuf && main_backbuf_hash != expose_state.main_hash;
  const gboolean main_ready_for_current_view
      = main_has_backbuf && (!roi_changed || main_backbuf_is_newer || !_darkroom_main_locked.surface);
  const gboolean showing_full_image = allow_uncropped_full_image || fabsf(dev->roi.scaling - 1.0f) < 1e-6f;
  const gboolean preview_matches_full_image
      = preview_has_backbuf && showing_full_image
        && (allow_uncropped_full_image
            || (full_width > 0 && full_height > 0
                && dev->preview_pipe->backbuf.width == full_width
                && dev->preview_pipe->backbuf.height == full_height));
  const gboolean full_image_backbuf_ready = main_ready_for_current_view || preview_matches_full_image;

  dt_aligned_pixel_t bg_color = { 0.0f };
  const char *draw_source = "background only";
  uint64_t draw_hash = (uint64_t)-1;
  gboolean drawn = FALSE;
  gboolean drawn_from_main = FALSE;

  // user param is Lab lightness/brightness (which is they same for greys)
  if(dev->iso_12646.enabled)
    _colormanage_ui_color(50., 0., 0., bg_color);
  else
    _colormanage_ui_color((float)dt_conf_get_int("display/brightness"), 0., 0., bg_color);

  cairo_pattern_set_filter(cairo_get_source(cr), CAIRO_FILTER_NEAREST);

  /* Selection policy, kept intentionally linear:
   * 1. Prefer the main pipe whenever it already has the backbuf for the
   *    current darkroom view. We do not guess that from size. If ROI did not
   *    change, the existing main frame stays authoritative. If ROI changed, a
   *    new main backbuf hash means the updated main frame arrived.
   * 2. If preview produced the exact same full-image buffer size first
   *    (zoom-to-fit / same ROI), treat it exactly like main.
   * 3. Only when zoom/pan changed and main has not caught up yet, use a scaled
   *    preview fallback surface. This gives immediate ROI feedback without
   *    discarding the last valid main image for pure history changes.
   * 4. If nothing newer is ready, keep showing the last valid composed frame to
   *    avoid flashing the background. */

  /* Once a full-image backbuf exists again, the ROI-scaled preview fallback is
   * obsolete. Keep main/preview selection simple and never let the fallback
   * outlive a valid full-image source. */
  if(full_image_backbuf_ready)
    _release_preview_fallback_surface();

  /* Rule 1: main wins whenever it is ready for the current view. During a
   * zoom/pan/widget-size transition, the previous main hash stays visible until
   * a different main hash is published; only then does main override preview
   * fallback again. */
  if(main_ready_for_current_view)
  {
    if(_lock_pipe_surface(dev, dev->pipe, &_darkroom_main_locked, FALSE, FALSE)
       && _darkroom_main_locked.surface
       && _render_main_locked_surface(cr, dev, &_darkroom_main_locked, width, height, border, bg_color))
    {
      expose_state.main_hash = _darkroom_main_locked.hash;
      expose_state.main_zoom_hash = zoom_hash;
      expose_state.image_surface_imgid = dev->image_storage.id;
      expose_state.image_surface_has_main = TRUE;
      _release_preview_fallback_surface();
      drawn = TRUE;
      drawn_from_main = TRUE;
      draw_source = "fresh main backbuf";
      draw_hash = _darkroom_main_locked.hash;
    }
    else if(main_backbuf_is_newer)
      dt_control_queue_redraw_center();
  }

  /* Rule 2: preview is directly equivalent to main only in full-image view
   * (fit scale / uncropped edit modes). Otherwise it must go through the
   * ROI-scaled fallback path below. */
  if(!drawn && preview_matches_full_image)
  {
    if(_lock_pipe_surface(dev, dev->preview_pipe, &_darkroom_preview_locked, FALSE, TRUE)
       && _darkroom_preview_locked.surface
       && _render_main_locked_surface(cr, dev, &_darkroom_preview_locked, width, height, border, bg_color))
    {
      expose_state.main_hash = _darkroom_preview_locked.hash;
      expose_state.main_zoom_hash = zoom_hash;
      expose_state.image_surface_imgid = dev->image_storage.id;
      expose_state.image_surface_has_main = TRUE;
      _release_preview_fallback_surface();
      drawn = TRUE;
      drawn_from_main = TRUE;
      draw_source = "fresh full-size preview backbuf";
      draw_hash = _darkroom_preview_locked.hash;
    }
  }

  /* Rule 3: scaled preview fallback is only for ROI changes. If the user just
   * zoomed/panned and main has not recomputed yet, rebuild a ROI-scaled preview
   * placeholder so navigation remains responsive. */
  if(!drawn && roi_changed && !full_image_backbuf_ready && preview_has_backbuf)
  {
    if(_lock_pipe_surface(dev, dev->preview_pipe, &_darkroom_preview_locked, TRUE, TRUE)
       && _build_preview_fallback_surface(dev, width, height, border, bg_color, zoom_hash)
       && _render_preview_fallback_surface(cr))
    {
      expose_state.image_surface_imgid = dev->image_storage.id;
      expose_state.image_surface_has_main = FALSE;
      drawn = TRUE;
      drawn_from_main = FALSE;
      draw_source = "fresh preview fallback";
      draw_hash = _darkroom_preview_fallback_backbuf_hash;
    }
  }

  /* Reuse the cached ROI-scaled preview fallback if it is still geometrically
   * valid for this zoom/pan. This avoids rebuilding it every expose while main
   * is still catching up. */
  if(!drawn && roi_changed && !full_image_backbuf_ready
     && _darkroom_preview_fallback_valid(dev, width, height, zoom_hash)
     && _render_preview_fallback_surface(cr))
  {
    expose_state.image_surface_imgid = dev->image_storage.id;
    expose_state.image_surface_has_main = FALSE;
    drawn = TRUE;
    drawn_from_main = FALSE;
    draw_source = "reused preview fallback";
    draw_hash = _darkroom_preview_fallback_backbuf_hash;
  }

  /* Rule 4: if no newer source is ready, keep the last valid main frame for
   * the same zoom/pan. History-only changes should not glitch to preview or
   * background while the new main backbuf is still being produced. */
  if(!drawn && _darkroom_locked_main_valid_for_zoom(&expose_state, zoom_hash)
     && _render_main_locked_surface(cr, dev, &_darkroom_main_locked, width, height, border, bg_color))
  {
    expose_state.image_surface_imgid = dev->image_storage.id;
    expose_state.image_surface_has_main = TRUE;
    drawn = TRUE;
    drawn_from_main = TRUE;
    draw_source = "reused last main backbuf";
    draw_hash = _darkroom_main_locked.hash;
  }

  if(drawn)
  {
    /* Persist the last composed frame so later exposes can reuse it if the
     * next requested source is temporarily unavailable. */
    expose_state.image_surface_has_main = drawn_from_main;
    _paint_all(cri, cr, dev->image_surface);
    dt_print(DT_DEBUG_DEV, "[darkroom] expose drew %s (backbuf hash=%" PRIu64 ")\n",
             draw_source, draw_hash);
    cairo_restore(cri);
  }
  else if(dev->image_surface && expose_state.image_surface_imgid == dev->image_storage.id)
  {
    /* No fresh source this time: repaint the last composed image surface
     * rather than clearing to background and waiting for another expose. */
    draw_source = expose_state.image_surface_has_main ? "reused last main surface"
                                                      : "reused last preview surface";
    draw_hash = expose_state.image_surface_has_main ? expose_state.main_hash
                                                    : _darkroom_preview_fallback_backbuf_hash;
    _paint_all(cri, cr, dev->image_surface);
    dt_print(DT_DEBUG_DEV, "[darkroom] expose drew %s (backbuf hash=%" PRIu64 ")\n",
             draw_source, draw_hash);
    cairo_restore(cri);
  }
  else
  {
    /* Cold-start / no valid frame cached anywhere yet. */
    cairo_set_source_rgb(cr, bg_color[0], bg_color[1], bg_color[2]);
    cairo_paint(cr);
    _paint_all(cri, cr, dev->image_surface);
    dt_print(DT_DEBUG_DEV, "[darkroom] expose drew %s (backbuf hash=%" PRIu64 ")\n",
             draw_source, draw_hash);
    cairo_restore(cri);
  }

  /* check if we should create a snapshot of view */
  if(darktable.develop->proxy.snapshot.request)
  {
    /* reset the request */
    darktable.develop->proxy.snapshot.request = FALSE;

    /* validation of snapshot filename */
    g_assert(!IS_NULL_PTR(darktable.develop->proxy.snapshot.filename));

    /* Store current image surface to snapshot file.
       FIXME: add checks so that we don't make snapshots of preview pipe image surface.
    */
    const int fd = g_open(darktable.develop->proxy.snapshot.filename, O_CREAT | O_WRONLY | O_BINARY, 0600);
    cairo_surface_write_to_png_stream(dev->image_surface, _write_snapshot_data, GINT_TO_POINTER(fd));
    close(fd);
  }

  // Displaying sample areas if enabled
  if(darktable.develop->color_picker.samples
     && (darktable.develop->color_picker.display_samples
         || (darktable.develop->color_picker.selected_sample &&
             darktable.develop->color_picker.selected_sample != darktable.develop->color_picker.primary_sample)))
  {
    _darkroom_pickers_draw(self, cri, width, height, pointerx, pointery, darktable.develop->color_picker.samples, FALSE);
  }

  // draw guide lines if needed
  if(!dev->gui_module || !(dev->gui_module->flags() & IOP_FLAGS_GUIDES_SPECIAL_DRAW))
  {
    const float wd = dev->roi.preview_width;
    const float ht = dev->roi.preview_height;
    const float scaling = dt_dev_get_overlay_scale(dev);

    cairo_save(cri);
    // don't draw guides on image margins
    dt_dev_clip_roi(dev, cri, width, height);
    // place origin at top-left corner of image
    dt_dev_rescale_roi(dev, cri, width, height);

    // draw guides with backbuffer dimensions, positioning and scaling handled by transformations
    dt_guides_draw(cri, 0, 0, wd, ht, scaling);
    cairo_restore(cri);
  }

  const gboolean picker_active = dt_iop_color_picker_is_visible(dev);

  // draw colorpicker for in focus module or execute module callback hook
  // FIXME: draw picker in gui_post_expose() hook in libs/colorpicker.c -- catch would be that live samples would appear over guides, softproof/gamut text overlay would be hidden by picker
  if(picker_active)
  {
    GSList samples = { .data = darktable.develop->color_picker.primary_sample, .next = NULL };
    _darkroom_pickers_draw(self, cri, width, height, pointerx, pointery, &samples, TRUE);
  }
  else
  {
    // display mask if we have a current module activated or if the masks manager module is expanded
    const gboolean display_masks = (dev->gui_module && dev->gui_module->enabled)
                                 || dt_lib_gui_get_expanded(dt_lib_get_module("masks"));

    if(dt_masks_get_visible_form(dev) && display_masks)
      dt_masks_events_post_expose(dev->gui_module, cri, width, height, pointerx, pointery);
      
    // module
    if(dev->gui_module && dev->gui_module->enabled && dev->gui_module->gui_post_expose)
      dev->gui_module->gui_post_expose(dev->gui_module, cri, width, height, pointerx, pointery);
  }

  // indicate if we are in gamut check or softproof mode
  if(darktable.color_profiles->mode != DT_PROFILE_NORMAL)
  {
    gchar *label = darktable.color_profiles->mode == DT_PROFILE_GAMUTCHECK ? _("gamut check") : _("soft proof");
    cairo_set_source_rgba(cri, 0.5, 0.5, 0.5, 0.5);
    PangoLayout *layout;
    PangoRectangle ink;
    PangoFontDescription *desc = pango_font_description_copy_static(darktable.bauhaus->pango_font_desc);
    pango_font_description_set_weight(desc, PANGO_WEIGHT_BOLD);
    layout = pango_cairo_create_layout(cri);
    pango_font_description_set_absolute_size(desc, DT_PIXEL_APPLY_DPI(20) * PANGO_SCALE);
    pango_layout_set_font_description(layout, desc);
    pango_layout_set_text(layout, label, -1);
    pango_layout_get_pixel_extents(layout, &ink, NULL);
    cairo_move_to(cri, ink.height * 2, height - (ink.height * 3));
    pango_cairo_layout_path(cri, layout);
    cairo_set_source_rgb(cri, 0.7, 0.7, 0.7);
    cairo_fill_preserve(cri);
    cairo_set_line_width(cri, 0.7);
    cairo_set_source_rgb(cri, 0.3, 0.3, 0.3);
    cairo_stroke(cri);
    pango_font_description_free(desc);
    g_object_unref(layout);
  }

  dt_show_times_f(&start, "[darkroom]", "redraw");
}

void reset(dt_view_t *self)
{
  dt_dev_reset_roi((dt_develop_t *)self->data);
}

static void _darkroom_log_image_load_error(const int ret)
{
  switch(ret)
  {
    case DT_DEV_IMAGE_STORAGE_MIPMAP_NOT_FOUND:
      dt_control_log(_("Could not load the image source data."));
      break;
    case DT_DEV_IMAGE_STORAGE_DB_NOT_READ:
      dt_control_log(_("Could not read image information from the database."));
      break;
    default:
      dt_control_log(_("We could not load the image."));
      break;
  }
}

static gboolean _darkroom_attach_missing_iop_guis(dt_develop_t *dev)
{
  if(IS_NULL_PTR(dev)) return FALSE;

  gboolean attached = FALSE;

  for(GList *modules = g_list_first(dev->iop); modules; modules = g_list_next(modules))
  {
    dt_iop_module_t *module = (dt_iop_module_t *)modules->data;

    if(!module || dt_iop_is_hidden(module) || module->expander) continue;

    if(!module->widget) dt_iop_gui_init(module);
    dt_iop_gui_set_expander(module);
    dt_iop_gui_update(module);
    attached = TRUE;
  }

  return attached;
}

static void _darkroom_image_loaded_callback(gpointer instance, guint request_id, guint result, gpointer user_data)
{
  dt_view_t *self = (dt_view_t *)user_data;
  dt_develop_t *dev = (dt_develop_t *)self->data;
  if(request_id == 0) return;
  if(darktable.view_manager->current_view != self) return;


  if(result)
  {
    _darkroom_log_image_load_error((int)result);
    return;
  }

  darktable.develop->proxy.wb_coeffs[0] = 0.f;

#ifdef USE_LUA

  dt_lua_async_call_alien(dt_lua_event_trigger_wrapper,
      0, NULL, NULL,
      LUA_ASYNC_TYPENAME, "const char*", "darkroom-image-loaded",
      LUA_ASYNC_TYPENAME, "dt_lua_image_t", GINT_TO_POINTER(dev->image_storage.id),
      LUA_ASYNC_DONE);

#endif

  // synch gui and flag pipe as dirty
  // this is done here and not in dt_read_history, as it would else be triggered before module->gui_init.
  // locks history mutex internally
  dt_dev_pop_history_items(dev);
  dt_dev_history_gui_update(dev);
  if(_darkroom_attach_missing_iop_guis(dev))
    dt_dev_history_gui_update(dev);

  dt_dev_pixelpipe_rebuild_all(dev);
  dt_dev_get_thumbnail_size(dev);

  if(_darkroom_pending_focus_module && g_list_find(dev->iop, _darkroom_pending_focus_module))
    dt_iop_request_focus(_darkroom_pending_focus_module);
  _darkroom_pending_focus_module = NULL;

  // Clean & Init the starting point of undo/redo
  dt_undo_clear(darktable.undo, DT_UNDO_DEVELOP);
  dt_dev_undo_start_record(dev);
  dt_dev_undo_end_record(dev);

  /* signal that darktable.develop is initialized and ready to be used */
  DT_DEBUG_CONTROL_SIGNAL_RAISE(darktable.signals, DT_SIGNAL_DEVELOP_INITIALIZE);

  dt_image_check_camera_missing_sample(&dev->image_storage);

  // clear selection, we don't want selections in darkroom
  dt_selection_clear(darktable.selection);

  // change active image for global actions (menu)
  dt_view_active_images_reset(FALSE);
  dt_view_active_images_add(dev->image_storage.id, TRUE);

  dt_control_queue_redraw_center();

  DT_DEBUG_CONTROL_SIGNAL_RAISE(darktable.signals, DT_SIGNAL_DEVELOP_IMAGE_CHANGED);

  dt_view_image_info_update(dev->image_storage.id);

  dt_dev_start_all_pipelines(dev);
}

static void _darkroom_ui_pipe_finish_signal_callback(gpointer instance, gpointer user_data)
{
  dt_view_t *self = (dt_view_t *)user_data;
  (void)instance;
  if(!self || !self->data)
    return;

  dt_develop_t *dev = (dt_develop_t *)self->data;
  dt_control_queue_redraw_center();

#if HAVE_ANSEL_AGENT_CHAT_BACKEND
  if(dev->agent_chat.pending_mid_turn_render && dev->agent_chat.active_request_id)
  {
    _agent_chat_encode_and_post_preview(dev);
    dev->agent_chat.pending_mid_turn_render = FALSE;
  }
#endif
}

int try_enter(dt_view_t *self)
{
  uint32_t num_selected = dt_selection_get_length(darktable.selection);
  int32_t imgid = dt_control_get_mouse_over_id();
  (void)self;

  _darkroom_pending_imgid = UNKNOWN_IMAGE;

  if(imgid != UNKNOWN_IMAGE)
  {
    ; // Needed to open image from filmstrip
  }
  else if(num_selected > 1)
  {
    dt_control_log(_("The current selection contains more than one image, which is ambiguous.\n"
                     "Select exactly one image to enter the darkroom."));
    return 1;
  }
  else if(num_selected == 0 && imgid == UNKNOWN_IMAGE)
  {
    dt_control_log(_("There is no image selected.\n"
                    "Select exactly one image to enter the darkroom."));
    return 1;
  }
  else
  {
    // Needed to open image at startup
    imgid = dt_selection_get_first_id(darktable.selection);
  }

  dt_view_active_images_reset(FALSE);

  if(imgid < 0)
  {
    // fail :(
    dt_control_log(_("No image to open !"));
    return 1;
  }

  _darkroom_pending_imgid = imgid;
  return 0;
}

static void _dev_change_image(dt_view_t *self, int32_t imgid)
{
  // Lazy trick to cleanup, reset, reinit, reload everything without
  // having to duplicate most of (but not all) the code in leave(),
  // try_enter() and enter() : simulate a roundtrip through lighttable.
  // This way, all images are loaded through the same path, handled at an higher level.
  // It's more robust, although slightly slower than re-initing only what is needed.
  dt_view_manager_switch(darktable.view_manager, "lighttable");
  dt_control_set_mouse_over_id(imgid);
  dt_view_manager_switch(darktable.view_manager, "darkroom");
}

static void _view_darkroom_filmstrip_activate_callback(gpointer instance, int32_t imgid, gpointer user_data)
{
  if(imgid > UNKNOWN_IMAGE)
  {
    // switch images in darkroom mode:
    _dev_change_image(user_data, imgid);
  }
}

/** toolbar buttons */

static gboolean _toolbar_show_popup(gpointer user_data)
{
  GtkPopover *popover = GTK_POPOVER(user_data);

  GtkWidget *button = gtk_popover_get_relative_to(popover);
  GdkRectangle button_rect = { 0 };
  GtkWidget *anchor = dt_gui_get_popup_relative_widget(button, &button_rect);
  GdkDevice *pointer = gdk_seat_get_pointer(gdk_display_get_default_seat(gdk_display_get_default()));

  int x, y;
  GdkWindow *pointer_window = gdk_device_get_window_at_position(pointer, &x, &y);
  gpointer   pointer_widget = NULL;
  if(pointer_window)
    gdk_window_get_user_data(pointer_window, &pointer_widget);

  gtk_popover_set_relative_to(popover, anchor ? anchor : button);

  GdkRectangle rect = { button_rect.x + button_rect.width / 2, button_rect.y, 1, 1 };

  if(pointer_widget == anchor)
  {
    rect.x = x;
    rect.y = y;
  }
  else if(pointer_widget && anchor && pointer_widget != anchor)
  {
    gtk_widget_translate_coordinates(pointer_widget, anchor, x, y, &rect.x, &rect.y);
  }

  gtk_popover_set_pointing_to(popover, &rect);

  // for the guides popover, it need to be updated before we show it
  if(darktable.view_manager && GTK_WIDGET(popover) == darktable.view_manager->guides_popover)
    dt_guides_update_popover_values();

  gtk_widget_show_all(GTK_WIDGET(popover));

  // cancel glib timeout if invoked by long button press
  return FALSE;
}

static dt_develop_t *_agent_chat_get_dev(void)
{
  if(!darktable.view_manager || !darktable.view_manager->proxy.darkroom.view)
    return NULL;

  return (dt_develop_t *)darktable.view_manager->proxy.darkroom.view->data;
}

static int32_t _agent_chat_current_image_id(void)
{
  return darktable.develop ? darktable.develop->image_storage.id : UNKNOWN_IMAGE;
}

static gboolean _agent_chat_scroll_to_end_idle(gpointer user_data)
{
  dt_develop_t *dev = (dt_develop_t *)user_data;
  if(!dev || !GTK_IS_WIDGET(dev->agent_chat.conversation_view))
    return G_SOURCE_REMOVE;

  GtkWidget *scroll = gtk_widget_get_ancestor(dev->agent_chat.conversation_view,
                                              GTK_TYPE_SCROLLED_WINDOW);
  if(GTK_IS_SCROLLED_WINDOW(scroll))
  {
    GtkAdjustment *adj = gtk_scrolled_window_get_vadjustment(GTK_SCROLLED_WINDOW(scroll));
    const gdouble bottom = MAX(0.0, gtk_adjustment_get_upper(adj) - gtk_adjustment_get_page_size(adj));
    gtk_adjustment_set_value(adj, bottom);
  }

  return G_SOURCE_REMOVE;
}

static void _agent_chat_scroll_to_end(dt_develop_t *dev)
{
  if(!dev || !GTK_IS_WIDGET(dev->agent_chat.conversation_view))
    return;

  g_idle_add(_agent_chat_scroll_to_end_idle, dev);
}

static void _agent_chat_replace_transcript(dt_develop_t *dev, const char *text)
{
  if(!dev || !GTK_IS_LABEL(dev->agent_chat.conversation_view))
    return;

  gtk_label_set_text(GTK_LABEL(dev->agent_chat.conversation_view), text ? text : "");
  _agent_chat_scroll_to_end(dev);
}

static void _agent_chat_append_message(dt_develop_t *dev, const char *speaker, const char *message)
{
  if(!dev || !GTK_IS_LABEL(dev->agent_chat.conversation_view))
    return;

  const char *existing = gtk_label_get_text(GTK_LABEL(dev->agent_chat.conversation_view));
  gchar *line = g_strdup_printf(_("%s: %s"), speaker ? speaker : "", message ? message : "");
  gchar *transcript = NULL;
  if(existing && existing[0] != '\0')
    transcript = g_strdup_printf("%s\n\n%s", existing, line);
  else
    transcript = g_strdup(line);

  gtk_label_set_text(GTK_LABEL(dev->agent_chat.conversation_view), transcript);
  g_free(transcript);
  g_free(line);
  _agent_chat_scroll_to_end(dev);
}

static void _agent_chat_set_status(dt_develop_t *dev, const char *status)
{
  if(dev && dev->agent_chat.status_label)
    gtk_label_set_text(GTK_LABEL(dev->agent_chat.status_label), status ? status : "");
}

static void _agent_chat_set_error(dt_develop_t *dev, const char *error)
{
  if(!dev || !dev->agent_chat.error_label)
    return;

  gtk_label_set_text(GTK_LABEL(dev->agent_chat.error_label), error ? error : "");
  gtk_widget_set_visible(dev->agent_chat.error_label, error && error[0] != '\0');
}

static void _agent_chat_update_sensitivity(dt_develop_t *dev)
{
  if(!dev)
    return;

  const gboolean backend_ready = HAVE_ANSEL_AGENT_CHAT_BACKEND;
  const gboolean ready = backend_ready && !dev->agent_chat.is_loading;

  if(dev->agent_chat.input_entry)
    gtk_widget_set_sensitive(dev->agent_chat.input_entry, ready);
  if(dev->agent_chat.send_button)
    gtk_widget_set_sensitive(dev->agent_chat.send_button, ready);
  if(dev->agent_chat.cancel_button)
    gtk_widget_set_sensitive(dev->agent_chat.cancel_button,
                             backend_ready && dev->agent_chat.active_request != NULL);
  if(dev->agent_chat.new_chat_button)
    gtk_widget_set_sensitive(dev->agent_chat.new_chat_button, backend_ready);
}

static void _agent_chat_set_loading(dt_develop_t *dev, gboolean is_loading)
{
  if(!dev)
    return;

  dev->agent_chat.is_loading = is_loading;
  if(dev->agent_chat.spinner)
  {
    gtk_widget_set_visible(dev->agent_chat.spinner, is_loading);
    if(is_loading)
      gtk_spinner_start(GTK_SPINNER(dev->agent_chat.spinner));
    else
      gtk_spinner_stop(GTK_SPINNER(dev->agent_chat.spinner));
  }

  _agent_chat_update_sensitivity(dev);
}

#if HAVE_ANSEL_AGENT_CHAT_BACKEND
static void _render_delivery_free(dt_agent_chat_render_delivery_t *delivery)
{
  if(!delivery)
    return;

  g_free(delivery->image_session_id);
  g_free(delivery->turn_id);
  g_free(delivery->jpeg_bytes);
  g_free(delivery->endpoint);
  g_free(delivery);
}

static gpointer _render_post_thread(gpointer user_data)
{
  dt_agent_chat_render_delivery_t *delivery = user_data;

  if(!delivery->jpeg_bytes || delivery->jpeg_len == 0)
  {
    _render_delivery_free(delivery);
    return NULL;
  }

  g_autofree gchar *render_endpoint = g_str_has_suffix(delivery->endpoint, "/")
                                        ? g_strconcat(delivery->endpoint, "render", NULL)
                                        : g_strconcat(delivery->endpoint, "/render", NULL);

  CURL *curl = curl_easy_init();
  if(!curl)
  {
    _render_delivery_free(delivery);
    return NULL;
  }

  g_autofree gchar *session_header
    = g_strdup_printf("X-Darktable-Image-Session-Id: %s",
                      delivery->image_session_id ? delivery->image_session_id : "");
  g_autofree gchar *turn_header
    = g_strdup_printf("X-Darktable-Turn-Id: %s",
                      delivery->turn_id ? delivery->turn_id : "");

  struct curl_slist *headers = NULL;
  headers = curl_slist_append(headers, "Content-Type: image/jpeg");
  headers = curl_slist_append(headers, session_header);
  headers = curl_slist_append(headers, turn_header);

  dt_curl_init(curl, FALSE);
  curl_easy_setopt(curl, CURLOPT_URL, render_endpoint);
  curl_easy_setopt(curl, CURLOPT_POST, 1L);
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, delivery->jpeg_bytes);
  curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)delivery->jpeg_len);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 15L);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

  curl_easy_perform(curl);

  curl_slist_free_all(headers);
  curl_easy_cleanup(curl);
  _render_delivery_free(delivery);
  return NULL;
}

static void _agent_chat_encode_and_post_preview(dt_develop_t *dev)
{
  if(!dev || !dev->agent_chat.image_session_id || !dev->agent_chat.active_request_id)
    return;

  dt_agent_image_state_t state;
  dt_agent_image_state_init(&state);

  GError *error = NULL;
  if(dt_agent_image_state_collect_from_dev(dev, &state, &error))
  {
    if(state.preview.available && state.preview.base64_data)
    {
      gsize decoded_len = 0;
      guchar *decoded = g_base64_decode(state.preview.base64_data, &decoded_len);
      if(decoded && decoded_len > 0)
      {
        dt_agent_chat_render_delivery_t *delivery = g_new0(dt_agent_chat_render_delivery_t, 1);
        delivery->image_session_id = g_strdup(dev->agent_chat.image_session_id);
        delivery->turn_id = g_strdup(dev->agent_chat.active_request_id);
        delivery->jpeg_bytes = decoded;
        delivery->jpeg_len = decoded_len;
        delivery->endpoint = dt_agent_client_dup_endpoint();
        g_thread_unref(g_thread_new("agent-chat-render", _render_post_thread, delivery));
      }
      else
      {
        g_free(decoded);
      }
    }
  }
  else
  {
    g_clear_error(&error);
  }

  dt_agent_image_state_clear(&state);
}

static void _agent_chat_submission_free(gpointer data)
{
  dt_agent_chat_submission_t *submission = (dt_agent_chat_submission_t *)data;
  if(!submission)
    return;

  g_free(submission->request_id);
  g_free(submission->prompt_text);
  g_free(submission->conversation_id);
  g_free(submission->image_session_id);
  g_free(submission);
}

static void _agent_chat_drop_active_request_handle(dt_develop_t *dev)
{
  if(!dev || !dev->agent_chat.active_request)
    return;

  dt_agent_client_request_unref(dev->agent_chat.active_request);
  dev->agent_chat.active_request = NULL;
}

static gboolean _agent_chat_submission_matches_active_request(const dt_develop_t *dev,
                                                              const dt_agent_chat_submission_t *submission)
{
  return dev && submission && dev->agent_chat.active_request_id
      && g_strcmp0(dev->agent_chat.active_request_id, submission->request_id) == 0;
}

static gboolean _agent_chat_submission_matches_current_session(const dt_develop_t *dev,
                                                               const dt_agent_chat_submission_t *submission)
{
  return dev && submission
      && dev->agent_chat.current_image_id == submission->image_id
      && g_strcmp0(dev->agent_chat.conversation_id, submission->conversation_id) == 0
      && g_strcmp0(dev->agent_chat.image_session_id, submission->image_session_id) == 0;
}

static void _agent_chat_set_active_request(dt_develop_t *dev,
                                           dt_agent_client_request_t *request,
                                           const char *request_id)
{
  if(!dev)
    return;

  _agent_chat_drop_active_request_handle(dev);
  g_free(dev->agent_chat.active_request_id);
  dev->agent_chat.active_request_id = g_strdup(request_id);
  dev->agent_chat.active_request = request;
  dev->agent_chat.pending_mid_turn_render = FALSE;
  dev->agent_chat.active_request_live_applied_count = 0;
  dev->agent_chat.active_request_tool_calls_used = 0;
  dev->agent_chat.active_request_tool_calls_max = 0;
}

static void _agent_chat_clear_active_request(dt_develop_t *dev)
{
  if(!dev)
    return;

  _agent_chat_drop_active_request_handle(dev);
  g_clear_pointer(&dev->agent_chat.active_request_id, g_free);
  dev->agent_chat.pending_mid_turn_render = FALSE;
  dev->agent_chat.active_request_live_applied_count = 0;
  dev->agent_chat.active_request_tool_calls_used = 0;
  dev->agent_chat.active_request_tool_calls_max = 0;
  _agent_chat_update_sensitivity(dev);
}

static gchar *_agent_chat_format_image_revision_id(const gboolean has_image_id,
                                                   const gint64 image_id,
                                                   const gint history_position,
                                                   const gint history_count)
{
  if(has_image_id && image_id > 0)
    return g_strdup_printf("image-%" G_GINT64_FORMAT "-history-%d-%d",
                           image_id, history_position, history_count);

  return g_strdup("image-unknown");
}

static void _agent_chat_fill_ui_context(dt_agent_chat_request_t *request)
{
  request->ui_context.view = g_strdup("darkroom");
  if(darktable.develop && darktable.develop->image_storage.id > 0)
  {
    request->ui_context.has_image_id = TRUE;
    request->ui_context.image_id = darktable.develop->image_storage.id;
  }
  if(darktable.develop && darktable.develop->image_storage.filename[0] != '\0')
    request->ui_context.image_name = g_strdup(darktable.develop->image_storage.filename);
}

static gboolean _agent_chat_build_request(dt_develop_t *dev,
                                          const char *message_text,
                                          dt_agent_chat_request_t *request,
                                          const gboolean prefer_cached_context,
                                          GError **error)
{
  dt_agent_chat_request_init(request);

  if(prefer_cached_context && _agent_chat_test_copy_cached_request(dev, message_text, request))
    return TRUE;

  if(!dev->agent_chat.app_session_id)
    dev->agent_chat.app_session_id = g_uuid_string_random();
  if(!dev->agent_chat.image_session_id)
    dev->agent_chat.image_session_id = g_uuid_string_random();
  if(!dev->agent_chat.conversation_id)
    dev->agent_chat.conversation_id = g_uuid_string_random();

  request->request_id = g_uuid_string_random();
  request->app_session_id = g_strdup(dev->agent_chat.app_session_id);
  request->image_session_id = g_strdup(dev->agent_chat.image_session_id);
  request->conversation_id = g_strdup(dev->agent_chat.conversation_id);
  request->turn_id = g_strdup(request->request_id);
  request->message_text = g_strdup(message_text);
  request->refinement_mode = DT_AGENT_REFINEMENT_MODE_MULTI;
  request->refinement_enabled = TRUE;
  request->refinement_pass_index = 1u;
  request->refinement_max_passes = DT_AGENT_CHAT_DEFAULT_MAX_REFINEMENT_TURNS;
  request->refinement_goal_text = g_strdup(message_text);
  _agent_chat_fill_ui_context(request);

  if(!dt_agent_capabilities_collect(dev, request->capabilities, error)
     || !dt_agent_image_state_collect_from_dev(dev, &request->image_state, error))
  {
    dt_agent_chat_request_clear(request);
    return FALSE;
  }

  request->image_revision_id = _agent_chat_format_image_revision_id(request->ui_context.has_image_id,
                                                                    request->ui_context.image_id,
                                                                    request->image_state.history_position,
                                                                    request->image_state.history_count);

  if(!request->request_id || !request->app_session_id || !request->image_session_id
     || !request->conversation_id || !request->turn_id || !request->image_revision_id
     || !request->message_text || !request->message_text[0] || !request->refinement_goal_text)
  {
    g_set_error(error, g_quark_from_static_string("dt-agent-chat-ui"), 1,
                "%s", _("failed to build an agent request"));
    dt_agent_chat_request_clear(request);
    return FALSE;
  }

  _agent_chat_test_cache_request_context(request);
  return TRUE;
}

static gboolean _agent_chat_apply_operation_range(const GPtrArray *operations,
                                                  const guint start_index,
                                                  dt_agent_execution_report_t *execution_report,
                                                  GError **error)
{
  if(!operations || start_index >= operations->len)
    return TRUE;

  dt_agent_chat_response_t partial_response;
  dt_agent_chat_response_init(&partial_response);
  if(partial_response.operations)
    g_ptr_array_unref(partial_response.operations);
  partial_response.operations = g_ptr_array_new();

  for(guint i = start_index; i < operations->len; i++)
    g_ptr_array_add(partial_response.operations, g_ptr_array_index((GPtrArray *)operations, i));

  const gboolean ok = dt_agent_execute_response(&partial_response, execution_report, error);

  g_ptr_array_unref(partial_response.operations);
  partial_response.operations = NULL;
  dt_agent_chat_response_clear(&partial_response);
  return ok;
}

static gboolean _agent_chat_test_env_truthy(const char *name)
{
  const char *value = g_getenv(name);
  return value && value[0] != '\0' && g_strcmp0(value, "0") != 0
      && g_ascii_strcasecmp(value, "false") != 0
      && g_ascii_strcasecmp(value, "no") != 0;
}

static guint _agent_chat_test_env_uint(const char *name, const guint fallback)
{
  const char *value = g_getenv(name);
  if(!value || value[0] == '\0')
    return fallback;

  char *end = NULL;
  const unsigned long parsed = g_ascii_strtoull(value, &end, 10);
  if(end == value || parsed > G_MAXUINT)
    return fallback;

  return (guint)parsed;
}

static double _agent_chat_exposure_from_state(const dt_agent_image_state_t *state)
{
  if(!state || !state->controls)
    return NAN;

  for(guint i = 0; i < state->controls->len; i++)
  {
    const dt_agent_image_control_t *control = g_ptr_array_index(state->controls, i);
    if(control && control->has_current_number
       && g_strcmp0(control->action_path, "iop/exposure/exposure") == 0)
      return control->current_number;
  }

  return NAN;
}

static double _agent_chat_current_exposure(dt_develop_t *dev)
{
  if(!dev)
    return NAN;

  dt_agent_image_state_t state;
  dt_agent_image_state_init(&state);
  g_autoptr(GError) error = NULL;
  const gboolean collected = dt_agent_image_state_collect_from_dev(dev, &state, &error);
  const double exposure = collected ? _agent_chat_exposure_from_state(&state) : NAN;
  dt_agent_image_state_clear(&state);
  return exposure;
}

static double _agent_chat_test_exposure_after_response(const double fallback,
                                                       const dt_agent_chat_response_t *response)
{
  double exposure = fallback;
  if(!response || !response->operations)
    return exposure;

  for(guint i = 0; i < response->operations->len; i++)
  {
    const dt_agent_chat_operation_t *operation = g_ptr_array_index(response->operations, i);
    if(!operation || operation->kind != DT_AGENT_OPERATION_SET_FLOAT
       || g_strcmp0(operation->action_path, "iop/exposure/exposure") != 0)
      continue;

    if(operation->value_mode == DT_AGENT_VALUE_MODE_SET)
      exposure = operation->number;
    else if(operation->value_mode == DT_AGENT_VALUE_MODE_DELTA && !isnan(exposure))
      exposure += operation->number;
  }

  return exposure;
}

static void _agent_chat_test_cache_request_context(const dt_agent_chat_request_t *request)
{
  if(!g_getenv("ANSEL_AGENT_TEST_RESULT_FILE") || !request)
    return;

  if(_agent_chat_test_cached_request_valid)
    dt_agent_chat_request_clear(&_agent_chat_test_cached_request);

  dt_agent_chat_request_copy(&_agent_chat_test_cached_request, request);
  _agent_chat_test_cached_request_valid = TRUE;
}

static gboolean _agent_chat_test_copy_cached_request(dt_develop_t *dev,
                                                     const char *message_text,
                                                     dt_agent_chat_request_t *request)
{
  if(!g_getenv("ANSEL_AGENT_TEST_RESULT_FILE")
     || !_agent_chat_test_cached_request_valid || !dev || !message_text || !request)
    return FALSE;

  dt_agent_chat_request_copy(request, &_agent_chat_test_cached_request);

  g_free(request->request_id);
  g_free(request->turn_id);
  g_free(request->message_text);
  g_free(request->refinement_goal_text);
  g_free(request->app_session_id);
  g_free(request->image_session_id);
  g_free(request->conversation_id);

  request->request_id = g_uuid_string_random();
  request->turn_id = g_strdup(request->request_id);
  request->message_text = g_strdup(message_text);
  request->refinement_goal_text = g_strdup(message_text);
  request->app_session_id = g_strdup(dev->agent_chat.app_session_id);
  request->image_session_id = g_strdup(dev->agent_chat.image_session_id);
  request->conversation_id = g_strdup(dev->agent_chat.conversation_id);

  return request->request_id && request->turn_id && request->message_text
      && request->refinement_goal_text && request->app_session_id
      && request->image_session_id && request->conversation_id;
}

static gboolean _agent_chat_test_quit_idle(gpointer user_data)
{
  (void)user_data;
  dt_control_quit();
  return G_SOURCE_REMOVE;
}

static void _agent_chat_test_schedule_quit(void)
{
  if(!g_getenv("ANSEL_AGENT_TEST_RESULT_FILE"))
    return;

  const guint delay_ms = _agent_chat_test_env_uint("ANSEL_AGENT_TEST_AUTORUN_QUIT_AFTER_MS", 1000u);
  g_timeout_add(MAX(1u, delay_ms), _agent_chat_test_quit_idle, NULL);
}

static void _agent_chat_test_write_report(dt_develop_t *dev,
                                          const char *status,
                                          const char *error_message,
                                          const dt_agent_chat_response_t *response,
                                          const guint operation_count,
                                          const guint execution_blocked_count,
                                          const guint execution_failed_count)
{
  const char *report_path = g_getenv("ANSEL_AGENT_TEST_RESULT_FILE");
  if(!report_path || report_path[0] == '\0')
    return;

  if(isnan(_agent_chat_test_exposure_before))
    _agent_chat_test_exposure_before = _agent_chat_current_exposure(dev);
  const double exposure_fallback = isnan(_agent_chat_test_exposure_after)
                                     ? _agent_chat_test_exposure_before
                                     : _agent_chat_test_exposure_after;
  const double current_exposure = isnan(_agent_chat_test_exposure_after)
                                    ? _agent_chat_test_exposure_after_response(exposure_fallback,
                                                                               response)
                                    : _agent_chat_test_exposure_after;
  _agent_chat_test_exposure_after = current_exposure;

  GKeyFile *key_file = g_key_file_new();
  g_key_file_set_string(key_file, "result", "status", status ? status : "error");
  if(error_message && error_message[0] != '\0')
    g_key_file_set_string(key_file, "result", "error", error_message);
  g_key_file_set_double(key_file, "result", "exposure_before", _agent_chat_test_exposure_before);
  g_key_file_set_double(key_file, "result", "current_exposure", current_exposure);
  g_key_file_set_integer(key_file, "result", "operation_count", (gint)operation_count);
  g_key_file_set_integer(key_file, "result", "execution_blocked_count", (gint)execution_blocked_count);
  g_key_file_set_integer(key_file, "result", "execution_failed_count", (gint)execution_failed_count);
  g_key_file_set_integer(key_file, "result", "active_image_id",
                         dev ? dev->image_storage.id : UNKNOWN_IMAGE);

  if(dev && dev->agent_chat.app_session_id)
    g_key_file_set_string(key_file, "result", "app_session_id", dev->agent_chat.app_session_id);
  if(dev && dev->agent_chat.image_session_id)
    g_key_file_set_string(key_file, "result", "image_session_id", dev->agent_chat.image_session_id);
  if(dev && dev->agent_chat.conversation_id)
    g_key_file_set_string(key_file, "result", "active_conversation_id", dev->agent_chat.conversation_id);

  const dt_agent_refinement_mode_t mode = response ? response->refinement_mode
                                                   : DT_AGENT_REFINEMENT_MODE_UNKNOWN;
  if(mode != DT_AGENT_REFINEMENT_MODE_UNKNOWN)
    g_key_file_set_string(key_file, "result", "refinement_mode",
                          dt_agent_refinement_mode_to_string(mode));
  if(response)
  {
    g_key_file_set_integer(key_file, "result", "refinement_enabled",
                           response->refinement_enabled ? 1 : 0);
    g_key_file_set_integer(key_file, "result", "refinement_max_turns",
                           (gint)response->refinement_max_passes);
    if(response->refinement_stop_reason && response->refinement_stop_reason[0] != '\0')
      g_key_file_set_string(key_file, "result", "refinement_stop_reason",
                            response->refinement_stop_reason);
  }
  g_key_file_set_integer(key_file, "result", "refinement_pass_count",
                         (gint)_agent_chat_test_refinement_pass_count);

  g_autoptr(GError) error = NULL;
  if(!g_key_file_save_to_file(key_file, report_path, &error) && error && error->message)
    dt_control_log(_("Failed to write agent smoke report: %s"), error->message);
  g_key_file_free(key_file);
}

static gboolean _agent_chat_test_autorun_idle(gpointer user_data)
{
  dt_develop_t *dev = (dt_develop_t *)user_data;
  const char *prompt = g_getenv("ANSEL_AGENT_TEST_AUTORUN_PROMPT");
  if(!prompt || prompt[0] == '\0')
    return G_SOURCE_REMOVE;

  if(!dev || dev->image_storage.id <= 0 || dev->agent_chat.is_loading)
    return G_SOURCE_CONTINUE;

  if(_agent_chat_test_autorun_started)
    return G_SOURCE_REMOVE;

  _agent_chat_test_autorun_started = TRUE;
  _agent_chat_test_refinement_pass_count = 0;
  _agent_chat_test_exposure_before = _agent_chat_current_exposure(dev);
  _agent_chat_test_exposure_after = _agent_chat_test_exposure_before;

  const gboolean multi_turn = _agent_chat_test_env_truthy("ANSEL_AGENT_TEST_MULTI_TURN_ENABLED");
  const guint max_turns = multi_turn
                            ? MAX(1u, _agent_chat_test_env_uint("ANSEL_AGENT_TEST_MULTI_TURN_MAX_TURNS",
                                                                 DT_AGENT_CHAT_DEFAULT_MAX_REFINEMENT_TURNS))
                            : 1u;
  if(!_agent_chat_submit_request(dev, prompt, 1u, max_turns, TRUE))
  {
    _agent_chat_test_write_report(dev, "error", "failed to submit autorun chat request",
                                  NULL, 0u, 0u, 1u);
    _agent_chat_test_schedule_quit();
  }

  return G_SOURCE_REMOVE;
}

static gchar *_agent_chat_format_tool_progress_message(const dt_agent_client_progress_t *progress,
                                                       const guint previous_tool_calls,
                                                       const guint previous_live_edit_count,
                                                       const guint live_edit_count)
{
  if(!progress || progress->tool_calls_used <= previous_tool_calls)
    return NULL;

  const guint max_tool_calls = progress->tool_calls_max > 0
                                 ? progress->tool_calls_max
                                 : MAX(1u, progress->tool_calls_used);
  const char *tool_name = progress->last_tool_name;
  if(!tool_name || !tool_name[0])
    tool_name = "";

  if(g_strcmp0(tool_name, "get_preview_image") == 0)
  {
    return g_strdup_printf(_("Tool %u/%u: checked the latest preview image."),
                           progress->tool_calls_used,
                           max_tool_calls);
  }

  if(g_strcmp0(tool_name, "get_image_state") == 0)
    return NULL;

  if(g_strcmp0(tool_name, "apply_operations") == 0)
  {
    const guint new_edits = live_edit_count > previous_live_edit_count
                              ? live_edit_count - previous_live_edit_count
                              : 0;
    if(new_edits > 0)
    {
      return g_strdup_printf(_("Tool %u/%u: applied %u edit%s (%u total)."),
                             progress->tool_calls_used,
                             max_tool_calls,
                             new_edits,
                             new_edits == 1 ? "" : "s",
                             live_edit_count);
    }

    return g_strdup_printf(_("Tool %u/%u: attempted live edits."),
                           progress->tool_calls_used,
                           max_tool_calls);
  }

  if(progress->message && progress->message[0] != '\0')
  {
    return g_strdup_printf(_("Tool %u/%u: %s"),
                           progress->tool_calls_used,
                           max_tool_calls,
                           progress->message);
  }

  return g_strdup_printf(_("Tool %u/%u: completed a tool call."),
                         progress->tool_calls_used,
                         max_tool_calls);
}

static gboolean _agent_chat_submit_request(dt_develop_t *dev,
                                           const char *message_text,
                                           const guint refinement_pass_index,
                                           const guint refinement_max_passes,
                                           const gboolean append_user_message)
{
  if(!dev || !message_text || !message_text[0] || dev->agent_chat.is_loading)
    return FALSE;

  _agent_chat_reset_for_current_image(dev, FALSE);
  if(append_user_message)
    _agent_chat_append_message(dev, _("you"), message_text);

  _agent_chat_set_error(dev, NULL);
  _agent_chat_set_loading(dev, TRUE);
  _agent_chat_set_status(dev,
                         refinement_pass_index > 1
                           ? _("Continuing multi-turn live edit...")
                           : _("Starting multi-turn live edit..."));

  dt_agent_chat_request_t request;
  g_autoptr(GError) error = NULL;
  const gboolean use_cached_test_context = refinement_pass_index > 1u
                                           && g_getenv("ANSEL_AGENT_TEST_RESULT_FILE");
  if(!_agent_chat_build_request(dev, message_text, &request, use_cached_test_context, &error))
  {
    _agent_chat_set_loading(dev, FALSE);
    _agent_chat_set_error(dev, error && error->message ? error->message
                                                       : _("failed to build an agent request"));
    _agent_chat_set_status(dev, _("Request failed"));
    return FALSE;
  }

  const gboolean multi_turn = refinement_max_passes > 1u;
  request.refinement_mode = multi_turn ? DT_AGENT_REFINEMENT_MODE_MULTI
                                       : DT_AGENT_REFINEMENT_MODE_SINGLE;
  request.refinement_enabled = multi_turn;
  request.refinement_pass_index = MAX(1u, refinement_pass_index);
  request.refinement_max_passes = multi_turn
                                    ? MAX(request.refinement_pass_index, refinement_max_passes)
                                    : 1u;

  dt_agent_chat_submission_t *submission = g_malloc0(sizeof(*submission));
  submission->request_id = g_strdup(request.request_id);
  submission->prompt_text = g_strdup(message_text);
  submission->conversation_id = g_strdup(request.conversation_id);
  submission->image_session_id = g_strdup(request.image_session_id);
  submission->image_id = dev->agent_chat.current_image_id;
  submission->refinement_pass_index = request.refinement_pass_index;
  submission->refinement_max_passes = request.refinement_max_passes;

  dt_agent_client_request_t *request_handle
    = dt_agent_client_chat_async(&request,
                                 _agent_chat_request_finished,
                                 _agent_chat_progress_finished,
                                 submission,
                                 _agent_chat_submission_free,
                                 &error);
  dt_agent_chat_request_clear(&request);

  if(!request_handle)
  {
    _agent_chat_submission_free(submission);
    _agent_chat_set_loading(dev, FALSE);
    _agent_chat_set_error(dev, error && error->message ? error->message
                                                       : _("failed to queue the agent request"));
    _agent_chat_set_status(dev, _("Request failed"));
    return FALSE;
  }

  _agent_chat_set_active_request(dev, request_handle, submission->request_id);
  if(g_getenv("ANSEL_AGENT_TEST_RESULT_FILE"))
    _agent_chat_test_refinement_pass_count++;
  if(dev->agent_chat.input_entry && append_user_message)
    gtk_entry_set_text(GTK_ENTRY(dev->agent_chat.input_entry), "");
  _agent_chat_update_sensitivity(dev);
  return TRUE;
}

static void _agent_chat_progress_finished(const dt_agent_client_progress_t *progress,
                                          gpointer user_data)
{
  dt_agent_chat_submission_t *submission = (dt_agent_chat_submission_t *)user_data;
  dt_develop_t *dev = _agent_chat_get_dev();
  if(!dev || !progress || !_agent_chat_submission_matches_active_request(dev, submission)
      || !_agent_chat_submission_matches_current_session(dev, submission) || progress->cancelled)
    return;

  if(progress->found)
  {
    const guint previous_tool_calls = dev->agent_chat.active_request_tool_calls_used;
    const guint previous_live_edit_count = dev->agent_chat.active_request_live_applied_count;
    dev->agent_chat.active_request_tool_calls_used = progress->tool_calls_used;
    if(progress->tool_calls_max > 0)
      dev->agent_chat.active_request_tool_calls_max = progress->tool_calls_max;

    const guint live_edit_count = progress->has_response && progress->response.operations
                                    ? progress->response.operations->len
                                    : progress->applied_operation_count;

    if(progress->has_response && progress->response.operations
       && progress->response.operations->len > dev->agent_chat.active_request_live_applied_count)
    {
      dt_agent_execution_report_t live_report;
      dt_agent_execution_report_init(&live_report);
      g_autoptr(GError) apply_error = NULL;
      if(!_agent_chat_apply_operation_range(progress->response.operations,
                                            dev->agent_chat.active_request_live_applied_count,
                                            &live_report,
                                            &apply_error))
      {
        const char *message = apply_error && apply_error->message ? apply_error->message
                                                                  : _("failed to apply live edits");
        _agent_chat_set_error(dev, message);
        _agent_chat_set_status(dev, _("Live apply failed"));
        _agent_chat_append_message(dev, _("system"), message);
        dt_agent_execution_report_clear(&live_report);
        _agent_chat_cancel_active_request(dev, _("Live apply failed"));
        return;
      }

      dev->agent_chat.active_request_live_applied_count = progress->response.operations->len;
      dt_dev_write_history(dev, FALSE);
      dt_dev_history_gui_update(dev);
      dt_dev_history_pixelpipe_update(dev, FALSE);
      dt_dev_history_notify_change(dev, dev->image_storage.id);
      dt_control_queue_redraw_center();
      if(progress->requires_render_callback)
        dev->agent_chat.pending_mid_turn_render = TRUE;
      dt_agent_execution_report_clear(&live_report);
    }

    g_autofree gchar *tool_progress_message = _agent_chat_format_tool_progress_message(progress,
                                                                                       previous_tool_calls,
                                                                                       previous_live_edit_count,
                                                                                       live_edit_count);
    if(tool_progress_message && tool_progress_message[0] != '\0')
      _agent_chat_append_message(dev, _("system"), tool_progress_message);

    if(progress->tool_calls_used > previous_tool_calls && progress->last_tool_name
       && progress->last_tool_name[0] != '\0')
    {
      const guint max_tool_calls = dev->agent_chat.active_request_tool_calls_max > 0
                                     ? dev->agent_chat.active_request_tool_calls_max
                                     : MAX(1u, progress->tool_calls_used);
      g_autofree gchar *tool_message = g_strdup_printf(
        _("Live edits | tool call %u/%u | %u edits applied | %s"),
        progress->tool_calls_used,
        max_tool_calls,
        dev->agent_chat.active_request_live_applied_count,
        progress->last_tool_name);
      _agent_chat_set_status(dev, tool_message);
    }
  }

  if(progress->status && progress->status[0] != '\0')
  {
    if(progress->found)
      _agent_chat_set_status(dev, progress->status);
  }
  else if(progress->message && progress->message[0] != '\0')
  {
    if(progress->found)
      _agent_chat_set_status(dev, progress->message);
  }
}

static void _agent_chat_request_finished(const dt_agent_client_result_t *result,
                                         gpointer user_data)
{
  dt_agent_chat_submission_t *submission = (dt_agent_chat_submission_t *)user_data;
  dt_develop_t *dev = _agent_chat_get_dev();
  if(!dev || !result || !submission)
    return;

  const gboolean active_request = _agent_chat_submission_matches_active_request(dev, submission);
  const gboolean current_session = _agent_chat_submission_matches_current_session(dev, submission);
  const guint already_live_applied_count = active_request
                                             ? dev->agent_chat.active_request_live_applied_count
                                             : 0u;
  if(active_request)
    _agent_chat_clear_active_request(dev);

  if(!current_session)
    return;

  _agent_chat_set_loading(dev, FALSE);

  if(result->cancelled)
  {
    _agent_chat_set_error(dev, NULL);
    _agent_chat_set_status(dev, _("Request canceled"));
    _agent_chat_test_write_report(dev, "error", "request canceled", NULL, 0u, 0u, 1u);
    _agent_chat_test_schedule_quit();
    return;
  }

  if(result->transport_error || !result->has_response)
  {
    const char *message = result->transport_error ? result->transport_error
                                                  : _("failed to contact the agent server");
    _agent_chat_set_error(dev, message);
    _agent_chat_set_status(dev, _("Request failed"));
    _agent_chat_append_message(dev, _("assistant"), message);
    _agent_chat_test_write_report(dev, "error", message, NULL, 0u, 0u, 1u);
    _agent_chat_test_schedule_quit();
    return;
  }

  if(result->response.image_session_id && result->response.image_session_id[0] != '\0')
  {
    g_free(dev->agent_chat.image_session_id);
    dev->agent_chat.image_session_id = g_strdup(result->response.image_session_id);
  }
  if(result->response.conversation_id && result->response.conversation_id[0] != '\0')
  {
    g_free(dev->agent_chat.conversation_id);
    dev->agent_chat.conversation_id = g_strdup(result->response.conversation_id);
  }

  _agent_chat_set_error(dev, NULL);

  if(result->response.message_text && result->response.message_text[0] != '\0')
    _agent_chat_append_message(dev, _("assistant"), result->response.message_text);

  if(g_strcmp0(result->response.status, "error") == 0)
  {
    const char *message = result->response.error_message ? result->response.error_message
                                                         : _("agent server returned an error");
    _agent_chat_set_error(dev, message);
    _agent_chat_set_status(dev, _("Server error"));
    if(!result->response.message_text || result->response.message_text[0] == '\0')
      _agent_chat_append_message(dev, _("assistant"), message);
    _agent_chat_test_write_report(dev, "error", message, &result->response,
                                  result->response.operations
                                    ? result->response.operations->len
                                    : 0u,
                                  0u, 1u);
    _agent_chat_test_schedule_quit();
    return;
  }

  guint execution_blocked_count = 0u;
  guint execution_failed_count = 0u;
  if(result->response.operations && result->response.operations->len > 0)
  {
    dt_agent_execution_report_t execution_report;
    dt_agent_execution_report_init(&execution_report);
    g_autoptr(GError) error = NULL;
    if(!_agent_chat_apply_operation_range(result->response.operations,
                                          already_live_applied_count,
                                          &execution_report,
                                          &error))
    {
      const char *message = error && error->message ? error->message
                                                    : _("failed to apply agent changes");
      _agent_chat_set_error(dev, message);
      _agent_chat_set_status(dev, _("Apply failed"));
      _agent_chat_append_message(dev, _("system"), message);
      _agent_chat_test_write_report(dev, "error", message, &result->response,
                                    result->response.operations->len,
                                    execution_report.blocked_count,
                                    execution_report.failed_count + 1u);
      _agent_chat_test_schedule_quit();
      dt_agent_execution_report_clear(&execution_report);
      return;
    }

    execution_blocked_count = execution_report.blocked_count;
    execution_failed_count = execution_report.failed_count;
    const double exposure_fallback = isnan(_agent_chat_test_exposure_after)
                                       ? _agent_chat_test_exposure_before
                                       : _agent_chat_test_exposure_after;
    _agent_chat_test_exposure_after = _agent_chat_test_exposure_after_response(exposure_fallback,
                                                                               &result->response);

    dev->agent_chat.active_request_live_applied_count = result->response.operations->len;
    dt_dev_write_history(dev, FALSE);
    dt_dev_history_gui_update(dev);
    dt_dev_history_pixelpipe_update(dev, FALSE);
    dt_dev_history_notify_change(dev, dev->image_storage.id);
    dt_control_queue_redraw_center();

    gchar *status = g_strdup_printf(execution_report.applied_count == 1
                                      ? _("Applied %u operation")
                                      : _("Applied %u operations"),
                                    execution_report.applied_count);
    _agent_chat_set_status(dev, status);
    g_free(status);
    dt_agent_execution_report_clear(&execution_report);
  }

  if(result->response.refinement_continue
     && result->response.refinement_pass_index < result->response.refinement_max_passes
     && submission->prompt_text && submission->prompt_text[0] != '\0')
  {
    g_autofree gchar *continue_status = g_strdup_printf(
      _("Continuing multi-turn live edit %u/%u"),
      result->response.refinement_pass_index + 1u,
      result->response.refinement_max_passes);
    _agent_chat_append_message(dev, _("system"), continue_status);
    if(_agent_chat_submit_request(dev,
                                  submission->prompt_text,
                                  result->response.refinement_pass_index + 1u,
                                  result->response.refinement_max_passes,
                                  FALSE))
      return;
  }

  _agent_chat_set_status(dev, _("Multi-turn live edit complete"));
  _agent_chat_test_write_report(dev, "ok", NULL, &result->response,
                                result->response.operations
                                  ? result->response.operations->len
                                  : 0u,
                                execution_blocked_count,
                                execution_failed_count);
  _agent_chat_test_schedule_quit();
}
#endif

static void _agent_chat_cancel_active_request(dt_develop_t *dev, const char *status_text)
{
  if(!dev)
    return;

#if HAVE_ANSEL_AGENT_CHAT_BACKEND
  if(dev->agent_chat.active_request)
  {
    dt_agent_client_request_cancel(dev->agent_chat.active_request, "cancelled");
    dt_agent_client_request_unref(dev->agent_chat.active_request);
    dev->agent_chat.active_request = NULL;
  }
#endif

  dev->agent_chat.active_request_live_applied_count = 0;
  dev->agent_chat.active_request_tool_calls_used = 0;
  dev->agent_chat.active_request_tool_calls_max = 0;
  dev->agent_chat.pending_mid_turn_render = FALSE;
  if(dev->agent_chat.is_loading)
    _agent_chat_set_loading(dev, FALSE);
  if(status_text)
    _agent_chat_set_status(dev, status_text);
  _agent_chat_set_error(dev, NULL);
  _agent_chat_update_sensitivity(dev);
}

static void _agent_chat_reset_for_current_image(dt_develop_t *dev, gboolean force_new_chat)
{
  if(!dev)
    return;

  const int32_t image_id = _agent_chat_current_image_id();
  if(!force_new_chat && dev->agent_chat.current_image_id == image_id
     && dev->agent_chat.image_session_id && dev->agent_chat.conversation_id)
  {
    _agent_chat_update_sensitivity(dev);
    return;
  }

  dev->agent_chat.current_image_id = image_id;
  g_free(dev->agent_chat.image_session_id);
  dev->agent_chat.image_session_id = g_uuid_string_random();
  g_free(dev->agent_chat.conversation_id);
  dev->agent_chat.conversation_id = g_uuid_string_random();
  _agent_chat_replace_transcript(dev, NULL);
  _agent_chat_set_error(dev, NULL);
  _agent_chat_set_loading(dev, FALSE);
  _agent_chat_set_status(dev, HAVE_ANSEL_AGENT_CHAT_BACKEND
                                ? _("ready for multi-turn live edits")
                                : _("agent backend not available in this build"));
  if(dev->agent_chat.input_entry)
    gtk_entry_set_text(GTK_ENTRY(dev->agent_chat.input_entry), "");
  _agent_chat_update_sensitivity(dev);
}

static void _agent_chat_window_init(GtkWidget *widget)
{
  const gint width = MAX(DT_PIXEL_APPLY_DPI(504),
                         dt_conf_get_int("plugins/darkroom/agent_chat/window_w"));
  const gint height = MAX(DT_PIXEL_APPLY_DPI(624),
                          dt_conf_get_int("plugins/darkroom/agent_chat/window_h"));
  const gint x = MAX(0, dt_conf_get_int("plugins/darkroom/agent_chat/window_x"));
  const gint y = MAX(0, dt_conf_get_int("plugins/darkroom/agent_chat/window_y"));

  gtk_window_set_default_size(GTK_WINDOW(widget), width, height);
  gtk_window_move(GTK_WINDOW(widget), x, y);
  gtk_window_resize(GTK_WINDOW(widget), width, height);
}

static void _agent_chat_window_write_config(GtkWidget *widget)
{
  if(!widget || !gtk_widget_get_window(widget))
    return;

  GtkAllocation allocation;
  gtk_widget_get_allocation(widget, &allocation);
  gint x = 0;
  gint y = 0;
  gtk_window_get_position(GTK_WINDOW(widget), &x, &y);
  dt_conf_set_int("plugins/darkroom/agent_chat/window_x", x);
  dt_conf_set_int("plugins/darkroom/agent_chat/window_y", y);
  dt_conf_set_int("plugins/darkroom/agent_chat/window_w", allocation.width);
  dt_conf_set_int("plugins/darkroom/agent_chat/window_h", allocation.height);
}

static gboolean _agent_chat_window_reflow_idle(gpointer user_data)
{
  GtkWidget *widget = GTK_WIDGET(user_data);
  if(!GTK_IS_WINDOW(widget) || !gtk_widget_get_visible(widget))
    return G_SOURCE_REMOVE;

  _agent_chat_window_init(widget);
  gtk_widget_queue_resize(widget);
  return G_SOURCE_REMOVE;
}

static gboolean _agent_chat_focus_input_idle(gpointer user_data)
{
  dt_develop_t *dev = (dt_develop_t *)user_data;
  if(!dev || !dev->agent_chat.floating_window || !dev->agent_chat.input_entry)
    return G_SOURCE_REMOVE;

  if(!gtk_widget_get_visible(dev->agent_chat.floating_window)
     || !gtk_widget_get_visible(dev->agent_chat.input_entry)
     || !gtk_widget_get_sensitive(dev->agent_chat.input_entry))
    return G_SOURCE_REMOVE;

  gtk_window_set_focus(GTK_WINDOW(dev->agent_chat.floating_window), dev->agent_chat.input_entry);
  gtk_widget_grab_focus(dev->agent_chat.input_entry);
  gtk_editable_set_position(GTK_EDITABLE(dev->agent_chat.input_entry), -1);
  return G_SOURCE_REMOVE;
}

static gboolean _agent_chat_window_delete_callback(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
  dt_develop_t *dev = (dt_develop_t *)user_data;
  (void)event;
  _agent_chat_window_write_config(widget);
  gtk_widget_hide(widget);
  if(dev && dev->agent_chat.button)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dev->agent_chat.button), FALSE);
  return TRUE;
}

static void _agent_chat_display_window(dt_develop_t *dev)
{
  if(!dev || !dev->agent_chat.floating_window)
    return;

  if(!gtk_widget_get_realized(dev->agent_chat.floating_window))
    _agent_chat_window_init(dev->agent_chat.floating_window);

  gtk_widget_show_all(dev->agent_chat.floating_window);
  g_idle_add(_agent_chat_window_reflow_idle, dev->agent_chat.floating_window);
  gtk_window_present(GTK_WINDOW(dev->agent_chat.floating_window));
  g_idle_add(_agent_chat_focus_input_idle, dev);
}

static void _agent_chat_toggle_callback(GtkToggleButton *button, gpointer user_data)
{
  dt_develop_t *dev = (dt_develop_t *)user_data;
  if(gtk_toggle_button_get_active(button))
  {
    _agent_chat_display_window(dev);
  }
  else if(dev && dev->agent_chat.floating_window)
  {
    _agent_chat_window_write_config(dev->agent_chat.floating_window);
    gtk_widget_hide(dev->agent_chat.floating_window);
  }
}

static void _agent_chat_send_clicked(GtkButton *button, gpointer user_data)
{
  dt_develop_t *dev = (dt_develop_t *)user_data;
  (void)button;

  if(!dev || !dev->agent_chat.input_entry)
    return;

  g_autofree gchar *message = g_strstrip(g_strdup(gtk_entry_get_text(GTK_ENTRY(dev->agent_chat.input_entry))));
  if(!message || !message[0])
    return;

#if !HAVE_ANSEL_AGENT_CHAT_BACKEND
  _agent_chat_set_error(dev, _("agent backend not available in this build"));
  _agent_chat_set_status(dev, _("Backend unavailable"));
  dt_control_log(_("The agent backend is not available in this build."));
  return;
#else
  _agent_chat_submit_request(dev, message, 1u, DT_AGENT_CHAT_DEFAULT_MAX_REFINEMENT_TURNS, TRUE);
#endif
}

static void _agent_chat_cancel_clicked(GtkButton *button, gpointer user_data)
{
  (void)button;
  _agent_chat_cancel_active_request((dt_develop_t *)user_data, _("Canceling request..."));
}

static void _agent_chat_new_chat_clicked(GtkButton *button, gpointer user_data)
{
  dt_develop_t *dev = (dt_develop_t *)user_data;
  (void)button;
  if(!dev)
    return;

  _agent_chat_cancel_active_request(dev, _("Starting a new chat"));
  _agent_chat_reset_for_current_image(dev, TRUE);
}

static void _agent_chat_entry_activate(GtkEntry *entry, gpointer user_data)
{
  (void)entry;
  _agent_chat_send_clicked(NULL, user_data);
}

static void _agent_chat_image_changed_callback(gpointer instance, gpointer user_data)
{
  dt_view_t *self = (dt_view_t *)user_data;
  (void)instance;
  if(!self || !self->data)
    return;

  dt_develop_t *dev = (dt_develop_t *)self->data;
  const int32_t image_id = _agent_chat_current_image_id();
  if(image_id == dev->agent_chat.current_image_id)
    return;

  _agent_chat_cancel_active_request(dev, _("Request canceled"));
  _agent_chat_reset_for_current_image(dev, TRUE);
}

/**
 * DOC
 * Toolbox accelerators forward keyboard activation to the existing Gtk buttons
 * so the keyboard path reuses the exact same callbacks, state changes and
 * popover anchoring as the pointer path.
 */
static gboolean _darkroom_toolbox_button_activate_accel(GtkAccelGroup *accel_group, GObject *accelerable,
                                                        guint keyval, GdkModifierType modifier,
                                                        gpointer data)
{
  GtkWidget *button = GTK_WIDGET(data);
  if(IS_NULL_PTR(button) || !gtk_widget_is_visible(button) || !gtk_widget_is_sensitive(button)) return FALSE;

  gtk_button_clicked(GTK_BUTTON(button));
  return TRUE;
}

static gboolean _darkroom_toolbox_button_focus_accel(GtkAccelGroup *accel_group, GObject *accelerable,
                                                     guint keyval, GdkModifierType modifier,
                                                     gpointer data)
{
  GtkWidget *button = GTK_WIDGET(data);
  if(IS_NULL_PTR(button) || !gtk_widget_is_visible(button) || !gtk_widget_is_sensitive(button)) return FALSE;

  GtkWidget *popover = g_object_get_data(G_OBJECT(button), "dt-darkroom-toolbox-popover");
  if(IS_NULL_PTR(popover) || !gtk_widget_is_sensitive(popover)) return FALSE;

  gtk_widget_grab_focus(button);
  gtk_popover_set_relative_to(GTK_POPOVER(popover), button);
  _toolbar_show_popup(popover);
  return TRUE;
}

static void _get_final_size_with_iso_12646(dt_develop_t *d)
{
  if(d->iso_12646.enabled)
  {
    // For ISO 12646, we want portraits and landscapes to cover roughly the same surface
    // no matter the size of the widget. Meaning we force them to fit a square
    // of length matching the smaller widget dimension. The goal is to leave
    // a consistent perceptual impression between pictures, independent from orientation.
    const int main_dim = MIN(d->roi.orig_width, d->roi.orig_height);
    d->roi.border_size = 0.125 * main_dim;
  }
  else
  {
    d->roi.border_size = DT_PIXEL_APPLY_DPI(dt_conf_get_int("plugins/darkroom/ui/border_size"));
  }

  dt_dev_configure(d, d->roi.orig_width - 2 * d->roi.border_size, d->roi.orig_height - 2 * d->roi.border_size);
}

/* colour assessment */
static void _iso_12646_quickbutton_clicked(GtkWidget *w, gpointer user_data)
{
  dt_develop_t *d = (dt_develop_t *)user_data;
  if (!d->gui_attached) return;

  d->iso_12646.enabled = !d->iso_12646.enabled;
  _get_final_size_with_iso_12646(d);

  // This is already called in _get_final_size_... but it's not enough
  dt_control_queue_redraw_center();
}

/* overlay color */
static void _guides_quickbutton_clicked(GtkWidget *widget, gpointer user_data)
{
  dt_guides_button_toggled(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
  dt_control_queue_redraw_center();
}

static void _guides_view_changed(gpointer instance, dt_view_t *old_view, dt_view_t *new_view, dt_lib_module_t *self)
{
  dt_guides_update_button_state();
}

/**
 * DOC
 * Overexposed and gamut modules are inserted inplace in pipeline at runtime,
 * only for the main preview, and don't add history items.
 * They all need a full history -> pipeline resynchronization.
 */

/* display */
static void _display_quickbutton_clicked(GtkWidget *w, gpointer user_data)
{
}

static void display_brightness_callback(GtkWidget *slider, gpointer user_data)
{
  dt_conf_set_int("display/brightness", (int)(dt_bauhaus_slider_get(slider)));
  dt_control_queue_redraw_center();
}

static void display_borders_callback(GtkWidget *slider, gpointer user_data)
{
  dt_develop_t *d = (dt_develop_t *)user_data;
  dt_conf_set_int("plugins/darkroom/ui/border_size", (int)dt_bauhaus_slider_get(slider));
  _get_final_size_with_iso_12646(d);
  dt_dev_pixelpipe_change_zoom_main(d);
}

/* overexposed */
static void _overexposed_quickbutton_clicked(GtkWidget *w, gpointer user_data)
{
  dt_develop_t *d = (dt_develop_t *)user_data;
  d->overexposed.enabled = !d->overexposed.enabled;
  dt_dev_pixelpipe_resync_history_main(d);
}

static void colorscheme_callback(GtkWidget *combo, gpointer user_data)
{
  dt_develop_t *d = (dt_develop_t *)user_data;
  d->overexposed.colorscheme = dt_bauhaus_combobox_get(combo);
  if(d->overexposed.enabled == FALSE)
    gtk_button_clicked(GTK_BUTTON(d->overexposed.button));

  dt_dev_pixelpipe_resync_history_main(d);
}

static void lower_callback(GtkWidget *slider, gpointer user_data)
{
  dt_develop_t *d = (dt_develop_t *)user_data;
  d->overexposed.lower = dt_bauhaus_slider_get(slider);
  if(d->overexposed.enabled == FALSE)
    gtk_button_clicked(GTK_BUTTON(d->overexposed.button));

  dt_dev_pixelpipe_resync_history_main(d);
}

static void upper_callback(GtkWidget *slider, gpointer user_data)
{
  dt_develop_t *d = (dt_develop_t *)user_data;
  d->overexposed.upper = dt_bauhaus_slider_get(slider);
  if(d->overexposed.enabled == FALSE)
    gtk_button_clicked(GTK_BUTTON(d->overexposed.button));

  dt_dev_pixelpipe_resync_history_main(d);
}

static void mode_callback(GtkWidget *slider, gpointer user_data)
{
  dt_develop_t *d = (dt_develop_t *)user_data;
  d->overexposed.mode = dt_bauhaus_combobox_get(slider);
  if(d->overexposed.enabled == FALSE)
    gtk_button_clicked(GTK_BUTTON(d->overexposed.button));

  dt_dev_pixelpipe_update_history_main(d);
}

/* rawoverexposed */
static void _rawoverexposed_quickbutton_clicked(GtkWidget *w, gpointer user_data)
{
  dt_develop_t *d = (dt_develop_t *)user_data;
  d->rawoverexposed.enabled = !d->rawoverexposed.enabled;
  dt_dev_pixelpipe_resync_history_main(d);
}

static void rawoverexposed_mode_callback(GtkWidget *combo, gpointer user_data)
{
  dt_develop_t *d = (dt_develop_t *)user_data;
  d->rawoverexposed.mode = dt_bauhaus_combobox_get(combo);
  if(d->rawoverexposed.enabled == FALSE)
    gtk_button_clicked(GTK_BUTTON(d->rawoverexposed.button));

  dt_dev_pixelpipe_resync_history_main(d);
}

static void rawoverexposed_colorscheme_callback(GtkWidget *combo, gpointer user_data)
{
  dt_develop_t *d = (dt_develop_t *)user_data;
  d->rawoverexposed.colorscheme = dt_bauhaus_combobox_get(combo);
  if(d->rawoverexposed.enabled == FALSE)
    gtk_button_clicked(GTK_BUTTON(d->rawoverexposed.button));

  dt_dev_pixelpipe_resync_history_main(d);
}

static void rawoverexposed_threshold_callback(GtkWidget *slider, gpointer user_data)
{
  dt_develop_t *d = (dt_develop_t *)user_data;
  d->rawoverexposed.threshold = dt_bauhaus_slider_get(slider);
  if(d->rawoverexposed.enabled == FALSE)
    gtk_button_clicked(GTK_BUTTON(d->rawoverexposed.button));

  dt_dev_pixelpipe_resync_history_main(d);
}

/* softproof */
static void _softproof_quickbutton_clicked(GtkWidget *w, gpointer user_data)
{
  dt_develop_t *d = (dt_develop_t *)user_data;
  if(darktable.color_profiles->mode == DT_PROFILE_SOFTPROOF)
    darktable.color_profiles->mode = DT_PROFILE_NORMAL;
  else
    darktable.color_profiles->mode = DT_PROFILE_SOFTPROOF;

  _update_softproof_gamut_checking(d);
  dt_dev_pixelpipe_resync_history_main(d);
}

/* gamut */
static void _gamut_quickbutton_clicked(GtkWidget *w, gpointer user_data)
{
  dt_develop_t *d = (dt_develop_t *)user_data;
  if(darktable.color_profiles->mode == DT_PROFILE_GAMUTCHECK)
    darktable.color_profiles->mode = DT_PROFILE_NORMAL;
  else
    darktable.color_profiles->mode = DT_PROFILE_GAMUTCHECK;

  _update_softproof_gamut_checking(d);

  dt_dev_pixelpipe_resync_history_main(d);
}

/* set the gui state for both softproof and gamut checking */
static void _update_softproof_gamut_checking(dt_develop_t *d)
{
  g_signal_handlers_block_by_func(d->profile.softproof_button, _softproof_quickbutton_clicked, d);
  g_signal_handlers_block_by_func(d->profile.gamut_button, _gamut_quickbutton_clicked, d);

  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->profile.softproof_button), darktable.color_profiles->mode == DT_PROFILE_SOFTPROOF);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->profile.gamut_button), darktable.color_profiles->mode == DT_PROFILE_GAMUTCHECK);

  g_signal_handlers_unblock_by_func(d->profile.softproof_button, _softproof_quickbutton_clicked, d);
  g_signal_handlers_unblock_by_func(d->profile.gamut_button, _gamut_quickbutton_clicked, d);
}


static void softproof_profile_callback(GtkWidget *combo, gpointer user_data)
{
  dt_develop_t *d = (dt_develop_t *)user_data;
  gboolean profile_changed = FALSE;
  const int pos = dt_bauhaus_combobox_get(combo);
  for(GList *profiles = darktable.color_profiles->profiles; profiles; profiles = g_list_next(profiles))
  {
    dt_colorspaces_color_profile_t *pp = (dt_colorspaces_color_profile_t *)profiles->data;
    if(pp->out_pos == pos)
    {
      if(darktable.color_profiles->softproof_type != pp->type
        || (darktable.color_profiles->softproof_type == DT_COLORSPACE_FILE
            && strcmp(darktable.color_profiles->softproof_filename, pp->filename)))

      {
        darktable.color_profiles->softproof_type = pp->type;
        g_strlcpy(darktable.color_profiles->softproof_filename, pp->filename,
                  sizeof(darktable.color_profiles->softproof_filename));
        profile_changed = TRUE;
      }
      goto end;
    }
  }

  // profile not found, fall back to sRGB. shouldn't happen
  fprintf(stderr, "can't find softproof profile `%s', using sRGB instead\n", dt_bauhaus_combobox_get_text(combo));
  profile_changed = darktable.color_profiles->softproof_type != DT_COLORSPACE_SRGB;
  darktable.color_profiles->softproof_type = DT_COLORSPACE_SRGB;
  darktable.color_profiles->softproof_filename[0] = '\0';

end:
  if(profile_changed)
  {
    DT_DEBUG_CONTROL_SIGNAL_RAISE(darktable.signals, DT_SIGNAL_CONTROL_PROFILE_USER_CHANGED, DT_COLORSPACES_PROFILE_TYPE_SOFTPROOF);
    dt_dev_pixelpipe_resync_history_main(d);
  }
}

/** end of toolbox */

#if 0

static void _overlay_cycle_callback(dt_action_t *action)
{
  const int currentval = dt_conf_get_int("darkroom/ui/overlay_color");
  const int nextval = (currentval + 1) % 6; // colors can go from 0 to 5
  dt_conf_set_int("darkroom/ui/overlay_color", nextval);
  dt_guides_set_overlay_colors();
  dt_control_queue_redraw_center();
}

static void _toggle_mask_visibility_callback(dt_action_t *action)
{
  if(darktable.gui->reset) return;

  dt_develop_t *dev = dt_action_view(action)->data;
  dt_iop_module_t *mod = dev->gui_module;

  //retouch and spot removal module use masks differently and have different buttons associated
  //keep the shortcuts independent
  if(mod && strcmp(mod->so->op, "spots") != 0 && strcmp(mod->so->op, "retouch") != 0)
  {
    dt_iop_gui_blend_data_t *bd = (dt_iop_gui_blend_data_t *)mod->blend_data;

    ++darktable.gui->reset;

    dt_iop_color_picker_reset(mod, TRUE);

    dt_masks_form_t *grp = dt_masks_get_from_id(darktable.develop, mod->blend_params->mask_id);
    if(grp && (grp->type & DT_MASKS_GROUP) && grp->points)
    {
      if(bd->masks_shown == DT_MASKS_EDIT_OFF)
        bd->masks_shown = DT_MASKS_EDIT_FULL;
      else
        bd->masks_shown = DT_MASKS_EDIT_OFF;

      gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bd->masks_edit), bd->masks_shown != DT_MASKS_EDIT_OFF);
      dt_masks_set_edit_mode(mod, bd->masks_shown);

      // set all add shape buttons to inactive
      for(int n = 0; n < DEVELOP_MASKS_NB_SHAPES; n++)
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(bd->masks_shapes[n]), FALSE);
    }

    --darktable.gui->reset;
  }
}


#endif

static gboolean _quickbutton_press_release(GtkWidget *button, GdkEventButton *event, GtkWidget *popover)
{
  static guint start_time = 0;

  int delay = 0;
  g_object_get(gtk_settings_get_default(), "gtk-long-press-time", &delay, NULL);

  if((event->type == GDK_BUTTON_PRESS && event->button == 3) ||
     (event->type == GDK_BUTTON_RELEASE && event->time - start_time > delay))
  {
    gtk_popover_set_relative_to(GTK_POPOVER(popover), button);
    g_object_set(G_OBJECT(popover), "transitions-enabled", FALSE, NULL);

    _toolbar_show_popup(popover);
    return TRUE;
  }
  else
  {
    start_time = event->time;
    return FALSE;
  }
}

void connect_button_press_release(GtkWidget *w, GtkWidget *p)
{
  g_signal_connect(w, "button-press-event", G_CALLBACK(_quickbutton_press_release), p);
  g_signal_connect(w, "button-release-event", G_CALLBACK(_quickbutton_press_release), p);
}

gboolean _focus_main_image(GtkAccelGroup *accel_group, GObject *accelerable, guint keyval,
                           GdkModifierType modifier, gpointer data)
{
  gtk_widget_grab_focus(dt_ui_center(darktable.gui->ui));
  return TRUE;
}

gboolean _switch_to_next_picture(GtkAccelGroup *accel_group, GObject *accelerable, guint keyval,
                                 GdkModifierType modifier, gpointer data)
{
  dt_view_t *view = (dt_view_t *)data;
  dt_develop_t *dev = (dt_develop_t *)view->data;
  int32_t current_img = dev->image_storage.id;
  GList *current_collection = dt_collection_get_all(darktable.collection, -1);
  GList *current_item = g_list_find(current_collection, GINT_TO_POINTER(current_img));

  if(current_item && current_item->next)
  {
    int32_t next_img = GPOINTER_TO_INT(current_item->next->data);
    g_list_free(current_collection);
    current_collection = NULL;
    _dev_change_image(data, next_img);
  }
  else
  {
    g_list_free(current_collection);
    current_collection = NULL;
  }

  return TRUE;
}

gboolean _switch_to_prev_picture(GtkAccelGroup *accel_group, GObject *accelerable, guint keyval,
                                 GdkModifierType modifier, gpointer data)
{
  dt_view_t *view = (dt_view_t *)data;
  dt_develop_t *dev = (dt_develop_t *)view->data;
  int32_t current_img = dev->image_storage.id;
  GList *current_collection = dt_collection_get_all(darktable.collection, -1);
  GList *current_item = g_list_find(current_collection, GINT_TO_POINTER(current_img));

  if(current_item && current_item->prev)
  {
    int32_t prev_img = GPOINTER_TO_INT(current_item->prev->data);
    g_list_free(current_collection);
    current_collection = NULL;
    _dev_change_image(data, prev_img);
  }
  else
  {
    g_list_free(current_collection);
    current_collection = NULL;
  }

  return TRUE;
}


void gui_init(dt_view_t *self)
{
  dt_develop_t *dev = (dt_develop_t *)self->data;

  dt_accels_new_darkroom_action(_switch_to_next_picture, self, N_("Darkroom/Actions"),
                                N_("Switch to the next picture"), GDK_KEY_Right, GDK_MOD1_MASK, _("Triggers the action"));
  dt_accels_new_darkroom_action(_switch_to_prev_picture, self, N_("Darkroom/Actions"),
                                N_("Switch to the previous picture"), GDK_KEY_Left, GDK_MOD1_MASK, _("Triggers the action"));

  gchar *path = dt_accels_build_path(_("Darkroom/Actions"), _("Give focus to the main image"));
  dt_accels_new_virtual_shortcut(darktable.gui->accels, darktable.gui->accels->darkroom_accels,
                                 path, dt_ui_center(darktable.gui->ui), GDK_KEY_Return, 0);
  dt_free(path);

  path = dt_accels_build_path(_("Darkroom/Main image"), _("Move up"));
  dt_accels_new_virtual_shortcut(darktable.gui->accels, darktable.gui->accels->darkroom_accels,
                                 path, dt_ui_center(darktable.gui->ui), GDK_KEY_Up, 0);
  dt_free(path);

  path = dt_accels_build_path(_("Darkroom/Main image"), _("Move up (coarse step)"));
  dt_accels_new_virtual_shortcut(darktable.gui->accels, darktable.gui->accels->darkroom_accels,
                                 path, dt_ui_center(darktable.gui->ui), GDK_KEY_Up, GDK_SHIFT_MASK);
  dt_free(path);

  path = dt_accels_build_path(_("Darkroom/Main image"), _("Move up (fine step)"));
  dt_accels_new_virtual_shortcut(darktable.gui->accels, darktable.gui->accels->darkroom_accels,
                                 path, dt_ui_center(darktable.gui->ui), GDK_KEY_Up, GDK_CONTROL_MASK);
  dt_free(path);

  path = dt_accels_build_path(_("Darkroom/Main image"), _("Move down"));
  dt_accels_new_virtual_shortcut(darktable.gui->accels, darktable.gui->accels->darkroom_accels,
                                 path, dt_ui_center(darktable.gui->ui), GDK_KEY_Down, 0);
  dt_free(path);

  path = dt_accels_build_path(_("Darkroom/Main image"), _("Move down (coarse step)"));
  dt_accels_new_virtual_shortcut(darktable.gui->accels, darktable.gui->accels->darkroom_accels,
                                 path, dt_ui_center(darktable.gui->ui), GDK_KEY_Down, GDK_SHIFT_MASK);
  dt_free(path);

  path = dt_accels_build_path(_("Darkroom/Main image"), _("Move down (fine step)"));
  dt_accels_new_virtual_shortcut(darktable.gui->accels, darktable.gui->accels->darkroom_accels,
                                 path, dt_ui_center(darktable.gui->ui), GDK_KEY_Down, GDK_CONTROL_MASK);
  dt_free(path);

  path = dt_accels_build_path(_("Darkroom/Main image"), _("Move left"));
  dt_accels_new_virtual_shortcut(darktable.gui->accels, darktable.gui->accels->darkroom_accels,
                                 path, dt_ui_center(darktable.gui->ui), GDK_KEY_Left, 0);
  dt_free(path);

  path = dt_accels_build_path(_("Darkroom/Main image"), _("Move left (coarse step)"));
  dt_accels_new_virtual_shortcut(darktable.gui->accels, darktable.gui->accels->darkroom_accels,
                                 path, dt_ui_center(darktable.gui->ui), GDK_KEY_Left, GDK_SHIFT_MASK);
  dt_free(path);

  path = dt_accels_build_path(_("Darkroom/Main image"), _("Move left (fine step)"));
  dt_accels_new_virtual_shortcut(darktable.gui->accels, darktable.gui->accels->darkroom_accels,
                                 path, dt_ui_center(darktable.gui->ui), GDK_KEY_Left, GDK_CONTROL_MASK);
  dt_free(path);

  path = dt_accels_build_path(_("Darkroom/Main image"), _("Move right"));
  dt_accels_new_virtual_shortcut(darktable.gui->accels, darktable.gui->accels->darkroom_accels,
                                 path, dt_ui_center(darktable.gui->ui), GDK_KEY_Right, 0);
  dt_free(path);

  path = dt_accels_build_path(_("Darkroom/Main image"), _("Move right (coarse step)"));
  dt_accels_new_virtual_shortcut(darktable.gui->accels, darktable.gui->accels->darkroom_accels,
                                 path, dt_ui_center(darktable.gui->ui), GDK_KEY_Right, GDK_SHIFT_MASK);
  dt_free(path);

  path = dt_accels_build_path(_("Darkroom/Main image"), _("Move right (fine step)"));
  dt_accels_new_virtual_shortcut(darktable.gui->accels, darktable.gui->accels->darkroom_accels,
                                 path, dt_ui_center(darktable.gui->ui), GDK_KEY_Right, GDK_CONTROL_MASK);
  dt_free(path);

  path = dt_accels_build_path(_("Darkroom/Main image"), _("Zoom in"));
  dt_accels_new_virtual_shortcut(darktable.gui->accels, darktable.gui->accels->darkroom_accels,
                                 path, dt_ui_center(darktable.gui->ui), GDK_KEY_plus, GDK_CONTROL_MASK);
  dt_free(path);

  path = dt_accels_build_path(_("Darkroom/Main image"), _("Zoom out"));
  dt_accels_new_virtual_shortcut(darktable.gui->accels, darktable.gui->accels->darkroom_accels,
                                 path, dt_ui_center(darktable.gui->ui), GDK_KEY_minus, GDK_CONTROL_MASK);
  dt_free(path);
  /*
   * Add view specific tool buttons
   */

  /* Enable ISO 12646-compliant colour assessment conditions */
  dev->iso_12646.button = dtgtk_togglebutton_new(dtgtk_cairo_paint_bulb, 0, NULL);
  gtk_widget_set_tooltip_text(dev->iso_12646.button,
                              _("toggle ISO 12646 color assessment conditions"));
  g_signal_connect(G_OBJECT(dev->iso_12646.button), "clicked", G_CALLBACK(_iso_12646_quickbutton_clicked), dev);
  dt_view_manager_module_toolbox_add(darktable.view_manager, dev->iso_12646.button, DT_VIEW_DARKROOM);
  dt_accels_new_darkroom_action(_darkroom_toolbox_button_activate_accel, dev->iso_12646.button,
                                N_("Darkroom/Toolbox"),
                                N_("Toggle ISO 12646 color assessment conditions"), 0, 0,
                                _("Triggers the action"));

  /* display background options */
  {
    dev->display.button = dtgtk_button_new(dtgtk_cairo_paint_display, 0, NULL);
    dt_view_manager_module_toolbox_add(darktable.view_manager, dev->display.button, DT_VIEW_DARKROOM);
    gtk_widget_set_tooltip_text(dev->display.button, _("Picture display options"));
    g_signal_connect(G_OBJECT(dev->display.button), "clicked",
                     G_CALLBACK(_display_quickbutton_clicked), dev);

    // and the popup window
    dev->display.floating_window = gtk_popover_new(dev->display.button);
    connect_button_press_release(dev->display.button, dev->display.floating_window);
    g_object_set_data(G_OBJECT(dev->display.button), "dt-darkroom-toolbox-popover",
                      dev->display.floating_window);
    dt_accels_new_darkroom_action(_darkroom_toolbox_button_focus_accel, dev->display.button,
                                  N_("Darkroom/Toolbox"),
                                  N_("Focus picture display options"), 0, 0,
                                  _("Shows the options popover"));

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(dev->display.floating_window), vbox);

    /** let's fill the encapsulating widgets */
    GtkWidget *brightness = dt_bauhaus_slider_new_with_range(darktable.bauhaus, DT_GUI_MODULE(NULL), 0, 100, 5, 50, 0);
    dt_bauhaus_slider_set(brightness, (int)dt_conf_get_int("display/brightness"));
    dt_bauhaus_widget_set_label(brightness, N_("Background brightness"));
    dt_bauhaus_slider_set_format(brightness, "%");
    g_signal_connect(G_OBJECT(brightness), "value-changed", G_CALLBACK(display_brightness_callback), dev);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(brightness), TRUE, TRUE, 0);

    GtkWidget *borders = dt_bauhaus_slider_new_with_range(darktable.bauhaus, DT_GUI_MODULE(NULL), 0, 250, 5, 10, 0);
    dt_bauhaus_slider_set(borders, dt_conf_get_int("plugins/darkroom/ui/border_size"));
    dt_bauhaus_widget_set_label(borders, N_("Picture margins"));
    dt_bauhaus_slider_set_format(borders, "px");
    g_signal_connect(G_OBJECT(borders), "value-changed", G_CALLBACK(display_borders_callback), dev);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(borders), TRUE, TRUE, 0);
  }

  _darkroom_ioporder_button = dtgtk_button_new(dt_draw_icon_flowchart, 0, NULL);
  gtk_widget_set_tooltip_text(_darkroom_ioporder_button, _("show the pipeline node graph"));
  g_signal_connect(G_OBJECT(_darkroom_ioporder_button), "clicked",
                   G_CALLBACK(_darkroom_ioporder_quickbutton_clicked), dev);
  dt_view_manager_module_toolbox_add(darktable.view_manager, _darkroom_ioporder_button, DT_VIEW_DARKROOM);
  dt_accels_new_darkroom_action(_darkroom_toolbox_button_activate_accel, _darkroom_ioporder_button,
                                N_("Darkroom/Toolbox"),
                                N_("Show the pipeline node graph"), 0, 0,
                                _("Triggers the action"));

  {
    dev->agent_chat.button = gtk_toggle_button_new_with_label(_("chat"));
    gtk_widget_set_tooltip_text(dev->agent_chat.button, _("toggle assistant chat popup"));
    g_signal_connect(G_OBJECT(dev->agent_chat.button), "toggled",
                     G_CALLBACK(_agent_chat_toggle_callback), dev);
    dt_view_manager_module_toolbox_add(darktable.view_manager, dev->agent_chat.button, DT_VIEW_DARKROOM);

    dev->agent_chat.floating_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name(dev->agent_chat.floating_window, "agent_chat_window");
    gtk_window_set_icon_name(GTK_WINDOW(dev->agent_chat.floating_window), "ansel");
    gtk_window_set_title(GTK_WINDOW(dev->agent_chat.floating_window), _("ansel - live edit chat"));
    gtk_widget_set_size_request(dev->agent_chat.floating_window,
                                DT_PIXEL_APPLY_DPI(504),
                                DT_PIXEL_APPLY_DPI(624));
    gtk_window_set_transient_for(GTK_WINDOW(dev->agent_chat.floating_window),
                                 GTK_WINDOW(dt_ui_main_window(darktable.gui->ui)));
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dev->agent_chat.floating_window), TRUE);
    g_signal_connect(G_OBJECT(dev->agent_chat.floating_window), "delete-event",
                     G_CALLBACK(_agent_chat_window_delete_callback), dev);

    GtkWidget *outer = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_PIXEL_APPLY_DPI(6));
    gtk_container_set_border_width(GTK_CONTAINER(outer), DT_PIXEL_APPLY_DPI(14));
    gtk_container_add(GTK_CONTAINER(dev->agent_chat.floating_window), outer);

    GtkWidget *title_label = gtk_label_new(_("Multi-turn live edit chat"));
    gtk_widget_set_halign(title_label, GTK_ALIGN_START);
    gtk_style_context_add_class(gtk_widget_get_style_context(title_label), "heading");
    gtk_box_pack_start(GTK_BOX(outer), title_label, FALSE, FALSE, 0);

    GtkWidget *status_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, DT_PIXEL_APPLY_DPI(6));
    gtk_box_pack_start(GTK_BOX(outer), status_row, FALSE, FALSE, 0);

    dev->agent_chat.status_label = gtk_label_new(HAVE_ANSEL_AGENT_CHAT_BACKEND
                                                   ? _("ready for multi-turn live edits")
                                                   : _("agent backend not available in this build"));
    gtk_label_set_xalign(GTK_LABEL(dev->agent_chat.status_label), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(dev->agent_chat.status_label), TRUE);
    gtk_box_pack_start(GTK_BOX(status_row), dev->agent_chat.status_label, TRUE, TRUE, 0);

    dev->agent_chat.spinner = gtk_spinner_new();
    gtk_widget_set_visible(dev->agent_chat.spinner, FALSE);
    gtk_box_pack_start(GTK_BOX(status_row), dev->agent_chat.spinner, FALSE, FALSE, 0);

    dev->agent_chat.cancel_button = gtk_button_new_with_label(_("cancel request"));
    gtk_widget_set_tooltip_text(dev->agent_chat.cancel_button, _("cancel the active chat request"));
    g_signal_connect(G_OBJECT(dev->agent_chat.cancel_button), "clicked",
                     G_CALLBACK(_agent_chat_cancel_clicked), dev);
    gtk_box_pack_start(GTK_BOX(status_row), dev->agent_chat.cancel_button, FALSE, FALSE, 0);

    dev->agent_chat.new_chat_button = gtk_button_new_with_label(_("new chat"));
    gtk_widget_set_tooltip_text(dev->agent_chat.new_chat_button,
                                _("reset the current conversation for this image"));
    g_signal_connect(G_OBJECT(dev->agent_chat.new_chat_button), "clicked",
                     G_CALLBACK(_agent_chat_new_chat_clicked), dev);
    gtk_box_pack_start(GTK_BOX(status_row), dev->agent_chat.new_chat_button, FALSE, FALSE, 0);

    dev->agent_chat.error_label = gtk_label_new("");
    gtk_label_set_xalign(GTK_LABEL(dev->agent_chat.error_label), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(dev->agent_chat.error_label), TRUE);
    gtk_widget_set_visible(dev->agent_chat.error_label, FALSE);
    gtk_box_pack_start(GTK_BOX(outer), dev->agent_chat.error_label, FALSE, FALSE, 0);

    GtkWidget *content = gtk_box_new(GTK_ORIENTATION_VERTICAL, DT_PIXEL_APPLY_DPI(6));
    gtk_widget_set_vexpand(content, TRUE);
    gtk_widget_set_hexpand(content, TRUE);
    gtk_box_pack_start(GTK_BOX(outer), content, TRUE, TRUE, 0);

    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_height(GTK_SCROLLED_WINDOW(scroll), DT_PIXEL_APPLY_DPI(320));
    gtk_widget_set_vexpand(scroll, TRUE);
    gtk_widget_set_hexpand(scroll, TRUE);
    gtk_box_pack_start(GTK_BOX(content), scroll, TRUE, TRUE, 0);

    dev->agent_chat.conversation_view = gtk_label_new("");
    gtk_widget_set_vexpand(dev->agent_chat.conversation_view, TRUE);
    gtk_widget_set_hexpand(dev->agent_chat.conversation_view, TRUE);
    gtk_widget_set_halign(dev->agent_chat.conversation_view, GTK_ALIGN_FILL);
    gtk_widget_set_valign(dev->agent_chat.conversation_view, GTK_ALIGN_START);
    gtk_label_set_xalign(GTK_LABEL(dev->agent_chat.conversation_view), 0.0f);
    gtk_label_set_yalign(GTK_LABEL(dev->agent_chat.conversation_view), 0.0f);
    gtk_label_set_line_wrap(GTK_LABEL(dev->agent_chat.conversation_view), TRUE);
    gtk_label_set_line_wrap_mode(GTK_LABEL(dev->agent_chat.conversation_view), PANGO_WRAP_WORD_CHAR);
    gtk_label_set_selectable(GTK_LABEL(dev->agent_chat.conversation_view), TRUE);
    gtk_widget_set_margin_start(dev->agent_chat.conversation_view, DT_PIXEL_APPLY_DPI(8));
    gtk_widget_set_margin_end(dev->agent_chat.conversation_view, DT_PIXEL_APPLY_DPI(8));
    gtk_widget_set_margin_top(dev->agent_chat.conversation_view, DT_PIXEL_APPLY_DPI(8));
    gtk_widget_set_margin_bottom(dev->agent_chat.conversation_view, DT_PIXEL_APPLY_DPI(8));
    gtk_container_add(GTK_CONTAINER(scroll), dev->agent_chat.conversation_view);

    GtkWidget *input_row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, DT_PIXEL_APPLY_DPI(6));
    gtk_widget_set_hexpand(input_row, TRUE);
    gtk_box_pack_start(GTK_BOX(content), input_row, FALSE, FALSE, 0);

    dev->agent_chat.input_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(dev->agent_chat.input_entry),
                                   _("describe the result you want; Ansel will refine with live edits"));
    gtk_widget_set_hexpand(dev->agent_chat.input_entry, TRUE);
    dt_accels_disconnect_on_text_input(dev->agent_chat.input_entry);
    g_signal_connect(G_OBJECT(dev->agent_chat.input_entry), "activate",
                     G_CALLBACK(_agent_chat_entry_activate), dev);
    gtk_box_pack_start(GTK_BOX(input_row), dev->agent_chat.input_entry, TRUE, TRUE, 0);

    dev->agent_chat.send_button = gtk_button_new_with_label(_("send"));
    g_signal_connect(G_OBJECT(dev->agent_chat.send_button), "clicked",
                     G_CALLBACK(_agent_chat_send_clicked), dev);
    gtk_box_pack_start(GTK_BOX(input_row), dev->agent_chat.send_button, FALSE, FALSE, 0);

    _agent_chat_replace_transcript(dev, NULL);
    _agent_chat_update_sensitivity(dev);
  }

  GtkWidget *colorscheme, *mode;

  /* create rawoverexposed popup tool */
  {
    // the button
    dev->rawoverexposed.button = dtgtk_togglebutton_new(dtgtk_cairo_paint_rawoverexposed, 0, NULL);
    gtk_widget_set_tooltip_text(dev->rawoverexposed.button,
                                _("toggle raw over exposed indication\nright click for options"));
    g_signal_connect(G_OBJECT(dev->rawoverexposed.button), "clicked",
                     G_CALLBACK(_rawoverexposed_quickbutton_clicked), dev);
    dt_view_manager_module_toolbox_add(darktable.view_manager, dev->rawoverexposed.button, DT_VIEW_DARKROOM);
    dt_gui_add_help_link(dev->rawoverexposed.button, dt_get_help_url("rawoverexposed"));

    // and the popup window
    dev->rawoverexposed.floating_window = gtk_popover_new(dev->rawoverexposed.button);
    connect_button_press_release(dev->rawoverexposed.button, dev->rawoverexposed.floating_window);
    g_object_set_data(G_OBJECT(dev->rawoverexposed.button), "dt-darkroom-toolbox-popover",
                      dev->rawoverexposed.floating_window);
    dt_accels_new_darkroom_action(_darkroom_toolbox_button_activate_accel, dev->rawoverexposed.button,
                                  N_("Darkroom/Toolbox"),
                                  N_("Toggle raw over exposed indication"), 0, 0,
                                  _("Triggers the action"));
    dt_accels_new_darkroom_action(_darkroom_toolbox_button_focus_accel, dev->rawoverexposed.button,
                                  N_("Darkroom/Toolbox"),
                                  N_("Focus raw over exposed indication options"), 0, 0,
                                  _("Shows the options popover"));

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(dev->rawoverexposed.floating_window), vbox);

    /** let's fill the encapsulating widgets */
    /* mode of operation */
    DT_BAUHAUS_COMBOBOX_NEW_FULL(darktable.bauhaus, mode, NULL, N_("mode"),
                                 _("select how to mark the clipped pixels"),
                                 dev->rawoverexposed.mode, rawoverexposed_mode_callback, dev,
                                 N_("mark with CFA color"), N_("mark with solid color"), N_("false color"));
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(mode), TRUE, TRUE, 0);

    /* color scheme */
    // FIXME can't use DT_BAUHAUS_COMBOBOX_NEW_FULL because of (unnecessary?) translation context
    colorscheme = dt_bauhaus_combobox_new(darktable.bauhaus, DT_GUI_MODULE(NULL));
    dt_bauhaus_widget_set_label(colorscheme, N_("color scheme"));
    dt_bauhaus_combobox_add(colorscheme, C_("solidcolor", "red"));
    dt_bauhaus_combobox_add(colorscheme, C_("solidcolor", "green"));
    dt_bauhaus_combobox_add(colorscheme, C_("solidcolor", "blue"));
    dt_bauhaus_combobox_add(colorscheme, C_("solidcolor", "black"));
    dt_bauhaus_combobox_set(colorscheme, dev->rawoverexposed.colorscheme);
    gtk_widget_set_tooltip_text(
        colorscheme,
        _("select the solid color to indicate over exposure.\nwill only be used if mode = mark with solid color"));
    g_signal_connect(G_OBJECT(colorscheme), "value-changed", G_CALLBACK(rawoverexposed_colorscheme_callback), dev);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(colorscheme), TRUE, TRUE, 0);

    /* threshold */
    GtkWidget *threshold = dt_bauhaus_slider_new_with_range(darktable.bauhaus, DT_GUI_MODULE(NULL), 0.0, 2.0, 0.01, 1.0, 3);
    dt_bauhaus_slider_set(threshold, dev->rawoverexposed.threshold);
    dt_bauhaus_widget_set_label(threshold, N_("clipping threshold"));
    gtk_widget_set_tooltip_text(
        threshold, _("threshold of what shall be considered overexposed\n1.0 - white level\n0.0 - black level"));
    g_signal_connect(G_OBJECT(threshold), "value-changed", G_CALLBACK(rawoverexposed_threshold_callback), dev);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(threshold), TRUE, TRUE, 0);

    gtk_widget_show_all(vbox);
  }

  /* create overexposed popup tool */
  {
    // the button
    dev->overexposed.button = dtgtk_togglebutton_new(dtgtk_cairo_paint_overexposed, 0, NULL);
    gtk_widget_set_tooltip_text(dev->overexposed.button,
                                _("toggle clipping indication\nright click for options"));
    g_signal_connect(G_OBJECT(dev->overexposed.button), "clicked",
                     G_CALLBACK(_overexposed_quickbutton_clicked), dev);
    dt_view_manager_module_toolbox_add(darktable.view_manager, dev->overexposed.button, DT_VIEW_DARKROOM);
    dt_gui_add_help_link(dev->overexposed.button, dt_get_help_url("overexposed"));

    // and the popup window
    dev->overexposed.floating_window = gtk_popover_new(dev->overexposed.button);
    connect_button_press_release(dev->overexposed.button, dev->overexposed.floating_window);
    g_object_set_data(G_OBJECT(dev->overexposed.button), "dt-darkroom-toolbox-popover",
                      dev->overexposed.floating_window);
    dt_accels_new_darkroom_action(_darkroom_toolbox_button_activate_accel, dev->overexposed.button,
                                  N_("Darkroom/Toolbox"),
                                  N_("Toggle clipping indication"), 0, 0,
                                  _("Triggers the action"));
    dt_accels_new_darkroom_action(_darkroom_toolbox_button_focus_accel, dev->overexposed.button,
                                  N_("Darkroom/Toolbox"),
                                  N_("Focus clipping indication options"), 0, 0,
                                  _("Shows the options popover"));

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(dev->overexposed.floating_window), vbox);

    /** let's fill the encapsulating widgets */
    /* preview mode */
    DT_BAUHAUS_COMBOBOX_NEW_FULL(darktable.bauhaus, mode, NULL, N_("clipping preview mode"),
                                 _("select the metric you want to preview\nfull gamut is the combination of all other modes"),
                                 dev->overexposed.mode, mode_callback, dev,
                                 N_("full gamut"), N_("any RGB channel"), N_("luminance only"), N_("saturation only"));
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(mode), TRUE, TRUE, 0);

    /* color scheme */
    DT_BAUHAUS_COMBOBOX_NEW_FULL(darktable.bauhaus, colorscheme, NULL, N_("color scheme"),
                                 _("select colors to indicate clipping"),
                                 dev->overexposed.colorscheme, colorscheme_callback, dev,
                                 N_("black & white"), N_("red & blue"), N_("purple & green"));
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(colorscheme), TRUE, TRUE, 0);

    /* lower */
    GtkWidget *lower = dt_bauhaus_slider_new_with_range(darktable.bauhaus, DT_GUI_MODULE(NULL), -32., -4., 1., -12.69, 2);
    dt_bauhaus_slider_set(lower, dev->overexposed.lower);
    dt_bauhaus_slider_set_format(lower, _(" EV"));
    dt_bauhaus_widget_set_label(lower, N_("lower threshold"));
    gtk_widget_set_tooltip_text(lower, _("clipping threshold for the black point,\n"
                                         "in EV, relatively to white (0 EV).\n"
                                         "8 bits sRGB clips blacks at -12.69 EV,\n"
                                         "8 bits Adobe RGB clips blacks at -19.79 EV,\n"
                                         "16 bits sRGB clips blacks at -20.69 EV,\n"
                                         "typical fine-art mat prints produce black at -5.30 EV,\n"
                                         "typical color glossy prints produce black at -8.00 EV,\n"
                                         "typical B&W glossy prints produce black at -9.00 EV."
                                         ));
    g_signal_connect(G_OBJECT(lower), "value-changed", G_CALLBACK(lower_callback), dev);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(lower), TRUE, TRUE, 0);

    /* upper */
    GtkWidget *upper = dt_bauhaus_slider_new_with_range(darktable.bauhaus, DT_GUI_MODULE(NULL), 0.0, 100.0, 0.1, 99.99, 2);
    dt_bauhaus_slider_set(upper, dev->overexposed.upper);
    dt_bauhaus_slider_set_format(upper, "%");
    dt_bauhaus_widget_set_label(upper, N_("upper threshold"));
    /* xgettext:no-c-format */
    gtk_widget_set_tooltip_text(upper, _("clipping threshold for the white point.\n"
                                         "100% is peak medium luminance."));
    g_signal_connect(G_OBJECT(upper), "value-changed", G_CALLBACK(upper_callback), dev);
    gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(upper), TRUE, TRUE, 0);

    gtk_widget_show_all(vbox);
  }

  /* create profile popup tool & buttons (softproof + gamut) */
  {
    // the softproof button
    dev->profile.softproof_button = dtgtk_togglebutton_new(dtgtk_cairo_paint_softproof, 0, NULL);
    gtk_widget_set_tooltip_text(dev->profile.softproof_button,
                                _("toggle softproofing\nright click for profile options"));
    g_signal_connect(G_OBJECT(dev->profile.softproof_button), "clicked",
                     G_CALLBACK(_softproof_quickbutton_clicked), dev);
    dt_view_manager_module_toolbox_add(darktable.view_manager, dev->profile.softproof_button, DT_VIEW_DARKROOM);
    dt_gui_add_help_link(dev->profile.softproof_button, dt_get_help_url("softproof"));

    // the gamut check button
    dev->profile.gamut_button = dtgtk_togglebutton_new(dtgtk_cairo_paint_gamut_check, 0, NULL);
    gtk_widget_set_tooltip_text(dev->profile.gamut_button,
                 _("toggle gamut checking\nright click for profile options"));
    g_signal_connect(G_OBJECT(dev->profile.gamut_button), "clicked",
                     G_CALLBACK(_gamut_quickbutton_clicked), dev);
    dt_view_manager_module_toolbox_add(darktable.view_manager, dev->profile.gamut_button, DT_VIEW_DARKROOM);
    dt_gui_add_help_link(dev->profile.gamut_button, dt_get_help_url("gamut"));

    // and the popup window, which is shared between the two profile buttons
    dev->profile.floating_window = gtk_popover_new(NULL);
    connect_button_press_release(dev->profile.softproof_button, dev->profile.floating_window);
    connect_button_press_release(dev->profile.gamut_button, dev->profile.floating_window);
    g_object_set_data(G_OBJECT(dev->profile.softproof_button), "dt-darkroom-toolbox-popover",
                      dev->profile.floating_window);
    g_object_set_data(G_OBJECT(dev->profile.gamut_button), "dt-darkroom-toolbox-popover",
                      dev->profile.floating_window);
    dt_accels_new_darkroom_action(_darkroom_toolbox_button_activate_accel,
                                  dev->profile.softproof_button, N_("Darkroom/Toolbox"),
                                  N_("Toggle softproofing"), 0, 0,
                                  _("Triggers the action"));
    dt_accels_new_darkroom_action(_darkroom_toolbox_button_focus_accel,
                                  dev->profile.softproof_button, N_("Darkroom/Toolbox"),
                                  N_("Focus softproof options"), 0, 0,
                                  _("Shows the options popover"));
    dt_accels_new_darkroom_action(_darkroom_toolbox_button_activate_accel,
                                  dev->profile.gamut_button, N_("Darkroom/Toolbox"),
                                  N_("Toggle gamut checking"), 0, 0,
                                  _("Triggers the action"));
    dt_accels_new_darkroom_action(_darkroom_toolbox_button_focus_accel,
                                  dev->profile.gamut_button, N_("Darkroom/Toolbox"),
                                  N_("Focus gamut checking options"), 0, 0,
                                  _("Shows the options popover"));

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(dev->profile.floating_window), vbox);

    /** let's fill the encapsulating widgets */
    char datadir[PATH_MAX] = { 0 };
    char confdir[PATH_MAX] = { 0 };
    dt_loc_get_user_config_dir(confdir, sizeof(confdir));
    dt_loc_get_datadir(datadir, sizeof(datadir));

    GtkWidget *softproof_profile = dt_bauhaus_combobox_new(darktable.bauhaus, DT_GUI_MODULE(NULL));
    dt_bauhaus_widget_set_label(softproof_profile, N_("softproof profile"));
    dt_bauhaus_combobox_set_entries_ellipsis(softproof_profile, PANGO_ELLIPSIZE_MIDDLE);
    gtk_box_pack_start(GTK_BOX(vbox), softproof_profile, TRUE, TRUE, 0);

    for(const GList *l = darktable.color_profiles->profiles; l; l = g_list_next(l))
    {
      dt_colorspaces_color_profile_t *prof = (dt_colorspaces_color_profile_t *)l->data;
      // the system display profile is only suitable for display purposes
      if(prof->out_pos > -1)
      {
        dt_bauhaus_combobox_add(softproof_profile, prof->name);
        if(prof->type == darktable.color_profiles->softproof_type
          && (prof->type != DT_COLORSPACE_FILE
              || !strcmp(prof->filename, darktable.color_profiles->softproof_filename)))
          dt_bauhaus_combobox_set(softproof_profile, prof->out_pos);
      }
    }

    char *system_profile_dir = g_build_filename(datadir, "color", "out", NULL);
    char *user_profile_dir = g_build_filename(confdir, "color", "out", NULL);
    char *tooltip = g_strdup_printf(_("softproof ICC profiles in %s or %s"), user_profile_dir, system_profile_dir);
    gtk_widget_set_tooltip_text(softproof_profile, tooltip);
    dt_free(tooltip);
    dt_free(system_profile_dir);
    dt_free(user_profile_dir);

    g_signal_connect(G_OBJECT(softproof_profile), "value-changed", G_CALLBACK(softproof_profile_callback), dev);

    _update_softproof_gamut_checking(dev);

    gtk_widget_show_all(vbox);
  }

  /* create grid changer popup tool */
  {
    // the button
    darktable.view_manager->guides_toggle = dtgtk_togglebutton_new(dtgtk_cairo_paint_grid, 0, NULL);
    gtk_widget_set_tooltip_text(darktable.view_manager->guides_toggle,
                                _("toggle guide lines\nright click for guides options"));
    darktable.view_manager->guides_popover = dt_guides_popover(self, darktable.view_manager->guides_toggle);
    g_object_ref(darktable.view_manager->guides_popover);
    g_signal_connect(G_OBJECT(darktable.view_manager->guides_toggle), "clicked",
                     G_CALLBACK(_guides_quickbutton_clicked), dev);
    connect_button_press_release(darktable.view_manager->guides_toggle, darktable.view_manager->guides_popover);
    g_object_set_data(G_OBJECT(darktable.view_manager->guides_toggle), "dt-darkroom-toolbox-popover",
                      darktable.view_manager->guides_popover);
    dt_view_manager_module_toolbox_add(darktable.view_manager, darktable.view_manager->guides_toggle,
                                       DT_VIEW_DARKROOM);
    dt_accels_new_darkroom_action(_darkroom_toolbox_button_activate_accel,
                                  darktable.view_manager->guides_toggle, N_("Darkroom/Toolbox"),
                                  N_("Toggle guide lines"), 0, 0,
                                  _("Triggers the action"));
    dt_accels_new_darkroom_action(_darkroom_toolbox_button_focus_accel,
                                  darktable.view_manager->guides_toggle, N_("Darkroom/Toolbox"),
                                  N_("Focus guide lines options"), 0, 0,
                                  _("Shows the options popover"));
    // we want to update button state each time the view change
    DT_DEBUG_CONTROL_SIGNAL_CONNECT(darktable.signals, DT_SIGNAL_VIEWMANAGER_VIEW_CHANGED,
                                    G_CALLBACK(_guides_view_changed), dev);
  }

  darktable.view_manager->proxy.darkroom.get_layout = _lib_darkroom_get_layout;
  dev->roi.border_size = DT_PIXEL_APPLY_DPI(dt_conf_get_int("plugins/darkroom/ui/border_size"));
}

static gboolean _is_scroll_captured_by_widget()
{
  dt_accels_t *accels = darktable.gui->accels;
  if(!darktable.gui->has_scroll_focus || accels->active_key.accel_key == 0) return FALSE;

  // When declaring shortcuts, bauhaus widgets write their accel path into a private data field
  gchar *accel_path = g_object_get_data(G_OBJECT(darktable.gui->has_scroll_focus), "accel-path");

  // Find if the registered accel keys matches currently pressed keys
  GtkAccelKey key = { 0 };
  return gtk_accel_map_lookup_entry(accel_path, &key)
    && key.accel_key == accels->active_key.accel_key
    && key.accel_mods == accels->active_key.accel_mods;
}


// If a bauhaus widget has the scroll focus from a keyboard shortcut,
// and the combination of keys attached to its accel path
// is still pressed, then we redirect any scroll event in the window to this widget.
// Warning: if mouse is over the central widget, central widget takes precedence over scrolling.
gboolean _scroll_on_focus(GdkEventScroll event, void *data)
{
  if(_is_scroll_captured_by_widget())
  {
    // Pass-through the scrolling event to the scrolling handler of the widget
    gboolean ret;
    g_signal_emit_by_name(G_OBJECT(darktable.gui->has_scroll_focus), "scroll-event", &event, &ret);
    return ret;
  }

  return FALSE;
}


void enter(dt_view_t *self)
{
  // Flush all background jobs (thumbnails generation) to spare resources for interactivity
  dt_control_flush_jobs_queue(darktable.control, DT_JOB_QUEUE_SYSTEM_FG);
  
  dt_print(DT_DEBUG_CONTROL, "[run_job+] 11 %f in darkroom mode\n", dt_get_wtime());
  dt_develop_t *dev = (dt_develop_t *)self->data;
  dev->exit = 0;
  _darkroom_pending_focus_module = NULL;

  // We need to init forms before we init module blending GUI
  dt_masks_gui_init(dev);
  dev->gui_module = NULL;

  if(IS_NULL_PTR(dev->iop))
    dev->iop = dt_dev_load_modules(dev);

  // Add IOP modules to the plugin list
  char option[1024];
  const char *active_plugin = dt_conf_get_string_const("plugins/darkroom/active");
  for(const GList *modules = g_list_first(dev->iop); modules; modules = g_list_next(modules))
  {
    dt_iop_module_t *module = (dt_iop_module_t *)(modules->data);

    /* initialize gui if iop have one defined */
    if(!dt_iop_is_hidden(module))
    {
      dt_iop_gui_init(module);
      dt_iop_gui_set_expander(module);

      if(module->multi_priority == 0)
      {
        snprintf(option, sizeof(option), "plugins/darkroom/%s/expanded", module->op);
        module->expanded = dt_conf_get_bool(option);
        dt_iop_gui_update_expanded(module);

        if(active_plugin && !strcmp(module->op, active_plugin))
          _darkroom_pending_focus_module = module;
      }
    }
  }

  // just make sure at this stage we have only history info into the undo, all automatic
  // tagging should be ignored.
  dt_undo_clear(darktable.undo, DT_UNDO_TAGS);

  dt_iop_color_picker_init();

  // Reset focus to center view
  dt_gui_refocus_center();

  // Attach shortcuts to new widgets
  dt_accels_connect_accels(darktable.gui->accels);
  dt_accels_connect_active_group(darktable.gui->accels, "darkroom");
  dt_accels_attach_scroll_handler(darktable.gui->accels, _scroll_on_focus, dev);

  // Attach bauhaus default signal callback to IOP
  darktable.bauhaus->default_value_changed_callback = dt_bauhaus_value_changed_default_callback;

  // This gets the first selected ID to scroll where relevant, so
  // runs it before clearing the selection
  dt_thumbtable_show(darktable.gui->ui->thumbtable_filmstrip);
  gtk_widget_show(dt_ui_center(darktable.gui->ui));
  dt_thumbtable_update_parent(darktable.gui->ui->thumbtable_filmstrip);

  /* connect signal for filmstrip image activate */
  DT_DEBUG_CONTROL_SIGNAL_CONNECT(darktable.signals, DT_SIGNAL_VIEWMANAGER_THUMBTABLE_ACTIVATE,
                                  G_CALLBACK(_view_darkroom_filmstrip_activate_callback), self);
  DT_DEBUG_CONTROL_SIGNAL_CONNECT(darktable.signals, DT_SIGNAL_DEVELOP_IMAGE_CHANGED,
                                  G_CALLBACK(_agent_chat_image_changed_callback), self);
  DT_DEBUG_CONTROL_SIGNAL_CONNECT(darktable.signals, DT_SIGNAL_DEVELOP_UI_PIPE_FINISHED,
                                  G_CALLBACK(_darkroom_ui_pipe_finish_signal_callback), self);

  gtk_widget_grab_focus(dt_ui_center(darktable.gui->ui)); // ensure the center view has focus for keybindings to work

  const int32_t imgid = _darkroom_pending_imgid;
  _darkroom_pending_imgid = UNKNOWN_IMAGE;
  dt_control_set_mouse_over_id(imgid);
  dt_control_set_keyboard_over_id(imgid);
  g_idle_add((GSourceFunc)dt_thumbtable_scroll_to_selection, darktable.gui->ui->thumbtable_filmstrip);
  int ret = dt_dev_load_image(darktable.develop, imgid);
  _darkroom_image_loaded_callback(NULL, imgid, ret, self);
  _agent_chat_reset_for_current_image(dev, TRUE);
#if HAVE_ANSEL_AGENT_CHAT_BACKEND
  if(g_getenv("ANSEL_AGENT_TEST_AUTORUN_PROMPT"))
    g_idle_add(_agent_chat_test_autorun_idle, dev);
#endif
}

void leave(dt_view_t *self)
{
  dt_develop_t *dev = (dt_develop_t *)self->data;
  dt_gui_throttle_cancel(dev);

  _release_expose_source_caches();
  if(dev->image_surface) cairo_surface_destroy(dev->image_surface);
  dev->image_surface = NULL;

  // Send all pipeline shutdown signals first
  dev->exit = 1;
  dt_atomic_set_int(&dev->pipe->shutdown, TRUE);
  dt_atomic_set_int(&dev->preview_pipe->shutdown, TRUE);
  if(dev->virtual_pipe) dt_atomic_set_int(&dev->virtual_pipe->shutdown, TRUE);
  dev->pipelines_started = FALSE;

  _darkroom_pending_focus_module = NULL;

  // While we wait for possible pipelines to finish,
  // do the GUI cleaning.

  // store last active plugin:
  if(darktable.develop->gui_module)
    dt_conf_set_string("plugins/darkroom/active", darktable.develop->gui_module->op);
  else
    dt_conf_set_string("plugins/darkroom/active", "");

  // Hide the popover floating windows
  gtk_widget_hide(dev->overexposed.floating_window);
  gtk_widget_hide(dev->rawoverexposed.floating_window);
  gtk_widget_hide(dev->profile.floating_window);
  if(dev->agent_chat.floating_window)
  {
    _agent_chat_window_write_config(dev->agent_chat.floating_window);
    gtk_widget_hide(dev->agent_chat.floating_window);
  }
  if(dev->agent_chat.button)
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(dev->agent_chat.button), FALSE);
  _agent_chat_cancel_active_request(dev, _("Request canceled"));

  // Detach the default callback for bauhaus widgets
  dt_accels_detach_scroll_handler(darktable.gui->accels);

  /* disconnect from filmstrip image activate */
  DT_DEBUG_CONTROL_SIGNAL_DISCONNECT(darktable.signals, G_CALLBACK(_view_darkroom_filmstrip_activate_callback),
  (gpointer)self);
  DT_DEBUG_CONTROL_SIGNAL_DISCONNECT(darktable.signals, G_CALLBACK(_agent_chat_image_changed_callback),
  (gpointer)self);
  DT_DEBUG_CONTROL_SIGNAL_DISCONNECT(darktable.signals, G_CALLBACK(_darkroom_ui_pipe_finish_signal_callback),
  (gpointer)self);

  dt_iop_color_picker_cleanup();

  if(darktable.develop->color_picker.picker)
    dt_iop_color_picker_reset(darktable.develop->color_picker.picker->module, FALSE);

  // Detach shortcuts
  dt_accels_disconnect_active_group(darktable.gui->accels);

  // Restore the previous selection
  dt_selection_select_single(darktable.selection, dt_view_active_images_get_first());
  dt_view_active_images_reset(FALSE);

  dt_thumbtable_hide(darktable.gui->ui->thumbtable_filmstrip);
  gtk_widget_hide(dt_ui_center(darktable.gui->ui));

  // Pipeline nodes reference modules from dev->iop
  // we need to destroy objects referencing modules
  // before destroying the actual modules being referenced.
  dt_pthread_mutex_lock(&dev->pipe->busy_mutex);
  dt_dev_pixelpipe_cleanup_nodes(dev->pipe);
  dt_dev_pixelpipe_cache_unref_hash(darktable.pixelpipe_cache, dt_dev_backbuf_get_hash(&dev->pipe->backbuf));
  dt_dev_set_backbuf(&dev->pipe->backbuf, 0, 0, 0, DT_PIXELPIPE_CACHE_HASH_INVALID, DT_PIXELPIPE_CACHE_HASH_INVALID);
  dt_pthread_mutex_unlock(&dev->pipe->busy_mutex);

  dt_pthread_mutex_lock(&dev->preview_pipe->busy_mutex);
  dt_dev_pixelpipe_cleanup_nodes(dev->preview_pipe);
  dt_dev_pixelpipe_cache_unref_hash(darktable.pixelpipe_cache, dt_dev_backbuf_get_hash(&dev->preview_pipe->backbuf));
  dt_dev_set_backbuf(&dev->preview_pipe->backbuf, 0, 0, 0, DT_PIXELPIPE_CACHE_HASH_INVALID,
                     DT_PIXELPIPE_CACHE_HASH_INVALID);
  dt_pthread_mutex_unlock(&dev->preview_pipe->busy_mutex);

  dt_pthread_mutex_lock(&dev->virtual_pipe->busy_mutex);
  dt_dev_pixelpipe_cleanup_nodes(dev->virtual_pipe);
  dt_dev_pixelpipe_cache_unref_hash(darktable.pixelpipe_cache, dt_dev_backbuf_get_hash(&dev->virtual_pipe->backbuf));
  dt_dev_set_backbuf(&dev->virtual_pipe->backbuf, 0, 0, 0, DT_PIXELPIPE_CACHE_HASH_INVALID,
                     DT_PIXELPIPE_CACHE_HASH_INVALID);
  dt_pthread_mutex_unlock(&dev->virtual_pipe->busy_mutex);

  /* Device-side cache payloads are only an acceleration layer. Once darkroom
   * leaves and all pipe workers are quiescent, drop all cached cl_mem objects
   * so a later reopen can only exact-hit host-authoritative cachelines. */
  dt_dev_pixelpipe_cache_flush_clmem(darktable.pixelpipe_cache, -1, NULL);

  dt_pthread_rwlock_wrlock(&dev->history_mutex);
  dt_dev_history_free_history(dev);
  dt_pthread_rwlock_unlock(&dev->history_mutex);

  // Not sure why using g_list_free_full() here shits the bed
  while(dev->iop)
  {
    dt_iop_module_t *module = (dt_iop_module_t *)(dev->iop->data);
    if(!dt_iop_is_hidden(module)) dt_iop_gui_cleanup_module(module);
    dt_iop_cleanup_module(module);
    dt_free(module);
    dev->iop = g_list_delete_link(dev->iop, dev->iop);
  }
  while(dev->alliop)
  {
    dt_iop_cleanup_module((dt_iop_module_t *)dev->alliop->data);
    dt_free(dev->alliop->data);
    dev->alliop = g_list_delete_link(dev->alliop, dev->alliop);
  }
  dev->iop = dev->alliop = NULL;

  // cleanup visible masks
  if(dev->form_gui)
  {
    dev->gui_module = NULL; // modules have already been g_free()
    dt_masks_gui_cleanup(dev);
  }

  // clear masks
  dt_pthread_rwlock_wrlock(&dev->masks_mutex);
  g_list_free_full(dev->forms, (void (*)(void *))dt_masks_free_form);
  dev->forms = NULL;
  g_list_free_full(dev->allforms, (void (*)(void *))dt_masks_free_form);
  dev->allforms = NULL;
  dt_pthread_rwlock_unlock(&dev->masks_mutex);

  // Fetch the new thumbnail if needed. Ensure it runs after we save history.
  dt_thumbtable_refresh_thumbnail(darktable.gui->ui->thumbtable_lighttable, darktable.develop->image_storage.id, TRUE);
  darktable.develop->image_storage.id = -1;

  // Release the cache entries for histogram buffers
  dt_dev_pixelpipe_cache_unref_hash(darktable.pixelpipe_cache, dt_dev_backbuf_get_hash(&dev->raw_histogram));
  dt_dev_backbuf_set_hash(&dev->raw_histogram, -1);

  dt_dev_pixelpipe_cache_unref_hash(darktable.pixelpipe_cache, dt_dev_backbuf_get_hash(&dev->output_histogram));
  dt_dev_backbuf_set_hash(&dev->output_histogram, -1);

  dt_dev_pixelpipe_cache_unref_hash(darktable.pixelpipe_cache, dt_dev_backbuf_get_hash(&dev->display_histogram));
  dt_dev_backbuf_set_hash(&dev->display_histogram, -1);

  /* GUI backbuffers were already released when each pipeline was quiesced above. Keep the view-side teardown
   * free of extra unref paths so pipeline ownership stays centralized. */


  dt_print(DT_DEBUG_CONTROL, "[run_job-] 11 %f in darkroom mode\n", dt_get_wtime());
}

void mouse_leave(dt_view_t *self)
{
  // if we are not hovering over a thumbnail in the filmstrip -> show metadata of opened image.
  dt_develop_t *dev = (dt_develop_t *)self->data;

  // masks
  gboolean handled = FALSE;
  if(dt_masks_get_visible_form(dev) && dt_masks_events_mouse_leave(dev->gui_module))
    handled = TRUE;
  // module
  else if(dev->gui_module && dev->gui_module->mouse_leave
          && dev->gui_module->mouse_leave(dev->gui_module))
    handled = TRUE;

  if(handled)
    dt_control_queue_redraw_center();

  // reset any changes the selected plugin might have made.
  dt_control_set_cursor(GDK_LEFT_PTR);
  dt_control_change_cursor(GDK_LEFT_PTR);
}

static gboolean _is_in_frame(const int width, const int height, const int x, const int y)
{
  return !((x < -DT_PIXEL_APPLY_DPI(2)) ||
           (x > (width + DT_PIXEL_APPLY_DPI(4))) ||
           (y < -DT_PIXEL_APPLY_DPI(2)) ||
           (y > (height + DT_PIXEL_APPLY_DPI(4))));
}

/* This helper function tests for a position to be within the displayed area
   of an image. To avoid "border cases" we accept values to be slightly out of area too.
*/
static gboolean mouse_in_imagearea(dt_view_t *self, double x, double y)
{
  dt_develop_t *dev = (dt_develop_t *)self->data;
  float image_box[4] = { 0.0f };
  dt_dev_get_image_box_in_widget(dev, self->width, self->height, image_box);

  return _is_in_frame(image_box[2], image_box[3], round(x - image_box[0]), round(y - image_box[1]));
}

static gboolean mouse_in_actionarea(dt_view_t *self, double x, double y)
{
  return _is_in_frame(self->width, self->height, round(x), round(y));
}

static void _set_default_cursor(dt_view_t *self, double x, double y)
{
  if(mouse_in_imagearea(self, x, y))
    dt_control_set_cursor(GDK_DOT);
  else if(mouse_in_actionarea(self, x, y))
    dt_control_set_cursor(GDK_CROSSHAIR);
  else
    dt_control_set_cursor(GDK_LEFT_PTR);
}

void mouse_enter(dt_view_t *self)
{
  dt_develop_t *dev = (dt_develop_t *)self->data;
  dt_masks_events_mouse_enter(dev->gui_module);
}

static void _delayed_history_commit(gpointer data)
{
  dt_develop_t *dev = (dt_develop_t *)data;

  // Figure out if an history item needs to be added
  // aka drawn masks have changed somehow. This is more expensive
  // but more reliable than handling individually all editing operations
  // in all callbacks in all possible mask types.
  dt_pthread_rwlock_wrlock(&dev->history_mutex);
  dt_dev_masks_update_hash(dev);
  dt_pthread_rwlock_unlock(&dev->history_mutex);

  if(dev->forms_changed)
    dt_dev_add_history_item(dev, dev->gui_module, FALSE, TRUE);
}

void mouse_moved(dt_view_t *self, double x, double y, double pressure, int which)
{
  dt_develop_t *dev = (dt_develop_t *)self->data;
  dt_control_t *ctl = darktable.control;
  const gboolean picker_active = dt_iop_color_picker_is_visible(dev);

  // change cursor appearance by default
  _set_default_cursor(self, x, y);
  gboolean ret = FALSE;

  if(picker_active && ctl->button_down && ctl->button_down_which == 1)
  {
    // module requested a color box
    if(mouse_in_imagearea(self, x, y))
    {
      dt_colorpicker_sample_t *const sample = darktable.develop->color_picker.primary_sample;
      gboolean sample_changed = FALSE;
      float mouse_point[2] = { (float)x, (float)y };
      dt_dev_coordinates_widget_to_image_norm(dev, mouse_point, 1);
      const float delta[2] = {
        1.0f / dev->roi.processed_width,
        1.0f / dev->roi.processed_height
      };

      if(sample->size == DT_LIB_COLORPICKER_SIZE_BOX)
      {
        const float box[4] = {
          fmaxf(0.0, MIN(sample->point[0], mouse_point[0]) - delta[0]),
          fmaxf(0.0, MIN(sample->point[1], mouse_point[1]) - delta[1]),
          fminf(1.0, MAX(sample->point[0], mouse_point[0]) + delta[0]),
          fminf(1.0, MAX(sample->point[1], mouse_point[1]) + delta[1])
        };

        for(int k = 0; k < 4; k++)
          sample_changed |= (sample->box[k] != box[k]);

        if(sample_changed)
          dt_lib_colorpicker_set_box_area(darktable.lib, box);
      }
      else if(sample->size == DT_LIB_COLORPICKER_SIZE_POINT)
      {
        sample_changed = memcmp(sample->point, mouse_point, sizeof(mouse_point)) != 0;
        if(sample_changed)
          dt_lib_colorpicker_set_point(darktable.lib, mouse_point);
      }
    }
    ret = TRUE;
  }
  else if(picker_active)
  {
    // Keep module-specific hover overlays and live-edit cursors disabled while the picker owns the center view.
    ret = TRUE;
  }

  // masks
  else if(dt_masks_get_visible_form(dev)
          && dt_masks_events_mouse_moved(dev->gui_module, x, y, pressure, which))
  {
    dt_gui_throttle_queue(dev, _delayed_history_commit, dev);
    ret = TRUE;
  }

  // module
  else if(dev->gui_module && dev->gui_module->mouse_moved
    &&dev->gui_module->mouse_moved(dev->gui_module, x, y, pressure, which))
  {
    ret = TRUE;
  }

  dt_control_commit_cursor();

  if(ret)
  {
    dt_control_queue_redraw_center();
    return;
  }

  // panning with left mouse button
  if(darktable.control->button_down && darktable.control->button_down_which == 1 && dev->roi.scaling > 1)
  {
    float delta[2] = { x - ctl->button_x, y - ctl->button_y };
    dt_dev_coordinates_widget_delta_to_image_delta(dev, delta, 1);

    // new roi position in full image scale
    float roi[2] = {
      dev->roi.x - (delta[0] / dev->roi.processed_width),
      dev->roi.y - (delta[1] / dev->roi.processed_height)
    };
    dt_dev_check_zoom_pos_bounds(dev, &roi[0], &roi[1], NULL, NULL); 

    dev->roi.x = roi[0];
    dev->roi.y = roi[1];

    // update clicked position
    ctl->button_x = x;
    ctl->button_y = y;

    dt_dev_pixelpipe_change_zoom_main(dev);
  }
}


int button_released(dt_view_t *self, double x, double y, int which, uint32_t state)
{
  dt_develop_t *dev = (dt_develop_t *)self->data;

  dt_print(DT_DEBUG_INPUT, "[darkroom] button released which: %d state: %d x: %.2f y: %.2f\n",
           which, state, x, y);

  if(dt_iop_color_picker_is_visible(dev) && which == 1)
  {
    // only sample box picker at end, for speed
    if(darktable.develop->color_picker.primary_sample->size == DT_LIB_COLORPICKER_SIZE_BOX)
      dt_control_set_cursor(GDK_LEFT_PTR);

    dt_control_queue_redraw_center();
    return 1;
  }
  // masks
  if(dt_masks_get_visible_form(dev)
     && dt_masks_events_button_released(dev->gui_module, x, y, which, state))
  {
    // Change on mask parameters and image output.
    dt_gui_throttle_queue(dev, _delayed_history_commit, dev);
    dt_dev_pixelpipe_update_history_preview(dev); // Needed if mask selection changed
    dt_control_queue_redraw_center();
    return 1;
  }

  // module
  if(dev->gui_module && dev->gui_module->enabled && dev->gui_module->button_released
     && dev->gui_module->button_released(dev->gui_module, x, y, which, state))
  {
    // Click in modules should handle history changes internally.
    return 1;
  }

  return 0;
}


int button_pressed(dt_view_t *self, double x, double y, double pressure, int which, int type, uint32_t state)
{
  dt_colorpicker_sample_t *const sample = darktable.develop->color_picker.primary_sample;
  dt_develop_t *dev = (dt_develop_t *)self->data;

  dt_print(DT_DEBUG_INPUT, "[darkroom] button pressed  which: %d  type: %d x: %.2f y: %.2f pressure: %f\n",
           which, type, x, y, pressure);

  // Grab focus on any click so we can interact from keyboard
  gtk_widget_grab_focus(dt_ui_center(darktable.gui->ui));

  if(dt_iop_color_picker_is_visible(dev))
  {
    float point[2] = { (float)x, (float)y };
    dt_dev_coordinates_widget_to_image_norm(dev, point, 1);

    const float zoom_scale = dt_dev_get_fit_scale(dev);
    float handle[2] = { 6.0f, 6.0f };
    dt_dev_coordinates_widget_delta_to_image_delta(dev, handle, 1);
    handle[0] /= dev->roi.processed_width;
    handle[1] /= dev->roi.processed_height;

    if(which == 1)
    {
      if(mouse_in_imagearea(self, x, y))
      {
        // The default box will be a square with 1% of the image width
        const float delta_x = 0.01f;
        const float delta_y = delta_x * (float)dev->roi.processed_width / (float)dev->roi.processed_height;

        if(sample->size == DT_LIB_COLORPICKER_SIZE_BOX)
        {
          /* Box drags need one anchor corner stored in sample->point so mouse motion can stretch
             the rectangle against that fixed corner. Keep that anchor local to the darkroom drag
             state, but let the picker API own the actual sampling geometry and update signaling. */
          memcpy(sample->point, point, sizeof(point));

          // this is slightly more than as drawn, to give room for slop
          gboolean on_corner_prev_box = TRUE;
          float opposite[2] = { 0.0f };

          if(fabsf(point[0] - sample->box[0]) <= handle[0])
            opposite[0] = sample->box[2];
          else if(fabsf(point[0] - sample->box[2]) <= handle[0])
            opposite[0] = sample->box[0];
          else
            on_corner_prev_box = FALSE;

          if(fabsf(point[1] - sample->box[1]) <= handle[1])
            opposite[1] = sample->box[3];
          else if(fabsf(point[1] - sample->box[3]) <= handle[1])
            opposite[1] = sample->box[1];
          else
            on_corner_prev_box = FALSE;

          if(on_corner_prev_box)
            memcpy(sample->point, opposite, sizeof(opposite));
          else
          {
            const dt_boundingbox_t box = {
              fmaxf(0.0, point[0] - delta_x),
              fmaxf(0.0, point[1] - delta_y),
              fminf(1.0, point[0] + delta_x),
              fminf(1.0, point[1] + delta_y)
            };
            dt_lib_colorpicker_set_box_area(darktable.lib, box);
          }
          dt_control_set_cursor(GDK_FLEUR);
        }
        else
        {
          /* Point pickers must not pre-write sample->point before going through the picker API,
             otherwise the setter sees no geometry change and skips the resample request. */
          dt_lib_colorpicker_set_point(darktable.lib, point);
        }
      }
      return 1;
    }

    if(which == 3)
    {
      // apply a live sample's area to the active picker?
      // FIXME: this is a naive implementation, nicer would be to cycle through overlapping samples then reset
      dt_iop_color_picker_t *picker = darktable.develop->color_picker.picker;
      if(darktable.develop->color_picker.display_samples && mouse_in_imagearea(self, x, y))
        for(GSList *samples = darktable.develop->color_picker.samples; samples; samples = g_slist_next(samples))
        {
          dt_colorpicker_sample_t *live_sample = samples->data;
          if(live_sample->size == DT_LIB_COLORPICKER_SIZE_BOX && picker->kind != DT_COLOR_PICKER_POINT)
          {
            if(point[0] < live_sample->box[0] || point[0] > live_sample->box[2]
               || point[1] < live_sample->box[1] || point[1] > live_sample->box[3])
              continue;
            dt_lib_colorpicker_set_box_area(darktable.lib, live_sample->box);
          }
          else if(live_sample->size == DT_LIB_COLORPICKER_SIZE_POINT && picker->kind != DT_COLOR_PICKER_AREA)
          {
            // magic values derived from _darkroom_pickers_draw
            float slop[2] = {
              MAX(26.0f, roundf(3.0f * zoom_scale)),
              MAX(26.0f, roundf(3.0f * zoom_scale))
            };
            dt_dev_coordinates_widget_delta_to_image_delta(dev, slop, 1);
            slop[0] /= dev->roi.processed_width;
            slop[1] /= dev->roi.processed_height;
            if(fabsf(point[0] - live_sample->point[0]) > slop[0]
               || fabsf(point[1] - live_sample->point[1]) > slop[1])
              continue;
            dt_lib_colorpicker_set_point(darktable.lib, live_sample->point);
          }
          else
            continue;
          return 1;
        }

      if(sample->size == DT_LIB_COLORPICKER_SIZE_BOX)
      {
        // default is hardcoded this way
        // FIXME: color_pixer_proxy should have an dt_iop_color_picker_clear_area() function for this
        dt_boundingbox_t reset = { 0.01f, 0.01f, 0.99f, 0.99f };
        dt_lib_colorpicker_set_box_area(darktable.lib, reset);
      }
      return 1;
    }
  }

  // masks
  if(dt_masks_get_visible_form(dev)
     && dt_masks_events_button_pressed(dev->gui_module, x, y, pressure, which, type, state))
  {
    dt_gui_throttle_queue(dev, _delayed_history_commit, dev);
    return 1;
  }
  // module
  if(dev->gui_module && dev->gui_module->enabled && dev->gui_module->button_pressed
     && dev->gui_module->button_pressed(dev->gui_module, x, y, pressure, which, type, state))
     return 1;

  if(which == 2)
  {
    // Incremental zoom-in on middle button click, from fit to 800% 
    // by power of 2 increments (100%, 200%, 400%, 800%).
    float new_scale = 1.f;
    if(dev->roi.scaling < 1.f || dev->roi.scaling > 7.f / dev->roi.natural_scale)
      new_scale = 1.f; // zoom to fit
    else if(dev->roi.scaling * dev->roi.natural_scale < 1.f)
      new_scale = 1.f / dev->roi.natural_scale; // 100 %
    else
      new_scale = floorf(dev->roi.scaling * dev->roi.natural_scale) * 2.f / dev->roi.natural_scale;

    const float point[2] = { x, y };
    return _change_scaling(dev, point, new_scale);
  }

  return 0;
}

static int _change_scaling(dt_develop_t *dev, const float point[2], const float new_scaling)
{
  const float old_scaling = dev->roi.scaling;

  // Round scaling to 1.0 (fit) if close enough
  const float epsilon = fabsf(old_scaling - new_scaling);
  if(fabsf(new_scaling - 1.0f) < epsilon)
    dev->roi.scaling = 1.0f;
  else
    dev->roi.scaling = new_scaling;

  if(!dt_dev_check_zoom_scale_bounds(dev))
  { 
    // Calculate zoom position offset to keep mouse position fixed during zoom
    float center[2] = { 0.0f };
    float mouse_offset[2] = { point[0], point[1] };
    dt_dev_get_widget_center(dev, center);
    mouse_offset[0] -= center[0];
    mouse_offset[1] -= center[1];

    
    // Keep the image point under the mouse fixed in widget coordinates while
    // the pipeline zoom stays DPI-invariant.
    const float old_zoom = dt_dev_get_widget_zoom_scale(dev, old_scaling);
    const float new_zoom = dt_dev_get_widget_zoom_scale(dev, dev->roi.scaling);
    if(old_zoom <= 1e-6f || new_zoom <= 1e-6f)
    {
      dev->roi.scaling = old_scaling;
      return 0;
    }

    // Adjust the center to compensate for the scale change
    int proc_w = 0.f, proc_h = 0.f;
    dt_dev_get_processed_size(dev, &proc_w, &proc_h);
    dev->roi.x += mouse_offset[0] * (1.f / old_zoom - 1.f / new_zoom) / proc_w;
    dev->roi.y += mouse_offset[1] * (1.f / old_zoom - 1.f / new_zoom) / proc_h;
    
    dt_dev_check_zoom_pos_bounds(dev, &dev->roi.x, &dev->roi.y, NULL, NULL);
    dt_dev_pixelpipe_change_zoom_main(dev);
    return 1;
  }
  else
  {
    // Invalid zoom level, keep previous value
    dev->roi.scaling = old_scaling;
    return 0;
  }
}

static gboolean _center_view_free_zoom(dt_view_t *self, double x, double y, int up, int state, int flow)
{
  dt_develop_t *dev = darktable.develop;

  // Commit the new scaling
  const float step = 1.02f;
  const float new_scaling = dev->roi.scaling * powf(step, (float)-flow);
  const float point[2] = { x, y };
  return _change_scaling(dev, point, new_scaling);
}


int scrolled(dt_view_t *self, double x, double y, int up, int state, int delta_y)
{
  if(_is_scroll_captured_by_widget()) return FALSE;

  dt_develop_t *dev = (dt_develop_t *)self->data;

  dt_print(DT_DEBUG_INPUT, "[darkroom] scrolled: up: %i x: %.2f y: %.2f state: %i flow: %i\n",
           up, x, y, state, delta_y);

  // masks
  if(dt_masks_get_visible_form(dev)
     && dt_masks_events_mouse_scrolled(dev->gui_module, x, y, up, state, delta_y))
  {
    // Scroll on masks changes their size, therefore mask parameters and image output.
    dt_gui_throttle_queue(dev, _delayed_history_commit, dev);
    return TRUE;
  }

  // module
  if(dev->gui_module && dev->gui_module->enabled && dev->gui_module->scrolled && dev->gui_module->scrolled(dev->gui_module, x, y, up, state))
  {
    // Scroll in modules should handle history changes internally.
    return TRUE;
  }

  // free zoom
  return _center_view_free_zoom(self, x, y, up, state, delta_y);
}

static void _key_scroll(dt_develop_t *dev)
{
  dt_dev_check_zoom_pos_bounds(dev, &dev->roi.x, &dev->roi.y, NULL, NULL);
  dt_control_queue_redraw_center();
  dt_dev_pixelpipe_change_zoom_main(dev);
}


int key_pressed(dt_view_t *self, GdkEventKey *event)
{
  if(!gtk_window_is_active(GTK_WINDOW(darktable.gui->ui->main_window))) return FALSE;

  dt_develop_t *dev = (dt_develop_t *)self->data;

  if(dt_masks_get_visible_form(dev) && dt_masks_events_key_pressed(dev->gui_module, event))
  {
    dt_gui_throttle_queue(dev, _delayed_history_commit, dev);
    return 1;
  }

  const gboolean shift = dt_modifier_is(event->state, GDK_SHIFT_MASK);
  const gboolean ctrl = dt_modifier_is(event->state, GDK_CONTROL_MASK);
  const gboolean ctrl_any = dt_modifiers_include(event->state, GDK_CONTROL_MASK);

  if(ctrl_any)
  {
    const float zoom_step = 1.1f;
    float center[2] = { 0.0f };
    dt_dev_get_widget_center(dev, center);

    switch(event->keyval)
    {
      case GDK_KEY_plus:
      case GDK_KEY_KP_Add:
        return _change_scaling(dev, center, dev->roi.scaling * zoom_step);
      case GDK_KEY_minus:
      case GDK_KEY_KP_Subtract:
        return _change_scaling(dev, center, dev->roi.scaling / zoom_step);
    }
  }

  float multiplier = (shift) ? 4.f :
                     (ctrl) ? 0.5f :
                     1.f;

  float delta[2] = { 10.f * multiplier, 10.f * multiplier };
  dt_dev_coordinates_widget_delta_to_image_delta(dev, delta, 1);

  switch(event->keyval)
  {
    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:
    {
      dev->roi.y -= delta[1] / (float)dev->roi.processed_height;
      _key_scroll(dev);
      return 1;
    }
    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:
    {
      dev->roi.y += delta[1] / (float)dev->roi.processed_height;
      _key_scroll(dev);
      return 1;
    }
    case GDK_KEY_Left:
    case GDK_KEY_KP_Left:
    {
      dev->roi.x -= delta[0] / (float)dev->roi.processed_width;
      _key_scroll(dev);
      return 1;
    }
    case GDK_KEY_Right:
    case GDK_KEY_KP_Right:
    {
      dev->roi.x += delta[0] / (float)dev->roi.processed_width;
      _key_scroll(dev);
      return 1;
    }
    case GDK_KEY_Escape:
    {
      dt_ctl_switch_mode_to("lighttable");
      return TRUE;
    }
  }

  return 0;
}

void configure(dt_view_t *self, int wd, int ht)
{
  dt_develop_t *dev = (dt_develop_t *)self->data;

  // Configure event is called when initing the view AND upon window resizes events (through Gtk widget/window resize commands).
  // At init time, final window size may not be correct just yet.
  // It will be when we call dt_ui_restore_panels(), which will resize stuff properly,
  // but that will be only when entering the current view.
  // Until we run dt_dev_configure(), main preview pipe gets output size -1×-1 px
  // which aborts the pipe recompute early. As soon as we init
  // sizes with something "valid" with regard to the pipe, pipeline runs.
  // Problem is it will not be valid with regard to the window size and the output will be thrown out
  // until we get the final size.
  // TD;DR: until we get the final window size, which happens
  // only when entering the view, don't configure the main preview pipeline, which will disable useless recomputes.
  if(dt_view_manager_get_current_view(darktable.view_manager) == self)
  {
    // Reference dimensions before ISO 12646 mode
    dev->roi.orig_height = ht;
    dev->roi.orig_width = wd;
    _get_final_size_with_iso_12646(dev);
  }
}

// clang-format off
// modelines: These editor modelines have been set for all relevant files by tools/update_modelines.py
// vim: shiftwidth=2 expandtab tabstop=2 cindent
// kate: tab-indents: off; indent-width 2; replace-tabs on; indent-mode cstyle; remove-trailing-spaces modified;
// clang-format on
