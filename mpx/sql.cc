//  MPX
//  Copyright (C) 2005-2007 MPX development.
//
//  This program is free software; you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation; either version 2 of the License, or
//  (at your option) any later version.
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
//  The MPX project hereby grants permission for non GPL-compatible GStreamer
//  plugins to be used and distributed together with GStreamer and MPX. This
//  permission is above and beyond the permissions granted by the GPL license
//  MPX is covered by.
#include "config.h"

#include <string>
#include <glibmm.h>
#include <glib/gstdio.h>
#include <boost/tuple/tuple.hpp>
#include <boost/variant.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <sqlite3.h>

#include "mpx/sql.hh"
#include "mpx/util-string.hh"

using namespace Glib;
using namespace std;

namespace
{
  // FIXME: Remove these
  using namespace MPX;
  using namespace MPX::SQL;

  static boost::format format_int  ("%lld");
  static boost::format format_real ("%f");

  static boost::format sql_error_f ("SQLite Error: %s, SQLite Error Code: %d, SQL String: '%s'");

  static boost::format sql_value_int_f ("'%s' %s DEFAULT 0 ");
  static boost::format sql_value_str_f ("'%s' %s DEFAULT NULL ");

  static boost::format sql_value_eq_f ("%s=%s");

} // anonymous

#define THROW_SQL_ERROR(sql, status) \
{ \
  if (status != 100 && status != 101) \
  { \
    g_log (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "SQL Error: %s (%d)", sqlite3_errmsg (m_sqlite), status); \
 \
    switch (status) \
    { \
      case SQLITE_ERROR: \
        throw SqlGenericError (sql); \
        break; \
 \
      case SQLITE_PERM: \
        throw SqlPermissionError (sql); \
        break; \
 \
      case SQLITE_ABORT: \
        throw SqlAbortError (sql); \
        break; \
 \
      case SQLITE_BUSY: \
        throw SqlBusyError (sql); \
        break; \
 \
      case SQLITE_LOCKED: \
        throw SqlLockedError (sql); \
        break; \
 \
      case SQLITE_NOMEM: \
        throw SqlOOM (sql); \
        break; \
 \
      case SQLITE_READONLY: \
        throw SqlReadOnlyError (sql); \
        break; \
 \
      case SQLITE_INTERRUPT: \
        throw SqlInterruptError (sql); \
        break; \
 \
      case SQLITE_IOERR: \
        throw SqlIOError (sql); \
        break; \
 \
      case SQLITE_CORRUPT: \
        throw SqlSQLCorruptError (sql); \
        break; \
 \
      case SQLITE_FULL: \
        throw SqlSQLFullError (sql); \
        break; \
 \
      case SQLITE_CANTOPEN: \
        throw SqlSQLOpenError (sql); \
        break; \
 \
      case SQLITE_PROTOCOL: \
        throw SqlLockError (sql); \
        break; \
 \
      case SQLITE_EMPTY: \
        throw SqlSQLEmptyError (sql); \
        break; \
 \
      case SQLITE_SCHEMA: \
        throw SqlSchemaError (sql); \
        break; \
 \
      case SQLITE_CONSTRAINT: \
        throw SqlConstraintError (sql); \
        break; \
 \
      case SQLITE_MISMATCH: \
        throw SqlDatatypeError (sql); \
        break; \
 \
      case SQLITE_MISUSE: \
        throw SqlMisuseError (sql); \
        break; \
 \
      case SQLITE_NOLFS: \
        throw SqlNoLFSError (sql); \
        break; \
 \
      case SQLITE_AUTH: \
        throw SqlAuthError (sql); \
        break; \
 \
      case SQLITE_FORMAT: \
        throw SqlFormatError (sql);  \
        break; \
 \
      case SQLITE_RANGE: \
        throw SqlRangeError (sql); \
        break; \
 \
      case SQLITE_NOTADB: \
        throw SqlNotSQLError (sql); \
        break; \
    } \
  } \
} // !THROW_SQL_ERROR

namespace MPX
{
  void
  absRatingFunc (sqlite3_context *ctx, int argc, sqlite3_value **argv)
  {
	gint64 trackRating = sqlite3_value_int64(argv[0]);
	gint64 albumRating = sqlite3_value_int64(argv[1]);
	gint64 absRating = (albumRating * 5) + trackRating;
	sqlite3_result_int64(ctx, absRating);
  }

