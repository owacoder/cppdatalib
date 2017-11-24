/*
 * mariadb.h
 *
 * Copyright © 2017 Oliver Adams
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef CPPDATALIB_MARIADB_H
#define CPPDATALIB_MARIADB_H

#ifndef CPPDATALIB_DISABLE_MARIADB

#include "../core/core.h"
#include <mysql/mysql.h>

namespace cppdatalib
{
    namespace mariadb
    {
        // The MySQL connection is assumed to be active, with a selected database, when connect() is called. Calling without these preconditions
        // fulfilled will result in an error being thrown.
        class parser : public core::stream_input
        {
            MYSQL *mysql;

            std::vector<std::string> types;
            std::string db;
            std::string table;
            bool escaped;

            std::string escape(const std::string &str)
            {
                std::string temp;
                temp.resize(str.length() * 2 + 1);
                unsigned long length = mysql_real_escape_string(mysql, &temp[0], str.c_str(), str.length());
                temp.resize(length);
                return temp;
            }

        public:
            parser(MYSQL *connection,
                   const std::string &db,
                   const std::string &table)
                : mysql(connection)
                , db(db)
                , table(table)
                , escaped(false)
            {}

            void connect(const char *host,
                         const char *user,
                         const char *passwd,
                         const char *db,
                         unsigned int port = 0,
                         unsigned long client_flag = 0)
            {
                if (!mysql_real_connect(mysql, host, user, passwd, db, port, NULL, client_flag))
                    throw core::error("MySQL - could not connect to specified database");
            }

            bool provides_prefix_string_size() const {return true;}
            bool provides_prefix_object_size() const {return true;}
            bool provides_prefix_array_size() const {return true;}

            // Returns the datatypes of columns from the previous convert() operation
            // Datatypes include the type, size specifier, and any other attributes (like the UNSIGNED attribute)
            std::vector<std::string> get_datatypes() const {return types;}

            core::stream_input &convert(core::stream_handler &writer)
            {
                MYSQL_RES *result = NULL;

                writer.begin();
                writer.begin_array(core::array_t(), core::stream_handler::unknown_size);

                try
                {
                    std::string query;
                    MYSQL_ROW row;

                    if (!escaped)
                    {
                        db = escape(db);
                        table = escape(table);
                        escaped = true;
                    }

                    //
                    // `EXPLAIN` to get the types of the data
                    //
                    query = "EXPLAIN " + db + "." + table;
                    if (mysql_real_query(mysql, query.c_str(), query.length()))
                    {
                        std::cerr << mysql_error(mysql) << std::endl;
                        throw core::error("MySQL - column type query failed");
                    }

                    result = mysql_use_result(mysql);
                    if (result == NULL)
                        throw core::error("MySQL - an error occured while attempting to use the query result");

                    unsigned int columns = mysql_num_fields(result);
                    if (columns < 2)
                        throw core::error("MySQL - invalid response while attempting to get column types");

                    types.clear();
                    while ((row = mysql_fetch_row(result)) != NULL)
                        types.push_back(row[1]? row[1]: "");

                    mysql_free_result(result);
                    result = NULL;

                    //
                    // `SELECT` all data out of the specified table
                    //
                    query = "SELECT * FROM " + table;
                    if (mysql_real_query(mysql, query.c_str(), query.length()))
                    {
                        std::cerr << mysql_error(mysql) << std::endl;
                        throw core::error("MySQL - table data query failed");
                    }

                    result = mysql_use_result(mysql);
                    if (result == NULL)
                        throw core::error("MySQL - an error occured while attempting to use the query result");

                    columns = mysql_num_fields(result);

                    while ((row = mysql_fetch_row(result)) != NULL)
                    {
                        writer.begin_array(core::array_t(), columns);

                        for (unsigned int i = 0; i < columns; ++i)
                        {
                            if (row[i] == NULL)
                                writer.write(core::null_t());
                            else
                            {
                                core::value value(row[i]);

                                // TODO: doesn't support ENUM values as distinct values
                                // TODO: doesn't support SET values as distinct values

                                if (types[i].find("char") != std::string::npos ||
                                        types[i].find("varchar") != std::string::npos ||
                                        types[i].find("tinytext") != std::string::npos ||
                                        types[i].find("text") != std::string::npos ||
                                        types[i].find("mediumtext") != std::string::npos ||
                                        types[i].find("longtext") != std::string::npos)
                                    value.convert_to_string();
                                else if (types[i].find("blob") != std::string::npos ||
                                         types[i].find("mediumblob") != std::string::npos ||
                                         types[i].find("longblob") != std::string::npos)
                                {
                                    value.convert_to_string();
                                    value.set_subtype(core::blob);
                                }
                                else if (types[i].find("tinyint") != std::string::npos ||
                                         types[i].find("smallint") != std::string::npos ||
                                         types[i].find("mediumint") != std::string::npos ||
                                         types[i].find("int") != std::string::npos ||
                                         types[i].find("bigint") != std::string::npos ||
                                         types[i].find("year") != std::string::npos)
                                {
                                    if (types[i].find("unsigned") != std::string::npos)
                                        value.convert_to_uint();
                                    else
                                        value.convert_to_int();
                                }
                                else if (types[i].find("float") != std::string::npos ||
                                         types[i].find("double") != std::string::npos)
                                    value.convert_to_real();
                                else if (types[i].find("decimal") != std::string::npos)
                                    value.set_subtype(core::bignum);
                                else if (types[i].find("date") != std::string::npos)
                                    value.set_subtype(core::date);
                                else if (types[i].find("time") != std::string::npos)
                                    value.set_subtype(core::time);
                                else if (types[i].find("datetime") != std::string::npos ||
                                         types[i].find("timestamp") != std::string::npos)
                                    value.set_subtype(core::datetime);

                                writer.write(value);
                            }
                        }

                        writer.end_array(core::array_t());
                    }

                    mysql_free_result(result);
                    result = NULL;
                }
                catch (core::error)
                {
                    if (result)
                        mysql_free_result(result);
                    throw;
                }

                writer.end_array(core::array_t());
                writer.end();

                return *this;
            }
        };
    }
}

#endif // CPPDATALIB_BUILD_MARIADB

#endif // CPPDATALIB_MARIADB_H
