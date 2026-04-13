#include "common/agent_catalog.h"

#include "bauhaus/bauhaus.h"
#include "common/darktable.h"
#include "develop/develop.h"
#include "develop/imageop.h"

#include <glib/gi18n.h>
#include <math.h>
#include <string.h>

typedef enum dt_agent_catalog_error_t
{
  DT_AGENT_CATALOG_ERROR_INVALID = 1,
} dt_agent_catalog_error_t;

static GQuark _agent_catalog_error_quark(void)
{
  return g_quark_from_static_string("dt-agent-catalog-error");
}

static gchar *_sanitize_segment(const char *value)
{
  if(!value || !value[0])
    return g_strdup("unknown");

  gchar *sanitized = g_strdup(value);
  for(char *iter = sanitized; *iter; iter++)
  {
    if((*iter >= 'a' && *iter <= 'z') || (*iter >= 'A' && *iter <= 'Z')
       || (*iter >= '0' && *iter <= '9') || *iter == '_')
      continue;
    *iter = '_';
  }
  return sanitized;
}

static gchar *_field_leaf_name(const dt_introspection_field_t *field)
{
  return _sanitize_segment(field && field->header.field_name ? field->header.field_name : NULL);
}

static gchar *_module_id(const dt_iop_module_t *module)
{
  return g_strdup(module && module->op[0] ? module->op : "unknown");
}

static gchar *_module_label(const dt_iop_module_t *module)
{
  if(module && module->name)
  {
    const char *label = module->name();
    if(label && label[0])
      return g_strdup(label);
  }

  return _module_id(module);
}

static gchar *_build_action_path(const dt_iop_module_t *module, const dt_introspection_field_t *field)
{
  g_autofree gchar *leaf = _field_leaf_name(field);
  return g_strdup_printf("iop/%s/%s", module && module->op[0] ? module->op : "unknown", leaf);
}

static gchar *_build_setting_id(const dt_iop_module_t *module, const dt_introspection_field_t *field)
{
  g_autofree gchar *leaf = _field_leaf_name(field);
  if(module && module->multi_priority > 0)
    return g_strdup_printf("setting.%s.%s.instance.%d", module->op, leaf, module->multi_priority);
  return g_strdup_printf("setting.%s.%s", module && module->op[0] ? module->op : "unknown", leaf);
}

static gchar *_build_capability_id(const dt_iop_module_t *module, const dt_introspection_field_t *field)
{
  g_autofree gchar *leaf = _field_leaf_name(field);
  if(module && module->multi_priority > 0)
    return g_strdup_printf("%s.%s.instance.%d", module->op, leaf, module->multi_priority);
  return g_strdup_printf("%s.%s", module && module->op[0] ? module->op : "unknown", leaf);
}

static gboolean _is_numeric_type(const dt_introspection_type_t type)
{
  switch(type)
  {
    case DT_INTROSPECTION_TYPE_FLOAT:
    case DT_INTROSPECTION_TYPE_DOUBLE:
    case DT_INTROSPECTION_TYPE_CHAR:
    case DT_INTROSPECTION_TYPE_INT8:
    case DT_INTROSPECTION_TYPE_UINT8:
    case DT_INTROSPECTION_TYPE_SHORT:
    case DT_INTROSPECTION_TYPE_USHORT:
    case DT_INTROSPECTION_TYPE_INT:
    case DT_INTROSPECTION_TYPE_UINT:
      return TRUE;
    default:
      return FALSE;
  }
}