  string
  mprintf (const char *format, ...)
  {
    va_list args;
    va_start (args, format);
    char *v = sqlite3_vmprintf (format, args);
    va_end (args);
    string r (v);
    sqlite3_free (v);
    return r; 
  }

  string
  gprintf (const char *format, ...)
  {
    va_list args;
    va_start (args, format);
    Glib::ScopedPtr<char> s (g_strdup_vprintf (format, args));
    va_end (args);
    return std::string(s.get());
  }

  namespace SQL
  {
      std::string
      SQLDB::list_columns_simple (ColumnV const& columns) const
      {
        ColumnV::const_iterator i = columns.begin ();
        ColumnV::const_iterator e = columns.end ();
        ColumnV::const_iterator l = columns.end ();
        l--;

        std::string sql;
        sql.reserve (2048);

        for ( ; i != e ; ++i)
        {
          sql += ' ';
          sql += *i;
          sql += ' ';

          if (i != l)
          {
            sql += ',';
          }
        }
        return sql;
      }

      std::string
      SQLDB::list_columns (ColumnV  const &columns,
                              string    const &value,
                              bool      exact_match) const
      {
        using boost::algorithm::split;
        using boost::algorithm::is_any_of;

        StrV subs;
        std::string splitstring (value);
        split (subs, splitstring, is_any_of (" "));

        ColumnV::const_iterator i = columns.begin ();
        ColumnV::const_iterator e = columns.end ();
        ColumnV::const_iterator l = columns.end ();
        l--;

        std::string v;
        v.reserve (2048); // we reserve 2K so we don't need to realloc for every single append

        if (exact_match)
        {
          static boost::format f_exact (" %s='%s' ");
          for ( ; i != e; ++i)
          {
            v += '(';

            StrV::const_iterator s_i = subs.begin ();
            StrV::const_iterator s_e = subs.end ();
            StrV::const_iterator s_l = subs.end ();
            s_l--;

            for ( ; s_i != s_e; ++s_i)
            {
              if (s_i->empty())
                continue;

              v += (f_exact % (*i) % mprintf ("%q", s_i->c_str())).str ();
              if (s_i != s_l)
              {
                v += " AND ";
              }
            }
            v += ')';
            if (i != l)
            {
              v += " OR ";
            }
          }
        }
        else
        {
          static boost::format f_like  (" %s LIKE '%%%s%%' ");

          for ( ; i != e; ++i)
          {
            v += '(';

            StrV::const_iterator s_i = subs.begin ();
            StrV::const_iterator s_e = subs.end ();
            StrV::const_iterator s_l = subs.end ();
            s_l--;

            for ( ; s_i != s_e; ++s_i)
            {
              v += (f_like % (*i) % mprintf ("%q", s_i->c_str ())).str ();
              if (s_i != s_l)
              {
                v += " AND ";
              }
            }
            v += ")";
            if (i != l)
            {
              v += " OR ";
            }
          }
        }
        return v;
      }

      std::string
      SQLDB::variant_serialize (Variant const& value) const
      {
        switch (value.which())
        {
          case VALUE_TYPE_STRING:
            return (mprintf ("'%q'", (boost::get<string> (value)).c_str ()));

          case VALUE_TYPE_INT:
            return (format_int % boost::get<guint64> (value)).str();

          case VALUE_TYPE_REAL:
            return (format_real % boost::get<double> (value)).str();
        }

        return std::string (); // borken GCC!
      }

