#include "common/agent_execute.h"

#include "common/agent_catalog.h"
#include "common/darktable.h"
#include "develop/develop.h"
#include "views/view.h"

#include <glib/gi18n.h>
#include <stdarg.h>
#include <string.h>

typedef enum dt_agent_execute_error_t
{
  DT_AGENT_EXECUTE_ERROR_INVALID = 1,
} dt_agent_execute_error_t;

typedef struct dt_agent_ordered_operation_t
{
  const dt_agent_chat_operation_t *operation;
  guint original_index;
} dt_agent_ordered_operation_t;

static GQuark _agent_execute_error_quark(void)
{
  return g_quark_from_static_string("dt-agent-execute-error");
}

static void _execution_result_free(gpointer data)
{
  dt_agent_execution_result_t *result = data;
  if(!result)
    return;

  g_free(result->operation_id);
  g_free(result->action_path);
  g_free(result->message);
  g_free(result);
}

void dt_agent_execution_report_init(dt_agent_execution_report_t *report)
{
  if(!report)
    return;

  memset(report, 0, sizeof(*report));
  report->results = g_ptr_array_new_with_free_func(_execution_result_free);
}

void dt_agent_execution_report_clear(dt_agent_execution_report_t *report)
{
  if(!report)
    return;

  if(report->results)
    g_ptr_array_unref(report->results);
  memset(report, 0, sizeof(*report));
}

const char *dt_agent_execution_status_to_string(dt_agent_execution_status_t status)
{
  switch(status)
  {
    case DT_AGENT_EXECUTION_STATUS_APPLIED:
      return "applied";
    case DT_AGENT_EXECUTION_STATUS_BLOCKED:
      return "blocked";
    case DT_AGENT_EXECUTION_STATUS_FAILED:
      return "failed";
    case DT_AGENT_EXECUTION_STATUS_UNKNOWN:
    default:
      return "unknown";
  }
}

static dt_agent_execution_result_t *_execution_result_new(const dt_agent_chat_operation_t *operation)
{
  dt_agent_execution_result_t *result = g_new0(dt_agent_execution_result_t, 1);
  result->operation_id = g_strdup(operation ? operation->operation_id : NULL);
  result->action_path = g_strdup(operation ? operation->action_path : NULL);
  return result;
}

static gboolean _execution_result_set_message(dt_agent_execution_result_t *result,
                                              const char *format,
                                              ...)
{
  va_list ap;
  va_start(ap, format);
  g_free(result->message);
  result->message = g_strdup_vprintf(format, ap);
  va_end(ap);
  return FALSE;
}

static gboolean _execution_result_set_blocked(dt_agent_execution_report_t *report,
                                              dt_agent_execution_result_t *result,
                                              GError **error,
                                              const char *format,
                                              ...)
{
  va_list ap;
  va_start(ap, format);
  g_free(result->message);
  result->message = g_strdup_vprintf(format, ap);
  va_end(ap);

  result->status = DT_AGENT_EXECUTION_STATUS_BLOCKED;
  report->blocked_count++;
  g_set_error(error, _agent_execute_error_quark(), DT_AGENT_EXECUTE_ERROR_INVALID,
              "%s", result->message ? result->message : _("operation blocked"));
  return FALSE;
}

static gboolean _execution_result_set_failed(dt_agent_execution_report_t *report,
                                             dt_agent_execution_result_t *result,
                                             GError **error)
{
  result->status = DT_AGENT_EXECUTION_STATUS_FAILED;
  g_free(result->message);
  result->message = g_strdup(error && *error && (*error)->message ? (*error)->message : _("operation failed"));
  report->failed_count++;
  return FALSE;
}

static gboolean _is_white_balance_action_path(const char *action_path)
{
  return action_path && g_str_has_prefix(action_path, "iop/temperature/");
}