static gboolean _read_number_from_field(const dt_introspection_field_t *field,
                                        const void *data,
                                        double *out_number)
{
  if(!field || !data || !out_number)
    return FALSE;

  switch(field->header.type)
  {
    case DT_INTROSPECTION_TYPE_FLOAT:
      *out_number = *(const float *)data;
      return TRUE;
    case DT_INTROSPECTION_TYPE_DOUBLE:
      *out_number = *(const double *)data;
      return TRUE;
    case DT_INTROSPECTION_TYPE_CHAR:
      *out_number = *(const char *)data;
      return TRUE;
    case DT_INTROSPECTION_TYPE_INT8:
      *out_number = *(const int8_t *)data;
      return TRUE;
    case DT_INTROSPECTION_TYPE_UINT8:
      *out_number = *(const uint8_t *)data;
      return TRUE;
    case DT_INTROSPECTION_TYPE_SHORT:
      *out_number = *(const short *)data;
      return TRUE;
    case DT_INTROSPECTION_TYPE_USHORT:
      *out_number = *(const unsigned short *)data;
      return TRUE;
    case DT_INTROSPECTION_TYPE_INT:
      *out_number = *(const int *)data;
      return TRUE;
    case DT_INTROSPECTION_TYPE_UINT:
      *out_number = *(const unsigned int *)data;
      return TRUE;
    default:
      return FALSE;
  }
}

static gboolean _read_choice_value_from_field(const dt_introspection_field_t *field,
                                              const void *data,
                                              gint *out_choice_value)
{
  if(!field || !data || !out_choice_value)
    return FALSE;

  switch(field->header.type)
  {
    case DT_INTROSPECTION_TYPE_ENUM:
    case DT_INTROSPECTION_TYPE_INT:
      *out_choice_value = *(const int *)data;
      return TRUE;
    case DT_INTROSPECTION_TYPE_UINT:
      *out_choice_value = (gint)*(const unsigned int *)data;
      return TRUE;
    default:
      return FALSE;
  }
}

static gboolean _read_bool_from_field(const dt_introspection_field_t *field,
                                      const void *data,
                                      gboolean *out_bool)
{
  if(!field || !data || !out_bool || field->header.type != DT_INTROSPECTION_TYPE_BOOL)
    return FALSE;

  *out_bool = *(const gboolean *)data;
  return TRUE;
}

static dt_introspection_field_t *_find_field_for_widget(const dt_iop_module_t *module, GtkWidget *widget)
{
  if(!module || !widget || !module->params || !module->so || !module->so->get_introspection_linear)
    return NULL;

  const DtBauhausWidget *bhw = DT_BAUHAUS_WIDGET(widget);
  if(!bhw || !bhw->field)
    return NULL;

  const ptrdiff_t offset = (const guint8 *)bhw->field - (const guint8 *)module->params;
  if(offset < 0 || offset >= module->params_size)
    return NULL;

  for(dt_introspection_field_t *iter = module->so->get_introspection_linear();
      iter && iter->header.type != DT_INTROSPECTION_TYPE_NONE; iter++)
  {
    if((ptrdiff_t)iter->header.offset == offset)
      return iter;
  }

  return NULL;
}

static gboolean _choice_value_exists(const dt_agent_action_descriptor_t *descriptor, gint requested_choice_value)
{
  if(!descriptor || !descriptor->choices)
    return FALSE;

  for(guint i = 0; i < descriptor->choices->len; i++)
  {
    const dt_agent_choice_option_t *option = g_ptr_array_index(descriptor->choices, i);
    if(option && option->choice_value == requested_choice_value)
      return TRUE;
  }

  return FALSE;
}