      std::string
      SQLDB::attributes_serialize (Query const& query) const
      {
          std::string sql;
          sql.reserve (0x800);

          AttributeSeq::size_type seq_idx = 0;
          AttributeSeq const& attributes (query.m_attributes);
          for (AttributeSeq::const_iterator seq_i = attributes.begin(), seq_i_last = --attributes.end(); seq_i != attributes.end(); ++seq_i)
          {
            AttributeV const& v (seq_i->first);
            AttributeChain const& chain (seq_i->second);

            sql += "(";

            AttributeChain::size_type attr_idx = 0;
            for (AttributeV::const_iterator i = v.begin(), i_last = --v.end(); i != v.end(); ++i)
            {
              Attribute const& attribute = *i;
              bool use_op = true;

              if (attribute.m_match_style == FUZZY)
                {
                  switch (attribute.m_variant.which())
                  {
                    case VALUE_TYPE_STRING:
                    {
                      string str = (boost::get<string> (attribute.m_variant));

                      sql += mprintf (" %s LIKE '%%%q%%' ", attribute.m_name.c_str(), str.c_str ());
                      break;
                    }

                    case VALUE_TYPE_INT:
                    {
                      sql += attribute.m_name;
                      sql += " LIKE '%";
                      sql += (format_int % boost::get<guint64> (attribute.m_variant)).str ();
                      sql += "%' ";
                      break;
                    }
                  }
                }
              else
              if (attribute.m_match_style == NOT_NULL)
                {
                  sql += mprintf (" %s NOT NULL ", attribute.m_name.c_str ());
                  use_op = false;
                }
              else
              if (attribute.m_match_style == IS_NULL)
                {
                  sql += mprintf (" (%s IS NULL OR length(%s) = 0) ",
                                      attribute.m_name.c_str(), attribute.m_name.c_str());
                  use_op = false;
                }
              else
                {
                  std::string op;

                  switch (attribute.m_match_style)
                  {
                    case EXACT:
                    {
                      op = "="; break;
                    }

                    case NOT_EQUAL:
                    {
                      op = "!="; break;
                    }

                    case LESSER_THAN:
                    {
                      op = "<"; break;
                    }

                    case GREATER_THAN:
                    {
                      op = ">"; break;
                    }

                    case LESSER_THAN_OR_EQUAL:
                    {
                      op = "<="; break;
                    }

                    case GREATER_THAN_OR_EQUAL:
                    {
                      op = ">="; break;
                    }

                    case IS_NULL:
                    case NOT_NULL:
                    case FUZZY:
                    case NOOP:
                    {
                        // do nothing, all are handled above (and NOOP needs nothing)
                    }
                  }

                  if (use_op)
                    {
                      switch (attribute.m_variant.which())
                        {
                          case VALUE_TYPE_STRING:
                            {
                              string str = (boost::get<string> (attribute.m_variant));
                              sql += mprintf (" %s %s '%q' ", attribute.m_name.c_str(), op.c_str(), str.c_str());
                              break;
                            }

                          case VALUE_TYPE_INT:
                            {
                              std::string str = (format_int % boost::get<guint64> (attribute.m_variant)).str ();
                              sql += mprintf (" %s %s '%q' ", attribute.m_name.c_str(), op.c_str(), str.c_str ());
                              break;
                            }
                        }
                    }
                }

            if (i != i_last)
            {
              if (!chain.empty())
              {
                switch (chain[attr_idx])
                {
                  case CHAIN_AND:
                    sql += " AND ";
                    break;

                  case CHAIN_OR:
                    sql += " OR ";
                    break;
                }
              }
              else
              {
                sql += " AND ";
              }
            }
          }

          sql += ")";

          if (seq_i != seq_i_last)
          {
            if (!query.m_seq_chain.empty())
            {
              switch (query.m_seq_chain[seq_idx])
              {
                case CHAIN_AND:
                  sql += " AND ";
                  break;

                case CHAIN_OR:
                  sql += " OR ";
                  break;
              }
            }
            else
            {
              sql += " AND ";
            }
          }
        }
        return sql;
      }

      int
      SQLDB::statement_prepare (sqlite3_stmt **stmt,
                                   string const &sql,
                                   sqlite3 * sql_local) const //LIES!
      
      {
        sqlite3 * sql_use = 0;

        if (sql_local)
          sql_use = sql_local;
        else
          sql_use = m_sqlite;

        char const* tail;
        int status = sqlite3_prepare_v2 (sql_use,
                                         sql.c_str(),
                                         std::strlen (sql.c_str ()),
                                         stmt,
                                         &tail);

        if (status == SQLITE_SCHEMA)
        {
          sqlite3_reset (*stmt);
          status = sqlite3_prepare (sql_use,
                                    sql.c_str(),
                                    std::strlen (sql.c_str()),
                                    stmt,
                                    &tail);
        }

        if (status)
        {
          g_warning ("SQL Error: '%s', SQL Statement: '%s'", sqlite3_errmsg (m_sqlite), sql.c_str ());
          sqlite3_reset (*stmt);
        }

        return status;
      }

