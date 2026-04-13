#pragma once

#include "common/agent_protocol.h"

G_BEGIN_DECLS

typedef enum dt_agent_execution_status_t
{
  DT_AGENT_EXECUTION_STATUS_UNKNOWN = 0,
  DT_AGENT_EXECUTION_STATUS_APPLIED,
  DT_AGENT_EXECUTION_STATUS_BLOCKED,
  DT_AGENT_EXECUTION_STATUS_FAILED,
} dt_agent_execution_status_t;

typedef struct dt_agent_execution_result_t
{
  gchar *operation_id;
  gchar *action_path;
  dt_agent_execution_status_t status;
  gchar *message;
  gboolean has_value_before;
  double value_before;
  gboolean has_value_after;
  double value_after;
} dt_agent_execution_result_t;

typedef struct dt_agent_execution_report_t
{
  GPtrArray *results;
  guint applied_count;
  guint blocked_count;
  guint failed_count;
} dt_agent_execution_report_t;

void dt_agent_execution_report_init(dt_agent_execution_report_t *report);
void dt_agent_execution_report_clear(dt_agent_execution_report_t *report);

const char *dt_agent_execution_status_to_string(dt_agent_execution_status_t status);

gboolean dt_agent_execute_response(const dt_agent_chat_response_t *response,
                                   dt_agent_execution_report_t *report,
                                   GError **error);

G_END_DECLS