static void _populate_choices(dt_agent_action_descriptor_t *descriptor)
{
  const DtBauhausWidget *bhw = DT_BAUHAUS_WIDGET(descriptor->widget);
  descriptor->choices = g_ptr_array_new_with_free_func(dt_agent_choice_option_free);

  if(descriptor->field && descriptor->field->header.type == DT_INTROSPECTION_TYPE_ENUM
     && descriptor->field->Enum.values)
  {
    for(dt_introspection_type_enum_tuple_t *value = descriptor->field->Enum.values; value->name; value++)
    {
      dt_agent_choice_option_t *option = g_new0(dt_agent_choice_option_t, 1);
      option->choice_value = value->value;
      option->choice_id = g_strdup(value->name);
      option->label = g_strdup(value->description && value->description[0] ? value->description : value->name);
      g_ptr_array_add(descriptor->choices, option);
    }
    descriptor->has_default_choice_value = TRUE;
    descriptor->default_choice_value = descriptor->field->Enum.Default;
    return;
  }

  const dt_bauhaus_combobox_data_t *combobox = &bhw->data.combobox;
  for(guint i = 0; combobox->entries && i < combobox->entries->len; i++)
  {
    const dt_bauhaus_combobox_entry_t *entry = g_ptr_array_index(combobox->entries, i);
    dt_agent_choice_option_t *option = g_new0(dt_agent_choice_option_t, 1);
    option->choice_value = (gint)i;
    option->choice_id = g_strdup_printf("choice.%u", i);
    option->label = g_strdup(entry && entry->label ? entry->label : "");
    g_ptr_array_add(descriptor->choices, option);
  }

  descriptor->has_default_choice_value = TRUE;
  descriptor->default_choice_value = combobox->defpos;
}

static dt_agent_action_descriptor_t *_build_descriptor(dt_iop_module_t *module, GtkWidget *widget)
{
  if(!module || !widget || !DT_IS_BAUHAUS_WIDGET(widget))
    return NULL;

  const DtBauhausWidget *bhw = DT_BAUHAUS_WIDGET(widget);
  if(!bhw->field)
    return NULL;

  dt_introspection_field_t *field = _find_field_for_widget(module, widget);
  if(!field)
    return NULL;

  dt_agent_action_descriptor_t *descriptor = g_new0(dt_agent_action_descriptor_t, 1);
  descriptor->module = module;
  descriptor->widget = widget;
  descriptor->field = field;
  descriptor->module_id = _module_id(module);
  descriptor->module_label = _module_label(module);
  descriptor->capability_id = _build_capability_id(module, field);
  descriptor->setting_id = _build_setting_id(module, field);
  descriptor->action_path = _build_action_path(module, field);
  descriptor->target_type = g_strdup(DT_AGENT_TARGET_TYPE);
  descriptor->label = g_strdup(bhw->label[0] ? bhw->label : field->header.field_name);

  if(bhw->type == DT_BAUHAUS_SLIDER && _is_numeric_type(field->header.type))
  {
    const dt_bauhaus_slider_data_t *slider = &bhw->data.slider;
    descriptor->operation_kind = DT_AGENT_OPERATION_SET_FLOAT;
    descriptor->kind_name = g_strdup(dt_agent_operation_kind_to_string(descriptor->operation_kind));
    descriptor->supported_modes = DT_AGENT_VALUE_MODE_FLAG_SET | DT_AGENT_VALUE_MODE_FLAG_DELTA;
    descriptor->has_number_range = TRUE;
    descriptor->min_number = slider->hard_min;
    descriptor->max_number = slider->hard_max;
    descriptor->default_number = slider->defpos;
    descriptor->step_number = dt_bauhaus_slider_get_step(widget);
    return descriptor;
  }

  if(bhw->type == DT_BAUHAUS_COMBOBOX && field->header.type == DT_INTROSPECTION_TYPE_BOOL)
  {
    descriptor->operation_kind = DT_AGENT_OPERATION_SET_BOOL;
    descriptor->kind_name = g_strdup(dt_agent_operation_kind_to_string(descriptor->operation_kind));
    descriptor->supported_modes = DT_AGENT_VALUE_MODE_FLAG_SET;
    descriptor->has_default_bool = TRUE;
    descriptor->default_bool = field->Bool.Default;
    return descriptor;
  }

  if(bhw->type == DT_BAUHAUS_COMBOBOX
     && (field->header.type == DT_INTROSPECTION_TYPE_ENUM
         || field->header.type == DT_INTROSPECTION_TYPE_INT
         || field->header.type == DT_INTROSPECTION_TYPE_UINT))
  {
    descriptor->operation_kind = DT_AGENT_OPERATION_SET_CHOICE;
    descriptor->kind_name = g_strdup(dt_agent_operation_kind_to_string(descriptor->operation_kind));
    descriptor->supported_modes = DT_AGENT_VALUE_MODE_FLAG_SET;
    _populate_choices(descriptor);
    return descriptor;
  }

  dt_agent_action_descriptor_free(descriptor);
  return NULL;
}

