/*
 * Bastile
 *
 * Copyright (C) 2004 Stefan Walter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

/**
 * Functions for tying BastileOperation objects to the UI.
 */
 
#ifndef __BASTILE_PROGRESS_H__
#define __BASTILE_PROGRESS_H__

#include "bastile-operation.h"
#include "bastile-widget.h"
#include "bastile-context.h"

/* -----------------------------------------------------------------------------
 * BASTILE APPBAR HOOKS
 */

/* Updates two glade widgets 'status' and 'progress' in the BastileWidget */
void                bastile_progress_status_set_operation      (BastileWidget *swidget,
                                                                 BastileOperation *operation);

/* Gets the operation set with above function */
BastileOperation*  bastile_progress_status_get_operation      (BastileWidget *swidget);

/* Keeps operation refed until progress dialog goes away */
void                bastile_progress_show                      (BastileOperation *operation,
                                                                 const gchar *title,
                                                                 gboolean delayed);

#endif /* __BASTILE_PROGRESS_H__ */