      void
      SQLDB::assemble_row (sqlite3_stmt* stmt,
                               RowV & rows) const
      {
        unsigned int columns = sqlite3_column_count (stmt);
    
        Row row;
        for (unsigned int n = 0 ; n < columns; ++n)
        {
              const char * name = sqlite3_column_name (stmt, n);
              if (name)
              {
                    switch (sqlite3_column_type (stmt, n))
                    {
                      case SQLITE_NULL:
                      {
                        /*....*/
                        break;
                      }

                      case SQLITE_INTEGER:
                      {
                        row.insert (std::make_pair (name, gint64 (sqlite3_column_int64 (stmt, n))));
                        break;
                      }

                      case SQLITE_FLOAT:
                      {
                        row.insert (std::make_pair (name, double (sqlite3_column_double (stmt, n))));
                        break;
                      }

                      case SQLITE_TEXT:
                      {
                        char const* value = reinterpret_cast<char const*> (sqlite3_column_text (stmt, n));
                        if (value)
                        {
                          row.insert (std::make_pair (name, string (value)));
                        }
                        break;
                      }
                    }

              }
        }
        rows.push_back (row);
      }

      void print_compiled_statement (sqlite3_stmt *stmt)
      {
        int count = sqlite3_bind_parameter_count (stmt);
        for (int n = 0; n < count; ++n)
        {
        }
      }

      unsigned int
      SQLDB::exec_sql (string const& sql)
      {
        int            status = 0;
        sqlite3_stmt  *stmt   = 0;
        unsigned int   rows   = 0;

        statement_prepare (&stmt, sql);

        for ( ; status != SQLITE_DONE ; )
        {
          status = sqlite3_step (stmt);

          if (status == SQLITE_BUSY)
            continue;

          if (status == SQLITE_DONE)
            break;

          if (status == SQLITE_ROW)
          {
            rows++;
            continue;
          }

          if (status != SQLITE_OK)
          {
            sqlite3_finalize (stmt);
            THROW_SQL_ERROR(sql, status)
          }
        }

        sqlite3_finalize (stmt);
        return rows;
      }

      SQLDB::SQLDB (std::string const& name, std::string const& path, DbOpenMode openmode)
        : m_name (name),
          m_path (path)
      {
        m_filename = build_filename (path, name) + ".mlib";

        if (file_test (m_filename, FILE_TEST_EXISTS) && openmode == SQLDB_TRUNCATE) {
          g_unlink (m_filename.c_str());
        }

        if (sqlite3_open (m_filename.c_str(), &m_sqlite)) {
          throw DbInitError ((boost::format("Unable to open database at '%s'") % m_filename).str());
        }

#ifdef SQLITE_TRACE
        sqlite3_trace (m_sqlite, SQLDB::sqlite_trace, this);
#endif //SQLITE_TRACE

		sqlite3_create_function(m_sqlite, "abs_rating", 2, SQLITE_ANY, this, absRatingFunc, 0, 0);

        exec_sql ("PRAGMA synchronous=OFF;"); 
      }

      SQLDB::~SQLDB ()
      {
        sqlite3_close (m_sqlite);
      }

      void
      SQLDB::del (std::string  const&  name,
                     AttributeV   const&  attrs)
      {
        static boost::format sql_delete_f ("DELETE FROM %s WHERE %s;");
        Query query (attrs);
        exec_sql ((sql_delete_f % name % attributes_serialize (attrs)).str());
      }

      void
      SQLDB::set     (std::string  const& name,
                         std::string  const& pkey,
                         std::string  const& pkey_value,
                         AttributeV   const& attributes)
      {
        std::string sql;
        sql.reserve (0x400);

        for (AttributeV::const_iterator i = attributes.begin(), i_last = --attributes.end(); i != attributes.end(); ++i)
        {
          Attribute const& attribute = *i;
          sql += (sql_value_eq_f % attribute.m_name % variant_serialize (attribute.m_variant)).str();
          if (i != i_last)
            sql += " , ";
        }

        static boost::format sql_update_f ("UPDATE %s SET %s WHERE %s='%s';");
        exec_sql ((sql_update_f % name % sql.c_str() % pkey.c_str() % pkey_value.c_str()).str());
      }