void dt_agent_action_descriptor_free(gpointer data)
{
  dt_agent_action_descriptor_t *descriptor = data;
  if(!descriptor)
    return;

  g_free(descriptor->module_id);
  g_free(descriptor->module_label);
  g_free(descriptor->capability_id);
  g_free(descriptor->setting_id);
  g_free(descriptor->label);
  g_free(descriptor->kind_name);
  g_free(descriptor->target_type);
  g_free(descriptor->action_path);
  if(descriptor->choices)
    g_ptr_array_unref(descriptor->choices);
  g_free(descriptor);
}

dt_agent_action_descriptor_t *dt_agent_action_descriptor_copy(const dt_agent_action_descriptor_t *src)
{
  if(!src)
    return NULL;

  dt_agent_action_descriptor_t *dest = g_new0(dt_agent_action_descriptor_t, 1);
  *dest = *src;
  dest->module_id = g_strdup(src->module_id);
  dest->module_label = g_strdup(src->module_label);
  dest->capability_id = g_strdup(src->capability_id);
  dest->setting_id = g_strdup(src->setting_id);
  dest->label = g_strdup(src->label);
  dest->kind_name = g_strdup(src->kind_name);
  dest->target_type = g_strdup(src->target_type);
  dest->action_path = g_strdup(src->action_path);
  if(src->choices)
  {
    dest->choices = g_ptr_array_new_with_free_func(dt_agent_choice_option_free);
    for(guint i = 0; i < src->choices->len; i++)
    {
      dt_agent_choice_option_t *option = g_ptr_array_index(src->choices, i);
      g_ptr_array_add(dest->choices, dt_agent_choice_option_copy(option));
    }
  }
  return dest;
}

gboolean dt_agent_catalog_is_action_path_allowed(const char *action_path)
{
  return action_path && g_str_has_prefix(action_path, "iop/") && action_path[4] != '\0';
}

gboolean dt_agent_catalog_collect_descriptors(const struct dt_develop_t *dev,
                                              GPtrArray *descriptors,
                                              GError **error)
{
  if(!dev || !descriptors)
  {
    g_set_error(error, _agent_catalog_error_quark(), DT_AGENT_CATALOG_ERROR_INVALID,
                "%s", _("missing agent catalog state"));
    return FALSE;
  }

  g_ptr_array_set_size(descriptors, 0);

  for(const GList *module_iter = dev->iop; module_iter; module_iter = g_list_next(module_iter))
  {
    dt_iop_module_t *module = module_iter->data;
    GList *widget_list = module ? DT_GUI_MODULE(module)->widget_list : NULL;
    if(!module || !widget_list)
      continue;

    for(const GList *widget_iter = widget_list; widget_iter; widget_iter = g_list_next(widget_iter))
    {
      GtkWidget *widget = widget_iter->data;
      dt_agent_action_descriptor_t *descriptor = _build_descriptor(module, widget);
      if(descriptor)
        g_ptr_array_add(descriptors, descriptor);
    }
  }

  return TRUE;
}

dt_agent_action_descriptor_t *dt_agent_catalog_find_descriptor(const struct dt_develop_t *dev,
                                                               const char *action_path,
                                                               const char *setting_id,
                                                               GError **error)
{
  if(!dev || !action_path || !setting_id)
  {
    g_set_error(error, _agent_catalog_error_quark(), DT_AGENT_CATALOG_ERROR_INVALID,
                "%s", _("missing agent descriptor lookup inputs"));
    return NULL;
  }

  g_autoptr(GPtrArray) descriptors = g_ptr_array_new_with_free_func(dt_agent_action_descriptor_free);
  if(!dt_agent_catalog_collect_descriptors(dev, descriptors, error))
    return NULL;

  for(guint i = 0; i < descriptors->len; i++)
  {
    const dt_agent_action_descriptor_t *descriptor = g_ptr_array_index(descriptors, i);
    if(g_strcmp0(descriptor->action_path, action_path) == 0
       && g_strcmp0(descriptor->setting_id, setting_id) == 0)
      return dt_agent_action_descriptor_copy(descriptor);
  }

  g_set_error(error, _agent_catalog_error_quark(), DT_AGENT_CATALOG_ERROR_INVALID,
              _("unsupported action path: %s"), action_path);
  return NULL;
}

