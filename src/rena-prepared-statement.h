/*
 * Copyright (C) 2024 Santelmo Technologies <santelmotechnologies@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef RENA_PREPARED_STATEMENT_H
#define RENA_PREPARED_STATEMENT_H

#include <glib.h>

G_BEGIN_DECLS

struct RenaPreparedStatement;
typedef struct RenaPreparedStatement RenaPreparedStatement;

void                     rena_prepared_statement_free              (RenaPreparedStatement *statement);
void                     rena_prepared_statement_bind_string       (RenaPreparedStatement *statement, gint n, const gchar *value);
void                     rena_prepared_statement_bind_int          (RenaPreparedStatement *statement, gint n, gint value);
gboolean                 rena_prepared_statement_step              (RenaPreparedStatement *statement);
gint                     rena_prepared_statement_get_int           (RenaPreparedStatement *statement, gint column);
const gchar *            rena_prepared_statement_get_string        (RenaPreparedStatement *statement, gint column);
void                     rena_prepared_statement_reset             (RenaPreparedStatement *statement);
const gchar *            rena_prepared_statement_get_sql           (RenaPreparedStatement *statement);

G_END_DECLS

#endif /* RENA_PREPARED_STATEMENT_H */
