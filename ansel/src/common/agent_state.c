#include "common/agent_state.h"

#include "common/agent_catalog.h"
#include "develop/develop.h"
#include "develop/dev_history.h"

#include <glib/gi18n.h>
#include <string.h>

typedef enum dt_agent_state_error_t
{
  DT_AGENT_STATE_ERROR_INVALID = 1,
} dt_agent_state_error_t;

static GQuark _agent_state_error_quark(void)
{
  return g_quark_from_static_string("dt-agent-state-error");
}

static void _agent_image_control_free(gpointer data)
{
  dt_agent_image_control_t *control = data;
  if(!control)
    return;

  g_free(control->module_id);
  g_free(control->module_label);
  g_free(control->setting_id);
  g_free(control->capability_id);
  g_free(control->label);
  g_free(control->kind);
  g_free(control->action_path);
  g_free(control->current_choice_id);
  if(control->choices)
    g_ptr_array_unref(control->choices);
  g_free(control);
}

static dt_agent_image_control_t *_agent_image_control_copy(const dt_agent_image_control_t *src)
{
  dt_agent_image_control_t *dest = g_new0(dt_agent_image_control_t, 1);
  dest->module_id = g_strdup(src->module_id);
  dest->module_label = g_strdup(src->module_label);
  dest->setting_id = g_strdup(src->setting_id);
  dest->capability_id = g_strdup(src->capability_id);
  dest->label = g_strdup(src->label);
  dest->kind = g_strdup(src->kind);
  dest->action_path = g_strdup(src->action_path);
  dest->supported_modes = src->supported_modes;
  dest->has_current_number = src->has_current_number;
  dest->current_number = src->current_number;
  if(src->choices)
  {
    dest->choices = g_ptr_array_new_with_free_func(dt_agent_choice_option_free);
    for(guint i = 0; i < src->choices->len; i++)
    {
      dt_agent_choice_option_t *option = g_ptr_array_index(src->choices, i);
      g_ptr_array_add(dest->choices, dt_agent_choice_option_copy(option));
    }
  }
  dest->has_default_choice_value = src->has_default_choice_value;
  dest->default_choice_value = src->default_choice_value;
  dest->has_current_choice_value = src->has_current_choice_value;
  dest->current_choice_value = src->current_choice_value;
  dest->current_choice_id = g_strdup(src->current_choice_id);
  dest->has_default_bool = src->has_default_bool;
  dest->default_bool = src->default_bool;
  dest->has_current_bool = src->has_current_bool;
  dest->current_bool = src->current_bool;
  dest->min_number = src->min_number;
  dest->max_number = src->max_number;
  dest->default_number = src->default_number;
  dest->step_number = src->step_number;
  return dest;
}

static void _agent_history_item_free(gpointer data)
{
  dt_agent_history_item_t *item = data;
  if(!item)
    return;

  g_free(item->module);
  g_free(item->instance_name);
  g_free(item);
}

static dt_agent_history_item_t *_agent_history_item_copy(const dt_agent_history_item_t *src)
{
  dt_agent_history_item_t *dest = g_new0(dt_agent_history_item_t, 1);
  dest->num = src->num;
  dest->module = g_strdup(src->module);
  dest->enabled = src->enabled;
  dest->multi_priority = src->multi_priority;
  dest->instance_name = g_strdup(src->instance_name);
  dest->iop_order = src->iop_order;
  return dest;
}

void dt_agent_image_metadata_clear(dt_agent_image_metadata_t *metadata)
{
  if(!metadata)
    return;

  g_free(metadata->image_name);
  g_free(metadata->camera_maker);
  g_free(metadata->camera_model);
  memset(metadata, 0, sizeof(*metadata));
}

void dt_agent_image_state_init(dt_agent_image_state_t *state)
{
  if(!state)
    return;

  memset(state, 0, sizeof(*state));
  state->controls = g_ptr_array_new_with_free_func(_agent_image_control_free);
  state->history = g_ptr_array_new_with_free_func(_agent_history_item_free);
}