gboolean dt_agent_catalog_supports_mode(const dt_agent_action_descriptor_t *descriptor,
                                        dt_agent_value_mode_t mode)
{
  if(!descriptor)
    return FALSE;

  switch(mode)
  {
    case DT_AGENT_VALUE_MODE_SET:
      return (descriptor->supported_modes & DT_AGENT_VALUE_MODE_FLAG_SET) != 0;
    case DT_AGENT_VALUE_MODE_DELTA:
      return (descriptor->supported_modes & DT_AGENT_VALUE_MODE_FLAG_DELTA) != 0;
    case DT_AGENT_VALUE_MODE_UNKNOWN:
    default:
      return FALSE;
  }
}

double dt_agent_catalog_clamp_number(const dt_agent_action_descriptor_t *descriptor,
                                     double requested_number)
{
  if(!descriptor || !descriptor->has_number_range)
    return requested_number;

  return CLAMP(requested_number, descriptor->min_number, descriptor->max_number);
}

gboolean dt_agent_catalog_read_current_number(const dt_agent_action_descriptor_t *descriptor,
                                              double *out_number,
                                              GError **error)
{
  if(!descriptor || !descriptor->module || !descriptor->field || !out_number)
  {
    g_set_error(error, _agent_catalog_error_quark(), DT_AGENT_CATALOG_ERROR_INVALID,
                "%s", _("missing numeric descriptor"));
    return FALSE;
  }

  const guint8 *data = (const guint8 *)descriptor->module->params + descriptor->field->header.offset;
  if(_read_number_from_field(descriptor->field, data, out_number))
    return TRUE;

  g_set_error(error, _agent_catalog_error_quark(), DT_AGENT_CATALOG_ERROR_INVALID,
              "%s", _("unsupported numeric control"));
  return FALSE;
}

gboolean dt_agent_catalog_read_current_choice(const dt_agent_action_descriptor_t *descriptor,
                                              gint *out_choice_value,
                                              gchar **out_choice_id,
                                              GError **error)
{
  if(!descriptor || !descriptor->module || !descriptor->field || !out_choice_value)
  {
    g_set_error(error, _agent_catalog_error_quark(), DT_AGENT_CATALOG_ERROR_INVALID,
                "%s", _("missing choice descriptor"));
    return FALSE;
  }

  const guint8 *data = (const guint8 *)descriptor->module->params + descriptor->field->header.offset;
  if(!_read_choice_value_from_field(descriptor->field, data, out_choice_value))
  {
    g_set_error(error, _agent_catalog_error_quark(), DT_AGENT_CATALOG_ERROR_INVALID,
                "%s", _("unsupported choice control"));
    return FALSE;
  }

  if(out_choice_id)
  {
    if(descriptor->field->header.type == DT_INTROSPECTION_TYPE_ENUM)
      *out_choice_id = g_strdup(dt_introspection_get_enum_name(descriptor->field, *out_choice_value));
    else
      *out_choice_id = g_strdup_printf("choice.%d", *out_choice_value);
  }

  return TRUE;
}

