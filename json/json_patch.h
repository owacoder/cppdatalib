/*
 * json_patch.h
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

#ifndef CPPDATALIB_JSON_PATCH_H
#define CPPDATALIB_JSON_PATCH_H

#include "../core/core.h"
#include "../json/json.h"

namespace cppdatalib
{
    namespace json
    {
        namespace patch
        {
            inline core::value diff(const core::value &from, const core::value &to)
            {
                (void) from, (void) to;

                struct traverse_diff
                {
                    core::value diff;

                    std::string escape_path(const std::string &key)
                    {
                        std::string result;

                        for (auto c: key)
                        {
                            if (c == '~')
                                result += "~0";
                            else if (c == '/')
                                result += "~1";
                            else
                                result.push_back(c);
                        }

                        return result;
                    }

                    std::string make_path(core::value::traversal_ancestry_finder finder)
                    {
                        std::string path;
                        auto ancestors = finder.get_ancestry();
                        for (size_t i = ancestors.size(); i;)
                        {
                            core::value::traversal_reference ref = ancestors[--i];
                            if (ref.is_array())
                                path += "/" + std::to_string(ref.get_array_index());
                            else if (ref.is_object())
                            {
                                if (ref.is_object_key())
                                    throw core::error("cppdatalib::core::json::patch - path references key, not value");
                                else if (!ref.get_object_key()->is_string())
                                    throw core::error("cppdatalib::core::json::patch - path contains key that is not a string");

                                path += "/" + escape_path(ref.get_object_key()->as_string());
                            }
                            else
                                throw core::error("cppdatalib::core::json::patch - path contains non-container");
                        }
                        return path;
                    }

                public:
                    traverse_diff() : diff(core::array_t()) {}

                    const core::value &get_diff() const {return diff;}

                    bool operator()(const core::value *arg,
                                    const core::value *arg2,
                                    core::value::traversal_ancestry_finder arg_ancestry,
                                    core::value::traversal_ancestry_finder arg2_ancestry)
                    {
                        core::value change;

                        std::cout << (arg? to_json(*arg): "NULL") << " : " << (arg2? to_json(*arg2): "NULL") << std::endl << std::endl;

                        if ((arg_ancestry.get_parent_count() > 0 && arg_ancestry.get_ancestry().front().is_object_key()) ||
                            (arg2_ancestry.get_parent_count() > 0 && arg2_ancestry.get_ancestry().front().is_object_key()))
                            return true;

                        if (arg == NULL && arg2 != NULL)
                        {
                            change["op"] = core::value("add");
                            change["path"] = core::value(make_path(arg2_ancestry));
                            change["value"] = *arg2;
                        }
                        else if (arg != NULL && arg2 == NULL)
                        {
                            change["op"] = core::value("remove");
                            change["path"] = core::value(make_path(arg_ancestry));
                        }
                        else if (arg != NULL && arg2 != NULL &&
                                 !arg->is_object() && !arg->is_array() &&
                                 !arg2->is_object() && !arg2->is_array() && *arg != *arg2)
                        {
                            change["op"] = core::value("replace");
                            change["path"] = core::value(make_path(arg_ancestry));
                            change["value"] = *arg2;
                        }

                        if (change.is_object())
                            diff.push_back(change);

                        return true;
                    }
                };

                struct traverse_diff_postfix
                {
                public:
                    bool operator()(const core::value *arg,
                                    const core::value *arg2,
                                    core::value::traversal_ancestry_finder,
                                    core::value::traversal_ancestry_finder)
                    {
                        std::cout << "POSTFIX: " << (arg? to_json(*arg): "NULL") << " : " << (arg2? to_json(*arg2): "NULL") << std::endl << std::endl;

                        return true;
                    }
                };

                traverse_diff traverser;
                //traverse_diff_postfix postfix;

                //from.parallel_diff_traverse(to, traverser, postfix);

                return traverser.get_diff();
            }
        }
    }
}

#endif // CPPDATALIB_JSON_PATCH_H