void dt_agent_image_state_clear(dt_agent_image_state_t *state)
{
  if(!state)
    return;

  dt_agent_image_metadata_clear(&state->metadata);
  if(state->controls)
    g_ptr_array_unref(state->controls);
  if(state->history)
    g_ptr_array_unref(state->history);
  g_free(state->preview.preview_id);
  g_free(state->preview.mime_type);
  g_free(state->preview.base64_data);
  memset(state, 0, sizeof(*state));
}

void dt_agent_image_state_copy(dt_agent_image_state_t *dest,
                               const dt_agent_image_state_t *src)
{
  dt_agent_image_state_init(dest);
  dest->history_position = src->history_position;
  dest->history_count = src->history_count;
  dest->metadata.has_image_id = src->metadata.has_image_id;
  dest->metadata.image_id = src->metadata.image_id;
  dest->metadata.image_name = g_strdup(src->metadata.image_name);
  dest->metadata.camera_maker = g_strdup(src->metadata.camera_maker);
  dest->metadata.camera_model = g_strdup(src->metadata.camera_model);
  dest->metadata.width = src->metadata.width;
  dest->metadata.height = src->metadata.height;
  dest->metadata.exif_exposure_seconds = src->metadata.exif_exposure_seconds;
  dest->metadata.exif_aperture = src->metadata.exif_aperture;
  dest->metadata.exif_iso = src->metadata.exif_iso;
  dest->metadata.exif_focal_length = src->metadata.exif_focal_length;

  for(guint i = 0; i < src->controls->len; i++)
  {
    const dt_agent_image_control_t *control = g_ptr_array_index(src->controls, i);
    g_ptr_array_add(dest->controls, _agent_image_control_copy(control));
  }

  for(guint i = 0; i < src->history->len; i++)
  {
    const dt_agent_history_item_t *item = g_ptr_array_index(src->history, i);
    g_ptr_array_add(dest->history, _agent_history_item_copy(item));
  }

  dest->preview.available = src->preview.available;
  dest->preview.preview_id = g_strdup(src->preview.preview_id);
  dest->preview.mime_type = g_strdup(src->preview.mime_type);
  dest->preview.width = src->preview.width;
  dest->preview.height = src->preview.height;
  dest->preview.base64_data = g_strdup(src->preview.base64_data);
  dest->histogram.available = src->histogram.available;
  dest->histogram.bin_count = src->histogram.bin_count;
  memcpy(dest->histogram.red, src->histogram.red, sizeof(src->histogram.red));
  memcpy(dest->histogram.green, src->histogram.green, sizeof(src->histogram.green));
  memcpy(dest->histogram.blue, src->histogram.blue, sizeof(src->histogram.blue));
  memcpy(dest->histogram.luma, src->histogram.luma, sizeof(src->histogram.luma));
}

static void _collect_metadata(const dt_develop_t *dev, dt_agent_image_state_t *state)
{
  const dt_image_t *image = &dev->image_storage;
  state->metadata.has_image_id = image->id > 0;
  state->metadata.image_id = image->id;
  state->metadata.image_name = image->filename[0] ? g_strdup(image->filename) : NULL;
  state->metadata.camera_maker = image->camera_maker[0] ? g_strdup(image->camera_maker) : NULL;
  state->metadata.camera_model = image->camera_model[0] ? g_strdup(image->camera_model) : NULL;
  state->metadata.width = image->width;
  state->metadata.height = image->height;
  state->metadata.exif_exposure_seconds = image->exif_exposure;
  state->metadata.exif_aperture = image->exif_aperture;
  state->metadata.exif_iso = image->exif_iso;
  state->metadata.exif_focal_length = image->exif_focal_length;
}

static void _copy_choices(dt_agent_image_control_t *control,
                          const dt_agent_action_descriptor_t *descriptor)
{
  if(!descriptor->choices)
    return;

  control->choices = g_ptr_array_new_with_free_func(dt_agent_choice_option_free);
  for(guint i = 0; i < descriptor->choices->len; i++)
  {
    dt_agent_choice_option_t *option = g_ptr_array_index(descriptor->choices, i);
    g_ptr_array_add(control->choices, dt_agent_choice_option_copy(option));
  }
}

