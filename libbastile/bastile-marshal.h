
#ifndef __bastile_marshal_MARSHAL_H__
#define __bastile_marshal_MARSHAL_H__

#include	<glib-object.h>

G_BEGIN_DECLS

/* VOID:STRING,DOUBLE (bastile-marshal.list:1) */
extern void bastile_marshal_VOID__STRING_DOUBLE (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);

/* VOID:OBJECT,UINT (bastile-marshal.list:2) */
extern void bastile_marshal_VOID__OBJECT_UINT (GClosure     *closure,
                                                GValue       *return_value,
                                                guint         n_param_values,
                                                const GValue *param_values,
                                                gpointer      invocation_hint,
                                                gpointer      marshal_data);

/* VOID:OBJECT,POINTER (bastile-marshal.list:3) */
extern void bastile_marshal_VOID__OBJECT_POINTER (GClosure     *closure,
                                                   GValue       *return_value,
                                                   guint         n_param_values,
                                                   const GValue *param_values,
                                                   gpointer      invocation_hint,
                                                   gpointer      marshal_data);

/* VOID:OBJECT,UINT,POINTER (bastile-marshal.list:4) */
extern void bastile_marshal_VOID__OBJECT_UINT_POINTER (GClosure     *closure,
                                                        GValue       *return_value,
                                                        guint         n_param_values,
                                                        const GValue *param_values,
                                                        gpointer      invocation_hint,
                                                        gpointer      marshal_data);

/* VOID:OBJECT,OBJECT (bastile-marshal.list:5) */
extern void bastile_marshal_VOID__OBJECT_OBJECT (GClosure     *closure,
                                                  GValue       *return_value,
                                                  guint         n_param_values,
                                                  const GValue *param_values,
                                                  gpointer      invocation_hint,
                                                  gpointer      marshal_data);

G_END_DECLS

#endif /* __bastile_marshal_MARSHAL_H__ */
