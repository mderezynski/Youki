//  MPX
//  Copyright (C) 2005-2007 MPX Project 
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License Version 2
//  as published by the Free Software Foundation.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
//
//  --
//
//  The MPX project hereby grants permission for non-GPL compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.
#ifndef MPX_SQL_HH
#define MPX_SQL_HH

#include "config.h"
#include <boost/variant.hpp>
#include <glibmm.h>
#ifdef HAVE_TR1
#include<tr1/unordered_map>
#endif // HAVE_TR1
#include <string>
#include <vector>
#include <sqlite3.h>

#include "mpx/types.hh"

namespace MPX
{
  std::string
  mprintf (const char *format, ...);

  std::string
  gprintf (const char *format, ...);

  namespace SQL
  {

#include "mpx/exception-db.hh"

    EXCEPTION(DbInitError)

    SQL_EXCEPTION(SqlGenericError, SQLITE_ERROR)
    SQL_EXCEPTION(SqlPermissionError, SQLITE_PERM)
    SQL_EXCEPTION(SqlAbortError, SQLITE_ABORT)
    SQL_EXCEPTION(SqlBusyError, SQLITE_BUSY)
    SQL_EXCEPTION(SqlLockedError, SQLITE_LOCKED)
    SQL_EXCEPTION(SqlOOM, SQLITE_NOMEM)
    SQL_EXCEPTION(SqlReadOnlyError, SQLITE_READONLY)
    SQL_EXCEPTION(SqlInterruptError, SQLITE_INTERRUPT)
    SQL_EXCEPTION(SqlIOError, SQLITE_IOERR)
    SQL_EXCEPTION(SqlSQLCorruptError, SQLITE_CORRUPT)
    SQL_EXCEPTION(SqlSQLFullError, SQLITE_FULL)
    SQL_EXCEPTION(SqlSQLOpenError, SQLITE_CANTOPEN)
    SQL_EXCEPTION(SqlLockError, SQLITE_PROTOCOL)
    SQL_EXCEPTION(SqlSQLEmptyError, SQLITE_EMPTY)
    SQL_EXCEPTION(SqlSchemaError, SQLITE_SCHEMA)
    SQL_EXCEPTION(SqlConstraintError, SQLITE_CONSTRAINT)
    SQL_EXCEPTION(SqlDatatypeError, SQLITE_MISMATCH)
    SQL_EXCEPTION(SqlMisuseError, SQLITE_MISUSE)
    SQL_EXCEPTION(SqlNoLFSError, SQLITE_NOLFS)
    SQL_EXCEPTION(SqlAuthError, SQLITE_AUTH)
    SQL_EXCEPTION(SqlFormatError, SQLITE_FORMAT)
    SQL_EXCEPTION(SqlRangeError, SQLITE_RANGE)
    SQL_EXCEPTION(SqlNotSQLError, SQLITE_NOTADB)

    enum HookType
    {
      HOOK_INSERT,
      HOOK_DELETE,
      HOOK_UPDATE,

      N_HOOKS,
    };

    enum DbOpenMode
    {
      SQLDB_TRUNCATE,
      SQLDB_OPEN,
    };

    enum TableRowMode
    {
      ROW_MODE_DEFAULT,
      ROW_MODE_REPLACE,
    };

    class SQLDB
    {
      public:

        SQLDB (std::string const& name, std::string const& path, DbOpenMode mode = SQLDB_TRUNCATE);
        ~SQLDB ();

        bool
        table_exists (std::string const& name);

        void
        get (std::string  const& name,
             RowV              & rows,
             Query        const& query = Query(),
             bool                reopen_db = false) const;

        void
        get (RowV               & rows,
             std::string   const& sql,
             bool                reopen_db = false) const;
            
        void
        set (std::string const& name,
             std::string const& key,
             std::string const& value,
             AttributeV  const& attributes);

        void
        set (std::string   const& name,
             std::string   const& where_clause,
             AttributeV    const& attributes);

        void
        del (std::string const& name,
             AttributeV  const& attributes);

        unsigned int
        exec_sql (std::string const& sql);

        int64_t
        last_insert_rowid ()
        {
          return sqlite3_last_insert_rowid (m_sqlite);
        }

      private:

        std::string
        list_columns_simple (ColumnV const& columns) const;

        std::string
        list_columns (ColumnV     const &columns,
                      std::string const &value,
                      bool              exact_match) const;

        std::string
        variant_serialize (Variant const& value) const;

        std::string
        attributes_serialize (Query const& query) const;

        int
        statement_prepare (sqlite3_stmt **stmt,
                           std::string const &sql,
                           sqlite3 * db = 0) const;

        void
        assemble_row (sqlite3_stmt* stmt,
                      RowV & rows) const;

        static void
        randFunc (sqlite3_context *ctx, int argc, sqlite3_value **argv);

        std::string     m_name;
        std::string     m_path; //sqlite3 filename = path+name+".mlib";
        std::string     m_filename; 

        sqlite3       * m_sqlite;
        Glib::Rand      m_rand;
    };
  } // namespace SQL
}; // namespace MPX

#endif // !MPX_SQL_HH