static void _collect_controls(const dt_develop_t *dev, dt_agent_image_state_t *state)
{
  g_autoptr(GPtrArray) descriptors = g_ptr_array_new_with_free_func(dt_agent_action_descriptor_free);
  if(!dt_agent_catalog_collect_descriptors(dev, descriptors, NULL))
    return;

  for(guint i = 0; i < descriptors->len; i++)
  {
    const dt_agent_action_descriptor_t *descriptor = g_ptr_array_index(descriptors, i);
    dt_agent_image_control_t *control = g_new0(dt_agent_image_control_t, 1);
    control->module_id = g_strdup(descriptor->module_id);
    control->module_label = g_strdup(descriptor->module_label);
    control->setting_id = g_strdup(descriptor->setting_id);
    control->capability_id = g_strdup(descriptor->capability_id);
    control->label = g_strdup(descriptor->label);
    control->kind = g_strdup(descriptor->kind_name);
    control->action_path = g_strdup(descriptor->action_path);
    control->supported_modes = descriptor->supported_modes;
    control->min_number = descriptor->min_number;
    control->max_number = descriptor->max_number;
    control->default_number = descriptor->default_number;
    control->step_number = descriptor->step_number;
    control->has_default_choice_value = descriptor->has_default_choice_value;
    control->default_choice_value = descriptor->default_choice_value;
    control->has_default_bool = descriptor->has_default_bool;
    control->default_bool = descriptor->default_bool;
    _copy_choices(control, descriptor);

    switch(descriptor->operation_kind)
    {
      case DT_AGENT_OPERATION_SET_FLOAT:
        control->has_current_number = dt_agent_catalog_read_current_number(descriptor, &control->current_number, NULL);
        break;
      case DT_AGENT_OPERATION_SET_CHOICE:
        control->has_current_choice_value = dt_agent_catalog_read_current_choice(
          descriptor, &control->current_choice_value, &control->current_choice_id, NULL);
        break;
      case DT_AGENT_OPERATION_SET_BOOL:
        control->has_current_bool = dt_agent_catalog_read_current_bool(descriptor, &control->current_bool, NULL);
        break;
      case DT_AGENT_OPERATION_UNKNOWN:
      default:
        break;
    }

    g_ptr_array_add(state->controls, control);
  }
}

static void _collect_history(const dt_develop_t *dev, dt_agent_image_state_t *state)
{
  state->history_position = dev->history_end;
  state->history_count = g_list_length(dev->history);

  gint index = 0;
  for(const GList *iter = dev->history; iter && index < dev->history_end; iter = g_list_next(iter), index++)
  {
    const dt_dev_history_item_t *history_item = iter->data;
    if(!history_item)
      continue;

    dt_agent_history_item_t *item = g_new0(dt_agent_history_item_t, 1);
    item->num = history_item->num;
    item->module = history_item->op_name[0] ? g_strdup(history_item->op_name) : NULL;
    item->enabled = history_item->enabled;
    item->multi_priority = history_item->multi_priority;
    item->instance_name = history_item->multi_name[0] ? g_strdup(history_item->multi_name) : NULL;
    item->iop_order = history_item->iop_order;
    g_ptr_array_add(state->history, item);
  }
}

gboolean dt_agent_image_state_collect_from_dev(const dt_develop_t *dev,
                                               dt_agent_image_state_t *state,
                                               GError **error)
{
  if(!dev || !state)
  {
    g_set_error(error, _agent_state_error_quark(), DT_AGENT_STATE_ERROR_INVALID,
                "%s", _("missing darkroom state"));
    return FALSE;
  }

  dt_agent_image_state_clear(state);
  dt_agent_image_state_init(state);
  _collect_metadata(dev, state);
  _collect_controls(dev, state);
  _collect_history(dev, state);
  return TRUE;
}