static gint _white_balance_operation_rank(const dt_agent_chat_operation_t *operation)
{
  if(!operation || !_is_white_balance_action_path(operation->action_path))
    return G_MAXINT;

  const char *leaf = strrchr(operation->action_path, '/');
  leaf = leaf ? leaf + 1 : operation->action_path;

  switch(operation->kind)
  {
    case DT_AGENT_OPERATION_SET_BOOL:
      return 0;
    case DT_AGENT_OPERATION_SET_CHOICE:
      return 1;
    case DT_AGENT_OPERATION_SET_FLOAT:
      if(g_strcmp0(leaf, "finetune") == 0)
        return 2;
      if(g_strcmp0(leaf, "temperature") == 0)
        return 3;
      if(g_strcmp0(leaf, "tint") == 0)
        return 4;
      if(g_strcmp0(leaf, "red") == 0)
        return 5;
      if(g_strcmp0(leaf, "green") == 0)
        return 6;
      if(g_strcmp0(leaf, "blue") == 0)
        return 7;
      if(g_strcmp0(leaf, "emerald") == 0)
        return 8;
      if(g_strcmp0(leaf, "yellow") == 0 || g_strcmp0(leaf, "various") == 0)
        return 9;
      return 10;
    case DT_AGENT_OPERATION_UNKNOWN:
    default:
      return 11;
  }
}

static gint _ordered_white_balance_operation_compare(gconstpointer a, gconstpointer b)
{
  const dt_agent_ordered_operation_t *left = a;
  const dt_agent_ordered_operation_t *right = b;
  const gint left_rank = _white_balance_operation_rank(left->operation);
  const gint right_rank = _white_balance_operation_rank(right->operation);

  if(left_rank != right_rank)
    return left_rank < right_rank ? -1 : 1;
  if(left->original_index != right->original_index)
    return left->original_index < right->original_index ? -1 : 1;
  return 0;
}

static GPtrArray *_ordered_operations_for_execution(const GPtrArray *operations)
{
  GPtrArray *ordered = g_ptr_array_new();
  if(!operations)
    return ordered;

  g_ptr_array_set_size(ordered, operations->len);
  for(guint i = 0; i < operations->len; i++)
    ordered->pdata[i] = g_ptr_array_index((GPtrArray *)operations, i);

  g_autoptr(GArray) wb_positions = g_array_new(FALSE, FALSE, sizeof(guint));
  g_autoptr(GArray) wb_operations = g_array_new(FALSE, FALSE, sizeof(dt_agent_ordered_operation_t));
  for(guint i = 0; i < operations->len; i++)
  {
    const dt_agent_chat_operation_t *operation = g_ptr_array_index((GPtrArray *)operations, i);
    if(!_is_white_balance_action_path(operation ? operation->action_path : NULL))
      continue;

    g_array_append_val(wb_positions, i);
    dt_agent_ordered_operation_t item = { .operation = operation, .original_index = i };
    g_array_append_val(wb_operations, item);
  }

  if(wb_operations->len < 2)
    return ordered;

  g_array_sort(wb_operations, _ordered_white_balance_operation_compare);
  for(guint i = 0; i < wb_positions->len; i++)
  {
    const guint position = g_array_index(wb_positions, guint, i);
    const dt_agent_ordered_operation_t item = g_array_index(wb_operations, dt_agent_ordered_operation_t, i);
    ordered->pdata[position] = (gpointer)item.operation;
  }

  return ordered;
}

static const dt_agent_choice_option_t *_choice_option_for_value(const dt_agent_action_descriptor_t *descriptor,
                                                                gint requested_choice_value)
{
  if(!descriptor || !descriptor->choices)
    return NULL;

  for(guint i = 0; i < descriptor->choices->len; i++)
  {
    const dt_agent_choice_option_t *option = g_ptr_array_index(descriptor->choices, i);
    if(option && option->choice_value == requested_choice_value)
      return option;
  }

  return NULL;
}

static gboolean _validate_white_balance_operation(const dt_agent_chat_operation_t *operation,
                                                  const dt_agent_action_descriptor_t *descriptor,
                                                  GError **error)
{
  if(!operation || !descriptor || !_is_white_balance_action_path(descriptor->action_path))
    return TRUE;

  if(!darktable.develop || !darktable.develop->proxy.chroma_adaptation)
    return TRUE;

  switch(operation->kind)
  {
    case DT_AGENT_OPERATION_SET_FLOAT:
      g_set_error(
        error,
        _agent_execute_error_quark(),
        DT_AGENT_EXECUTE_ERROR_INVALID,
        "%s",
        _("white-balance numeric adjustments conflict with active chromatic adaptation; set white balance to camera reference or as shot to reference, or disable chromatic adaptation, then retry"));
      return FALSE;
    case DT_AGENT_OPERATION_SET_BOOL:
      if(operation->has_bool_value && !operation->bool_value)
      {
        g_set_error(
          error,
          _agent_execute_error_quark(),
          DT_AGENT_EXECUTE_ERROR_INVALID,
          "%s",
          _("disabling white balance conflicts with active chromatic adaptation; keep white balance enabled or disable chromatic adaptation, then retry"));
        return FALSE;
      }
      return TRUE;
    case DT_AGENT_OPERATION_SET_CHOICE:
      if(!operation->has_choice_value)
        return TRUE;
      if(operation->choice_value == 3 || operation->choice_value == 4)
        return TRUE;

      {
        const dt_agent_choice_option_t *option = _choice_option_for_value(descriptor, operation->choice_value);
        g_set_error(
          error,
          _agent_execute_error_quark(),
          DT_AGENT_EXECUTE_ERROR_INVALID,
          _("white-balance choice '%s' conflicts with active chromatic adaptation; use camera reference or as shot to reference, or disable chromatic adaptation, then retry"),
          option && option->label ? option->label : _("unknown"));
      }
      return FALSE;
    case DT_AGENT_OPERATION_UNKNOWN:
    default:
      return TRUE;
  }
}

