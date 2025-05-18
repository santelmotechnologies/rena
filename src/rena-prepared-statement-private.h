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

#ifndef RENA_PREPARED_STATEMENT_PRIVATE_H
#define RENA_PREPARED_STATEMENT_PRIVATE_H

#include <sqlite3.h>

#include "rena-database.h"
#include "rena-prepared-statement.h"

RenaPreparedStatement* rena_prepared_statement_new               (sqlite3_stmt *stmt, RenaDatabase *database);
void                     rena_prepared_statement_finalize          (RenaPreparedStatement *statement);

#endif /* RENA_PREPARED_STATEMENT_PRIVATE_H */