gboolean dt_agent_catalog_read_current_bool(const dt_agent_action_descriptor_t *descriptor,
                                            gboolean *out_bool_value,
                                            GError **error)
{
  if(!descriptor || !descriptor->module || !descriptor->field || !out_bool_value)
  {
    g_set_error(error, _agent_catalog_error_quark(), DT_AGENT_CATALOG_ERROR_INVALID,
                "%s", _("missing bool descriptor"));
    return FALSE;
  }

  const guint8 *data = (const guint8 *)descriptor->module->params + descriptor->field->header.offset;
  if(_read_bool_from_field(descriptor->field, data, out_bool_value))
    return TRUE;

  g_set_error(error, _agent_catalog_error_quark(), DT_AGENT_CATALOG_ERROR_INVALID,
              "%s", _("unsupported bool control"));
  return FALSE;
}

gboolean dt_agent_catalog_write_number(const dt_agent_action_descriptor_t *descriptor,
                                       double requested_number,
                                       double *out_applied_number,
                                       GError **error)
{
  if(!descriptor || !descriptor->widget || !DT_IS_BAUHAUS_WIDGET(descriptor->widget)
     || DT_BAUHAUS_WIDGET(descriptor->widget)->type != DT_BAUHAUS_SLIDER)
  {
    g_set_error(error, _agent_catalog_error_quark(), DT_AGENT_CATALOG_ERROR_INVALID,
                "%s", _("missing numeric widget"));
    return FALSE;
  }

  dt_bauhaus_slider_set(descriptor->widget, dt_agent_catalog_clamp_number(descriptor, requested_number));
  if(out_applied_number)
    return dt_agent_catalog_read_current_number(descriptor, out_applied_number, error);
  return TRUE;
}

gboolean dt_agent_catalog_write_choice(const dt_agent_action_descriptor_t *descriptor,
                                       gint requested_choice_value,
                                       gint *out_applied_choice_value,
                                       GError **error)
{
  if(!descriptor || !descriptor->widget || !DT_IS_BAUHAUS_WIDGET(descriptor->widget)
     || DT_BAUHAUS_WIDGET(descriptor->widget)->type != DT_BAUHAUS_COMBOBOX)
  {
    g_set_error(error, _agent_catalog_error_quark(), DT_AGENT_CATALOG_ERROR_INVALID,
                "%s", _("missing choice widget"));
    return FALSE;
  }

  if(!_choice_value_exists(descriptor, requested_choice_value))
  {
    g_set_error(error, _agent_catalog_error_quark(), DT_AGENT_CATALOG_ERROR_INVALID,
                _("unsupported choice value for action path: %s"), descriptor->action_path);
    return FALSE;
  }

  if(descriptor->field && descriptor->field->header.type == DT_INTROSPECTION_TYPE_ENUM)
  {
    if(!dt_bauhaus_combobox_set_from_value(descriptor->widget, requested_choice_value))
    {
      g_set_error(error, _agent_catalog_error_quark(), DT_AGENT_CATALOG_ERROR_INVALID,
                  _("unsupported enum choice for action path: %s"), descriptor->action_path);
      return FALSE;
    }
  }
  else
    dt_bauhaus_combobox_set(descriptor->widget, requested_choice_value);

  if(out_applied_choice_value)
    return dt_agent_catalog_read_current_choice(descriptor, out_applied_choice_value, NULL, error);
  return TRUE;
}

gboolean dt_agent_catalog_write_bool(const dt_agent_action_descriptor_t *descriptor,
                                     gboolean requested_bool_value,
                                     gboolean *out_applied_bool_value,
                                     GError **error)
{
  if(!descriptor || !descriptor->widget || !DT_IS_BAUHAUS_WIDGET(descriptor->widget)
     || DT_BAUHAUS_WIDGET(descriptor->widget)->type != DT_BAUHAUS_COMBOBOX)
  {
    g_set_error(error, _agent_catalog_error_quark(), DT_AGENT_CATALOG_ERROR_INVALID,
                "%s", _("missing bool widget"));
    return FALSE;
  }

  dt_bauhaus_combobox_set(descriptor->widget, requested_bool_value ? 1 : 0);
  if(out_applied_bool_value)
    return dt_agent_catalog_read_current_bool(descriptor, out_applied_bool_value, error);
  return TRUE;
}