      void
      SQLDB::set (std::string    const& name,
                     string  const& where_clause,
                     AttributeV    const& attributes)
      {
        std::string sql;
        sql.reserve (0x400);

        for (AttributeV::const_iterator i = attributes.begin(), i_last = --attributes.end(); i != attributes.end(); ++i)
        {
          Attribute const& attribute = *i;
          sql += (sql_value_eq_f % attribute.m_name % variant_serialize (attribute.m_variant)).str ();
          if (i != i_last)
            sql += " , ";
        }

        static boost::format sql_update_where_f ("UPDATE %s SET %s %s;");
        exec_sql ((sql_update_where_f % name % sql.c_str() % where_clause.c_str()).str ());
      }

      void
      SQLDB::get (RowV & rows, string const& sql, bool reopen_db) const
      {
        // Re-open the db if neccessary
        sqlite3 * sql_local = 0;
        if (reopen_db)
        {
          sqlite3_open (m_filename.c_str(), &sql_local);
        }

        sqlite3_stmt * stmt = 0;
        int status = SQLITE_OK;
        do{
          status = statement_prepare (&stmt, sql, sql_local);
          if (status != SQLITE_OK)
          {
            THROW_SQL_ERROR(sql, status)
          }
          do{
            status = sqlite3_step (stmt);
            switch (status)
            {
              case SQLITE_BUSY:
                continue;
              case SQLITE_ROW:
              {
                assemble_row (stmt, rows);
                break;
              }
            }
            if (status != SQLITE_OK)
            {
              if (sql_local)
                sqlite3_close (sql_local);
              THROW_SQL_ERROR(sql, status)
            }
          } while (status != SQLITE_DONE);
          status = sqlite3_finalize (stmt);
        } while (status == SQLITE_SCHEMA);
      
        if (sql_local)
          sqlite3_close (sql_local);
      }

      void
      SQLDB::get ( std::string const& name,
                      RowV             & rows,
                      Query       const& query,
                      bool               reopen_db) const
      {
        char const* 
          sql_select_where_f ("SELECT %s %s FROM %s WHERE %s %s;");
        char const* 
          sql_select_f ("SELECT %s %s FROM %s %s;");

        std::string sql;
        sql.reserve (0x400);

        if (!query.m_attributes.empty ())
          {
            sql += (mprintf (sql_select_where_f,
                                  query.m_prefix.c_str(),
                                  query.m_columns.c_str(),
                                  name.c_str(),
                                  attributes_serialize (query).c_str(),
                                  query.m_suffix.c_str()));
          }
        else
          {
            sql += (mprintf ( sql_select_f,
                                  query.m_prefix.c_str(),
                                  query.m_columns.c_str(),
                                  name.c_str(),
                                  query.m_suffix.c_str()));
          }

        // Re-open the db if neccessary
        sqlite3 * sql_local = 0;
        if (reopen_db)
        {
          sqlite3_open (m_filename.c_str(), &sql_local);
        }

        sqlite3_stmt * stmt = 0;
        int status = SQLITE_OK;
        do{
          status = statement_prepare (&stmt, sql, sql_local);
          if (status != SQLITE_OK)
          {
            THROW_SQL_ERROR(sql, status)
          }
          do{
            status = sqlite3_step (stmt);
            switch (status)
            {
              case SQLITE_BUSY:
                continue;
              case SQLITE_ROW:
              {
                assemble_row (stmt, rows);
                break;
              }
            }
            if (status != SQLITE_OK)
            {
              if (sql_local)
                sqlite3_close (sql_local);
              THROW_SQL_ERROR(sql, status)
            }
          } while (status != SQLITE_DONE);
          status = sqlite3_finalize (stmt);
        } while (status == SQLITE_SCHEMA);

        if (sql_local)
          sqlite3_close (sql_local);
      }

      bool
      SQLDB::table_exists (std::string const& name)
      {
        static boost::format sql_table_exists_f ("SELECT name FROM sqlite_master WHERE name='%s';");
        return (0 != exec_sql ((sql_table_exists_f % name).str()));
      }

  } // SQL
} // MPX
