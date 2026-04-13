#pragma once

#include "common/agent_capabilities.h"
#include "common/agent_protocol.h"
#include "common/introspection.h"

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define DT_AGENT_TARGET_TYPE "ansel-action"

#define DT_AGENT_VALUE_MODE_FLAG_SET (1u << 0)
#define DT_AGENT_VALUE_MODE_FLAG_DELTA (1u << 1)

struct dt_iop_module_t;
struct dt_develop_t;

typedef struct dt_agent_action_descriptor_t
{
  gchar *module_id;
  gchar *module_label;
  gchar *capability_id;
  gchar *setting_id;
  gchar *label;
  gchar *kind_name;
  gchar *target_type;
  gchar *action_path;
  dt_agent_operation_kind_t operation_kind;
  guint supported_modes;
  struct dt_iop_module_t *module;
  GtkWidget *widget;
  dt_introspection_field_t *field;
  gboolean has_number_range;
  double min_number;
  double max_number;
  double default_number;
  double step_number;
  GPtrArray *choices;
  gboolean has_default_choice_value;
  gint default_choice_value;
  gboolean has_default_bool;
  gboolean default_bool;
} dt_agent_action_descriptor_t;

void dt_agent_action_descriptor_free(gpointer data);
dt_agent_action_descriptor_t *dt_agent_action_descriptor_copy(
  const dt_agent_action_descriptor_t *src);

gboolean dt_agent_catalog_collect_descriptors(const struct dt_develop_t *dev,
                                              GPtrArray *descriptors,
                                              GError **error);
dt_agent_action_descriptor_t *dt_agent_catalog_find_descriptor(const struct dt_develop_t *dev,
                                                               const char *action_path,
                                                               const char *setting_id,
                                                               GError **error);
gboolean dt_agent_catalog_is_action_path_allowed(const char *action_path);

gboolean dt_agent_catalog_supports_mode(const dt_agent_action_descriptor_t *descriptor,
                                        dt_agent_value_mode_t mode);
double dt_agent_catalog_clamp_number(const dt_agent_action_descriptor_t *descriptor,
                                     double requested_number);
gboolean dt_agent_catalog_read_current_number(const dt_agent_action_descriptor_t *descriptor,
                                              double *out_number,
                                              GError **error);
gboolean dt_agent_catalog_read_current_choice(const dt_agent_action_descriptor_t *descriptor,
                                              gint *out_choice_value,
                                              gchar **out_choice_id,
                                              GError **error);
gboolean dt_agent_catalog_read_current_bool(const dt_agent_action_descriptor_t *descriptor,
                                            gboolean *out_bool_value,
                                            GError **error);
gboolean dt_agent_catalog_write_number(const dt_agent_action_descriptor_t *descriptor,
                                       double requested_number,
                                       double *out_applied_number,
                                       GError **error);
gboolean dt_agent_catalog_write_choice(const dt_agent_action_descriptor_t *descriptor,
                                       gint requested_choice_value,
                                       gint *out_applied_choice_value,
                                       GError **error);
gboolean dt_agent_catalog_write_bool(const dt_agent_action_descriptor_t *descriptor,
                                     gboolean requested_bool_value,
                                     gboolean *out_applied_bool_value,
                                     GError **error);

G_END_DECLS
