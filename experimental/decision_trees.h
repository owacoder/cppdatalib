/*
 * decision_trees.h
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

#ifndef CPPDATALIB_DECISION_TREES_H
#define CPPDATALIB_DECISION_TREES_H

#include "../core/core.h"

namespace cppdatalib
{
    namespace experimental
    {
        namespace impl
        {
            template<typename Float>
            Float igain(Float positive, Float negative)
            {
                Float sum = positive + negative;
                Float temp_p = positive / sum;
                Float temp_n = negative / sum;

                if (temp_p != 0)
                    temp_p *= log2(temp_p);

                if (temp_n != 0)
                    temp_n *= log2(temp_n);

                return -temp_p - temp_n;
            }

            // stats: object, keys are distinct column names, values are {"entropy", "gain", "values"} tuples
            // gain: gain of entire dataset
            // array: an array of objects
            // columns: a list of keys that each object in array contains
            // results: a list of boolean classifiers, with the same length as array
            void build_stats(core::value &stats, double gain, const core::array_t &array, const core::array_t &columns, const core::array_t &results)
            {
                for (const auto &column: columns)
                {
                    core::value column_values = core::object_t(); // object, keys are distinct column values, values are {"positive", "negative"} tuples
                    for (size_t idx = 0; idx < array.size(); ++idx)
                    {
                        core::value &tuple = column_values.member(array.data()[idx].member(column));
                        if (results.data()[idx].as_bool())
                            tuple["positive"].get_uint_ref() += 1;
                        else
                            tuple["negative"].get_uint_ref() += 1;
                    }

                    double item_entropy = 0.0, item_gain = 0.0;
                    for (auto &item: column_values.get_object_unchecked())
                    {
                        double positive = item.second["positive"];
                        double negative = item.second["negative"];

                        item_entropy += ((positive + negative) / array.size()) * igain(positive, negative);
                    }
                    item_gain = gain - item_entropy;

                    stats.member(column) = core::object_t{{"entropy", item_entropy}, {"gain", item_gain}, {"values", column_values}};
                }
            }

            void make_tree(core::value &tree, double gain, core::array_t array, core::array_t columns, core::array_t results)
            {
                core::value stats;

                if (array.size() == 0)
                    return;

                // Build statistics for this (sub-)dataset
                build_stats(stats, gain, array, columns, results);

                // Find highest column gain in dataset
                size_t max_gain_idx = 0;
                for (size_t j = 1; j < columns.size(); ++j)
                    if (stats.member(columns.data()[j])["gain"].as_real() > stats.member(columns.data()[max_gain_idx])["gain"].as_real())
                        max_gain_idx = j;

                // Then build the tree for that column
                core::value &tuple = tree.member(columns.data()[max_gain_idx]);
                core::value values = stats.member(columns.data()[max_gain_idx])["values"];

                if (values.is_object())
                {
                    for (const auto &value: values.get_object_unchecked())
                    {
                        if (value.second["positive"].as_uint() == 0 ||
                            value.second["negative"].as_uint() == 0)
                            tuple.member(value.first) = value.second["positive"].as_uint() != 0;
                        else
                        {
                            core::value columns_copy = columns;
                            core::value array_copy = core::array_t();
                            core::value results_copy = core::array_t();
                            size_t idx = 0;

                            for (const auto &item: array)
                            {
                                if (item.member(columns.data()[max_gain_idx]) == value.first)
                                {
                                    array_copy.push_back(item);
                                    results_copy.push_back(results.data()[idx]);
                                }

                                ++idx;
                            }
                            columns_copy.erase_element(max_gain_idx);

                            make_tree(tuple.member(value.first),
                                      gain,
                                      array_copy.get_array_unchecked(),
                                      columns_copy.get_array_unchecked(),
                                      results_copy.get_array_unchecked());
                        }
                    }
                }
                else
                    throw core::error("cppdatalib::experimental::impl::make_tree - invalid stats provided, cannot make tree");
            }

            core::value make_tree(double gain, const core::array_t &array, const core::array_t &columns, const core::array_t &results)
            {
                core::value tree;
                make_tree(tree, gain, array, columns, results);
                return tree;
            }
        }

        // array is an array of objects
        // columns is a list of keys that each object in array contains
        // results is a list of boolean classifiers, with the same length as array
        //
        // result is an object. Keys in the result object alternate between column names and values. (i.e. root = object containing column name,
        // next level is object containing column values, next level is either bool (signifying end of parse) or object containing column name, etc.)
        // Leaves, or terminal values, are either true or false
        core::value make_decision_tree(const core::array_t &array, const core::array_t &columns, const core::array_t &results)
        {
            if (array.size() != results.size())
                throw core::error("cppdatalib::experimental::make_decision_tree - sample and result arrays are not the same size");

            // First obtain positive and negative results
            double positive = 0.0, negative = 0.0;

            for (const auto &item: results)
            {
                if (item.as_bool())
                    positive += 1.0;
                else
                    negative += 1.0;
            }

            // Then make the tree
            return impl::make_tree(impl::igain(positive, negative), array, columns, results);
        }

        // Tests a tuple based on a previously made decision tree (i.e. a tree returned from make_decision_tree())
        // The tuple should be based on the same fields the tree was made with
        // The value returned is either true, false, or null
        core::value test_decision_tree(const core::value &tree, const core::value &test_tuple)
        {
            if (!test_tuple.is_object())
                throw core::error("cppdatalib::experimental::test_decision_tree - cannot test non-object type, must be an object tuple");

            if (tree.is_object())
            {
                // ref is the pointer to the field we're testing in the tuple
                const core::value *ref = test_tuple.member_ptr(tree.get_object_unchecked().begin()->first);
                if (!ref)
                    return core::null_t(); // Unknown result, since it doesn't have the column specified in the tree

                // values is the set of possible values that the tuple could have in the decision tree
                const core::value &values = tree.get_object_unchecked().begin()->second;

                // If tuple field is not found in the tree, we don't know. We haven't seen this value before
                const core::value *member = values.member_ptr(*ref);
                if (!member)
                    return core::null_t();

                return test_decision_tree(*member, test_tuple);
            }
            else if (tree.is_bool() || tree.is_null())
                return tree;
            else
                throw core::error("cppdatalib::experimental::test_decision_tree - invalid decision tree passed as parameter");
        }
    }
}

#endif // CPPDATALIB_DECISION_TREES_H
