#pragma once

#include "common/agent_protocol.h"

G_BEGIN_DECLS

typedef struct _dt_job_t dt_job_t;
typedef struct dt_agent_client_request_t dt_agent_client_request_t;

typedef struct dt_agent_client_result_t
{
  gint http_status;
  gchar *endpoint;
  gchar *transport_error;
  gboolean cancelled;
  gboolean has_response;
  dt_agent_chat_response_t response;
} dt_agent_client_result_t;

typedef struct dt_agent_client_progress_t
{
  gint http_status;
  gchar *endpoint;
  gchar *transport_error;
  gboolean cancelled;
  gboolean found;
  gboolean has_response;
  gchar *status;
  gchar *message;
  gchar *last_tool_name;
  guint tool_calls_used;
  guint tool_calls_max;
  guint applied_operation_count;
  gboolean requires_render_callback;
  dt_agent_chat_response_t response;
} dt_agent_client_progress_t;

typedef void (*dt_agent_client_callback_t)(const dt_agent_client_result_t *result,
                                           gpointer user_data);
typedef void (*dt_agent_client_progress_callback_t)(const dt_agent_client_progress_t *progress,
                                                    gpointer user_data);

void dt_agent_client_result_clear(dt_agent_client_result_t *result);
void dt_agent_client_progress_clear(dt_agent_client_progress_t *progress);
char *dt_agent_client_dup_endpoint(void);
void dt_agent_client_request_cancel(dt_agent_client_request_t *request, const char *reason);
void dt_agent_client_request_unref(dt_agent_client_request_t *request);

dt_agent_client_request_t *dt_agent_client_chat_async(const dt_agent_chat_request_t *request,
                                                      dt_agent_client_callback_t callback,
                                                      dt_agent_client_progress_callback_t progress_callback,
                                                      gpointer user_data,
                                                      GDestroyNotify destroy,
                                                      GError **error);

G_END_DECLS
