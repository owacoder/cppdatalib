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
                core::uint_t pos = 0, neg = 0;
                for (size_t i = 0; i < results.size(); ++i)
                    if (results[i].as_bool())
                        ++pos;
                    else
                        ++neg;

                stats["positive"] = cppdatalib::core::value(pos);
                stats["negative"] = cppdatalib::core::value(neg);

                for (const auto &column: columns)
                {
                    core::value column_values = cppdatalib::core::value(core::object_t()); // object, keys are distinct column values, values are {"positive", "negative"} tuples
                    for (size_t idx = 0; idx < array.size(); ++idx)
                    {
                        core::value &tuple = column_values.member(array[idx].member(column));
                        if (results[idx].as_bool())
                            tuple["positive"].get_uint_ref() += 1;
                        else
                            tuple["negative"].get_uint_ref() += 1;
                    }

                    double item_entropy = 0.0, item_gain = 0.0;
                    for (auto &item: column_values.get_object_unchecked())
                    {
                        double positive = item.second["positive"].as_real();
                        double negative = item.second["negative"].as_real();

                        item_entropy += ((positive + negative) / array.size()) * igain(positive, negative);
                    }
                    item_gain = gain - item_entropy;

                    stats["stats"].member(column) = cppdatalib::core::value(core::object_t{
                                                                                {cppdatalib::core::value("entropy"), cppdatalib::core::value(item_entropy)},
                                                                                {cppdatalib::core::value("gain"), cppdatalib::core::value(item_gain)},
                                                                                {cppdatalib::core::value("values"), cppdatalib::core::value(column_values)}});
                }
            }

            // gain is the gain for the master dataset
            // array is an array of objects
            // columns is a list of keys that each object in array contains
            // results is a list of boolean classifiers, with the same length as array
            //
            // result is an object. Keys in the result object alternate between column names and values. (i.e. root = object containing column name,
            // next level is object containing column values, next level is either bool (signifying end of parse) or object containing column name, etc.)
            // Leaves, or terminal values, are either true or false
            void make_tree(core::value &tree, double gain, size_t dataset_size, core::array_t array, core::array_t columns, core::array_t results)
            {
                core::value stats;

                if (array.size() == 0)
                    return;

                // Build statistics for this (sub-)dataset
                build_stats(stats, gain, array, columns, results);

                core::value &metrics = stats["stats"];
                // Find highest column gain in dataset
                size_t max_gain_idx = 0;
                for (size_t j = 1; j < columns.size(); ++j)
                    if (metrics.member(columns[j])["gain"].as_real() > metrics.member(columns[max_gain_idx])["gain"].as_real())
                        max_gain_idx = j;

                // Then build the tree for that column
                core::value &column_stats = metrics.member(columns[max_gain_idx]);
                core::value &parent_tuple = tree.member(columns[max_gain_idx]);
                core::value &tuple = parent_tuple["node"];
                core::value &values = column_stats["values"];
                parent_tuple["probability"] = cppdatalib::core::value(stats["positive"].as_real() / array.size());

                if (values.is_object())
                {
                    for (const auto &value: values.get_object_unchecked())
                    {
                        if (value.second["positive"].as_uint() == 0 ||
                            value.second["negative"].as_uint() == 0)
                            tuple.member(value.first) = cppdatalib::core::value(value.second["positive"].as_uint() != 0);
                        else
                        {
                            core::value columns_copy = cppdatalib::core::value(columns);
                            core::value array_copy = cppdatalib::core::value(core::array_t());
                            core::value results_copy = cppdatalib::core::value(core::array_t());
                            size_t idx = 0;

                            for (const auto &item: array)
                            {
                                if (item.member(columns[max_gain_idx]) == value.first)
                                {
                                    array_copy.push_back(item);
                                    results_copy.push_back(results[idx]);
                                }

                                ++idx;
                            }
                            columns_copy.erase_element(max_gain_idx);

                            make_tree(tuple.member(value.first),
                                      gain,
                                      dataset_size,
                                      array_copy.get_array_unchecked(),
                                      columns_copy.get_array_unchecked(),
                                      results_copy.get_array_unchecked());
                        }
                    }
                }
                else
                    throw core::error("cppdatalib::experimental::impl::make_tree - invalid stats provided, cannot make tree");
            }

            core::value make_tree(double gain, size_t dataset_size, const core::array_t &array, const core::array_t &columns, const core::array_t &results)
            {
                core::value tree;
                make_tree(tree, gain, dataset_size, array, columns, results);
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
            return impl::make_tree(impl::igain(positive, negative), array.size(), array, columns, results);
        }

        // Tests a tuple based on a previously made decision tree (i.e. a tree returned from make_decision_tree())
        // The tuple should be based on the same fields the tree was made with
        // If probability_result is false, the value returned is either true, false, or null.
        // If probability_result is true, the value returned is either null or a real value between 0 and 1, returning the probability that the tree thinks the correct response is true
        // TODO: probability is based on classification of tree entries, not actual data. Probabilities of actual data should be stored in trees
        core::value test_decision_tree(const core::value &tree, const core::value &test_tuple, bool probability_result = false)
        {
            if (!test_tuple.is_object())
                throw core::error("cppdatalib::experimental::test_decision_tree - cannot test non-object type, must be an object tuple");

            if (tree.object_size() > 0)
            {
                // ref is the pointer to the field we're testing in the tuple
                const core::value *ref = test_tuple.member_ptr(tree.get_object_unchecked().begin()->first);
                if (!ref)
                    return core::null_t(); // Unknown result, since it doesn't have the column specified in the tree

                // node is the pointer to the object containing possible values
                const core::value *node = tree.get_object_unchecked().begin()->second.member_ptr(cppdatalib::core::value("node"));
                if (!node || !node->is_object())
                    throw core::error("cppdatalib::experimental::test_decision_tree - invalid decision tree passed as parameter");

                // If tuple field is not found in the tree, we don't know. We haven't seen this value before. Use probability estimate in tree
                const core::value *member = node->member_ptr(*ref);
                if (!member)
                {
                    const core::value *probability = tree.get_object_unchecked().begin()->second.member_ptr(cppdatalib::core::value("probability"));
                    if (!probability)
                        throw core::error("cppdatalib::experimental::test_decision_tree - invalid decision tree passed as parameter");

                    core::real_t result = probability->as_real();

                    if (probability_result)
                        return cppdatalib::core::value(result);

                    return result < 0.5? core::value(false): result > 0.5? core::value(true): core::value();
                }

                return test_decision_tree(*member, test_tuple, probability_result);
            }
            else if (tree.is_null())
            {
                if (probability_result)
                    return cppdatalib::core::value(0.5);
                return tree;
            }
            else if (tree.is_bool())
            {
                if (probability_result)
                    return cppdatalib::core::value(tree.as_bool()? 1.0: 0.0);
                return tree;
            }
            else
                throw core::error("cppdatalib::experimental::test_decision_tree - invalid decision tree passed as parameter");
        }
    }
}

#endif // CPPDATALIB_DECISION_TREES_H