static gboolean _validate_operation_contract(const dt_agent_chat_operation_t *operation,
                                             GError **error)
{
  if(g_strcmp0(operation->status, "planned") != 0)
  {
    g_set_error(error, _agent_execute_error_quark(), DT_AGENT_EXECUTE_ERROR_INVALID,
                _("unsupported operation status: %s"), operation->status ? operation->status : _("unknown"));
    return FALSE;
  }

  if(g_strcmp0(operation->target_type, DT_AGENT_TARGET_TYPE) != 0)
  {
    g_set_error(error, _agent_execute_error_quark(), DT_AGENT_EXECUTE_ERROR_INVALID,
                _("unsupported target type: %s"), operation->target_type ? operation->target_type : _("unknown"));
    return FALSE;
  }

  return TRUE;
}

static gboolean _execute_operation(const dt_agent_chat_operation_t *operation,
                                   dt_agent_execution_report_t *report,
                                   GError **error)
{
  dt_agent_execution_result_t *result = _execution_result_new(operation);
  g_ptr_array_add(report->results, result);

  if(!_validate_operation_contract(operation, error))
    return _execution_result_set_blocked(report, result, error, "%s", (*error)->message);

  g_autoptr(GError) descriptor_error = NULL;
  dt_agent_action_descriptor_t *descriptor = dt_agent_catalog_find_descriptor(
    darktable.develop, operation->action_path, operation->setting_id, &descriptor_error);
  if(!descriptor)
  {
    const char *message = descriptor_error && descriptor_error->message ? descriptor_error->message : _("unsupported action path");
    return _execution_result_set_blocked(report, result, error, "%s", message);
  }

  if(!_validate_white_balance_operation(operation, descriptor, error))
  {
    dt_agent_action_descriptor_free(descriptor);
    return _execution_result_set_blocked(report, result, error, "%s", (*error)->message);
  }

  gboolean ok = FALSE;
  switch(operation->kind)
  {
    case DT_AGENT_OPERATION_SET_FLOAT:
    {
      result->has_value_before = dt_agent_catalog_read_current_number(descriptor, &result->value_before, error);
      if(!result->has_value_before)
      {
        dt_agent_action_descriptor_free(descriptor);
        return _execution_result_set_failed(report, result, error);
      }

      double target = operation->number;
      if(operation->value_mode == DT_AGENT_VALUE_MODE_DELTA)
      {
        if(!dt_agent_catalog_supports_mode(descriptor, DT_AGENT_VALUE_MODE_DELTA))
        {
          dt_agent_action_descriptor_free(descriptor);
          return _execution_result_set_blocked(report, result, error,
                                               _("unsupported value mode for action path: %s"),
                                               operation->action_path ? operation->action_path : _("unknown"));
        }
        target = result->value_before + operation->number;
      }
      else if(operation->value_mode != DT_AGENT_VALUE_MODE_SET)
      {
        dt_agent_action_descriptor_free(descriptor);
        return _execution_result_set_blocked(report, result, error, "%s", _("unsupported numeric value mode"));
      }

      ok = dt_agent_catalog_write_number(descriptor, target, &result->value_after, error);
      result->has_value_after = ok;
      if(ok)
        _execution_result_set_message(result, "%s", _("applied"));
      break;
    }
    case DT_AGENT_OPERATION_SET_CHOICE:
    {
      if(operation->value_mode != DT_AGENT_VALUE_MODE_SET || !operation->has_choice_value)
      {
        dt_agent_action_descriptor_free(descriptor);
        return _execution_result_set_blocked(report, result, error, "%s", _("unsupported choice value mode"));
      }

      const dt_agent_choice_option_t *option = _choice_option_for_value(descriptor, operation->choice_value);
      if(!option)
      {
        dt_agent_action_descriptor_free(descriptor);
        return _execution_result_set_blocked(report, result, error,
                                             _("unsupported choice value for action path: %s"),
                                             operation->action_path ? operation->action_path : _("unknown"));
      }

      if(operation->choice_id && operation->choice_id[0]
         && g_strcmp0(operation->choice_id, option->choice_id) != 0)
      {
        dt_agent_action_descriptor_free(descriptor);
        return _execution_result_set_blocked(report, result, error,
                                             _("choice id mismatch for action path: %s"),
                                             operation->action_path ? operation->action_path : _("unknown"));
      }

      gint applied_choice_value = 0;
      ok = dt_agent_catalog_write_choice(descriptor, operation->choice_value, &applied_choice_value, error);
      if(ok)
        _execution_result_set_message(result, _("applied choice %d"), applied_choice_value);
      break;
    }
    case DT_AGENT_OPERATION_SET_BOOL:
    {
      if(operation->value_mode != DT_AGENT_VALUE_MODE_SET || !operation->has_bool_value)
      {
        dt_agent_action_descriptor_free(descriptor);
        return _execution_result_set_blocked(report, result, error, "%s", _("unsupported bool value mode"));
      }

      gboolean applied_bool = FALSE;
      ok = dt_agent_catalog_write_bool(descriptor, operation->bool_value, &applied_bool, error);
      if(ok)
        _execution_result_set_message(result, "%s", applied_bool ? _("applied on") : _("applied off"));
      break;
    }
    case DT_AGENT_OPERATION_UNKNOWN:
    default:
      dt_agent_action_descriptor_free(descriptor);
      return _execution_result_set_blocked(report, result, error,
                                           _("unsupported operation kind: %s"),
                                           operation->kind_name ? operation->kind_name : _("unknown"));
  }

  dt_agent_action_descriptor_free(descriptor);
  if(!ok)
    return _execution_result_set_failed(report, result, error);

  result->status = DT_AGENT_EXECUTION_STATUS_APPLIED;
  report->applied_count++;
  return TRUE;
}

