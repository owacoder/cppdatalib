/*
 * filesystem.h
 *
 * Copyright © 2018 Oliver Adams
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

#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#if __cplusplus >= 201703L

#include "../core/core.h"
#include <filesystem>
#include <iostream>
#include <fstream>

#elif __cplusplus >= 201402L

#include "../core/core.h"
#include <experimental/filesystem>
#include <iostream>
#include <fstream>

#endif

#if __cplusplus >= 201402L
namespace cppdatalib
{
    namespace filesystem
    {
#if __cplusplus >= 201703L
        namespace fs = std::filesystem;
#else
        namespace fs = std::experimental::filesystem;
#endif

        class stream_writer : public core::stream_handler
        {
            fs::path path_root;
            std::vector<std::string> directories;

            std::ofstream stream;
            std::string key;
            bool scalar_is_key;

            bool safe_write; // true: don't allow overwriting files, false: allow overwriting files
            bool safe_dirs; // true: don't allow pre-existing directories, false: allow pre-existing directories

        public:
            // By default, pre-existing files and directories are not allowed
            stream_writer(const fs::path &path_root, bool safe_write = true, bool safe_dirs = true) : path_root(path_root), safe_write(safe_write), safe_dirs(safe_dirs) {}

            std::string name() const {return "cppdatalib::filesystem::stream_writer";}

            void set_safe_write(bool safe) {safe_write = safe;}
            bool is_safe_write() const {return safe_write;}

            void set_safe_dirs(bool safe) {safe_dirs = safe;}
            bool is_safe_dirs() const {return safe_dirs;}

        protected:
            fs::path path() const {
                fs::path result(path_root);
                for (const std::string &dir: directories)
                    result /= dir;
                return result;
            }

            void begin_() {stream.precision(CPPDATALIB_REAL_DIG);}

            // Keys are file or directory names
            void begin_key_(const core::value &v)
            {
                if (!v.is_string() && !v.is_int() && !v.is_uint() && !v.is_real())
                    throw core::error("filesystem - cannot write non-string, non-numeric key");
                scalar_is_key = true;
            }
            void end_key_(const core::value &)
            {
                if (key == "." ||
                    key == ".." ||
                    key.find('/') != key.npos ||
                    key.find(fs::path::preferred_separator) != key.npos)
                    throw core::custom_error("filesystem - invalid filename, cannot open \"" + path().native() + "\" for writing");
                scalar_is_key = false;
            }

            // Items are either files or lists of directory items
            void begin_item_(const core::value &v)
            {
                if (current_container() == core::array)
                    key = std::to_string(current_container_size());

                if (!v.is_array() && !v.is_object())
                {
                    if (safe_write && fs::exists(path() / key))
                        throw core::custom_error("filesystem - file \"" + (path() / key).native() + "\" already exists");

                    stream.open(path() / key);
                    if (!stream)
                        throw core::custom_error("filesystem - error when opening \"" + (path() / key).native() + "\" for writing");
                }
            }
            void end_item_(const core::value &v)
            {
                if (!v.is_array() && !v.is_object())
                    stream.close();
            }

            void bool_(const core::value &v) {stream << (v.get_bool_unchecked()? "true": "false");}
            void integer_(const core::value &v)
            {
                if (scalar_is_key)
                    key = std::to_string(v.get_int_unchecked());
                else
                    stream << v.get_int_unchecked();
            }
            void uinteger_(const core::value &v)
            {
                if (scalar_is_key)
                    key = std::to_string(v.get_uint_unchecked());
                else
                    stream << v.get_uint_unchecked();
            }
            void real_(const core::value &v)
            {
                if (!std::isfinite(v.get_real_unchecked()))
                    throw core::error("filesystem - cannot write 'NaN' or 'Infinity' values");

                if (scalar_is_key)
                    key = std::to_string(v.get_real_unchecked());
                else
                    stream << v.get_real_unchecked();
            }
            void begin_string_(const core::value &, core::int_t, bool is_key)
            {
                if (is_key)
                    key.clear();
            }
            void string_data_(const core::value &v, bool is_key)
            {
                if (is_key)
                    key += v.get_string_unchecked();
                else
                    stream << v.get_string_unchecked();
            }

            void begin_array_(const core::value &, core::int_t, bool)
            {
                if (nesting_depth() > 0)
                {
                    if (fs::status(path() / key).type() == fs::file_type::directory)
                    {
                        if (safe_dirs)
                            throw core::custom_error("filesystem - directory \"" + (path() / key).native() + "\" already exists");
                    }
                    else if (!fs::create_directory(path() / key))
                        throw core::custom_error("filesystem - unable to create directory \"" + (path() / key).native() + "\"");

                    directories.push_back(key);
                }
            }
            void end_array_(const core::value &, bool)
            {
                if (nesting_depth() > 1)
                    directories.pop_back();
            }

            void begin_object_(const core::value &, core::int_t, bool)
            {
                if (nesting_depth() > 0)
                {
                    if (fs::status(path() / key).type() == fs::file_type::directory)
                    {
                        if (safe_dirs)
                            throw core::custom_error("filesystem - directory \"" + (path() / key).native() + "\" already exists");
                    }
                    else if (!fs::create_directory(path() / key))
                        throw core::custom_error("filesystem - unable to create directory \"" + (path() / key).native() + "\"");

                    directories.push_back(key);
                }
            }
            void end_object_(const core::value &, bool)
            {
                if (nesting_depth() > 1)
                    directories.pop_back();
            }
        };
    }
}

#endif

#endif // FILESYSTEM_H