gboolean dt_agent_execute_response(const dt_agent_chat_response_t *response,
                                   dt_agent_execution_report_t *report,
                                   GError **error)
{
  if(!response || !report || !report->results)
  {
    g_set_error(error, _agent_execute_error_quark(), DT_AGENT_EXECUTE_ERROR_INVALID,
                "%s", _("missing execution inputs"));
    return FALSE;
  }

  if(!response->operations)
  {
    g_set_error(error, _agent_execute_error_quark(), DT_AGENT_EXECUTE_ERROR_INVALID,
                "%s", _("chat response is missing operations"));
    return FALSE;
  }

  const dt_view_t *cv = darktable.view_manager ? darktable.view_manager->current_view : NULL;
  if(!cv || cv->view((dt_view_t *)cv) != DT_VIEW_DARKROOM)
  {
    g_set_error(error, _agent_execute_error_quark(), DT_AGENT_EXECUTE_ERROR_INVALID,
                "%s", _("agent edits require darkroom view"));
    return FALSE;
  }

  g_autoptr(GPtrArray) ordered_operations = _ordered_operations_for_execution(response->operations);

  gboolean all_ok = TRUE;
  g_autoptr(GError) first_error = NULL;
  for(guint i = 0; i < ordered_operations->len; i++)
  {
    const dt_agent_chat_operation_t *operation = g_ptr_array_index(ordered_operations, i);
    if(all_ok)
    {
      g_autoptr(GError) operation_error = NULL;
      if(!_execute_operation(operation, report, &operation_error))
      {
        all_ok = FALSE;
        if(operation_error)
          first_error = g_steal_pointer(&operation_error);
      }
      continue;
    }

    dt_agent_execution_result_t *result = _execution_result_new(operation);
    result->status = DT_AGENT_EXECUTION_STATUS_BLOCKED;
    result->message = g_strdup(_("blocked by a previous operation failure"));
    g_ptr_array_add(report->results, result);
    report->blocked_count++;
  }

  if(!all_ok && first_error)
    g_propagate_error(error, g_steal_pointer(&first_error));

  return all_ok;
}
