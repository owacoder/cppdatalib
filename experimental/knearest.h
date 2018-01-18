/*
 * knearest.h
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

#ifndef CPPDATALIB_KNEAREST_H
#define CPPDATALIB_KNEAREST_H

#include "../core/core.h"
#include "algorithm.h"

namespace cppdatalib
{
    namespace experimental
    {
        template<typename R, typename Distance>
        core::value distance(const core::value &test_tuple, const core::value &dataset_tuple, Distance measure)
        {
            if (test_tuple.is_object() && dataset_tuple.is_object())
            {
                core::value lhs = core::array_t();
                core::value rhs = core::array_t();

                // Join columns (this is to make sure that all columns are accounted for; NULL values are inserted where the value is missing)
                auto test_tuple_it = test_tuple.get_object_unchecked().begin();
                auto data_tuple_it = dataset_tuple.get_object_unchecked().begin();

                for (; test_tuple_it != test_tuple.get_object_unchecked().end() &&
                       data_tuple_it != dataset_tuple.get_object_unchecked().end();)
                {
                    if (test_tuple_it->first < data_tuple_it->first)
                    {
                        lhs.push_back(test_tuple_it->second);
                        rhs.push_back(core::null_t());
                        ++test_tuple_it;
                    }
                    else if (data_tuple_it->first < test_tuple_it->first)
                    {
                        lhs.push_back(core::null_t());
                        rhs.push_back(data_tuple_it->second);
                        ++data_tuple_it;
                    }
                    else
                    {
                        lhs.push_back(test_tuple_it->second);
                        rhs.push_back(data_tuple_it->second);
                        ++test_tuple_it;
                        ++data_tuple_it;
                    }
                }

                while (test_tuple_it != test_tuple.get_object_unchecked().end())
                {
                    lhs.push_back(test_tuple_it->second);
                    rhs.push_back(core::null_t());
                    ++test_tuple_it;
                }

                while (data_tuple_it != dataset_tuple.get_object_unchecked().end())
                {
                    lhs.push_back(core::null_t());
                    rhs.push_back(data_tuple_it->second);
                    ++data_tuple_it;
                }

                return euclidean<R, Distance>(lhs.get_array_unchecked().begin(),
                                              rhs.get_array_unchecked().begin(),
                                              lhs.array_size(),
                                              measure);
            }
            else if (test_tuple.is_array() && dataset_tuple.is_array())
            {
                if (test_tuple.array_size() != dataset_tuple.array_size())
                    throw core::error("cppdatalib::experimental::k_nearest_neighbor - test-sample and training-data arrays are not the same size");

                return euclidean<R, Distance>(test_tuple.get_array_unchecked().begin(),
                                              dataset_tuple.get_array_unchecked().begin(),
                                              test_tuple.array_size(),
                                              measure);
            }
            else
                return static_cast<R>(measure(test_tuple, dataset_tuple));
        }

        template<typename R, typename Distance>
        core::value k_nearest_neighbor_classify(const core::value &test_tuple, const core::array_t &array, const core::array_t &results, size_t k, Distance measure)
        {
            if (array.size() != results.size())
                throw core::error("cppdatalib::experimental::k_nearest_neighbor_classify - sample and result arrays are not the same size");

            // Limit k to dataset size
            if (array.size() < k)
                k = array.size();

            // Generate list of distances, with indexes tied to them
            core::value distances;
            for (size_t idx = 0; idx < array.size(); ++idx)
                distances.push_back(core::object_t{{"distance", distance<R, Distance>(test_tuple, array[idx], measure)}, {"index", idx}});

            auto lambda = [](const core::value &lhs, const core::value &rhs){return lhs["distance"].operator R() < rhs["distance"].operator R();};

            // Sort to find the k closest points
            n_selection_sort(distances.get_array_ref().begin(),
                             distances.get_array_ref().end(),
                             lambda,
                             k);

            // Accumulate probabilities for each class
            core::value result = core::object_t();
            for (size_t idx = 0; idx < k; ++idx)
                result.member(results[distances[idx]["index"].as_uint()]).get_real_ref() += 1.0 / k;

            return result;
        }

        template<typename R, typename Distance, typename Weight>
        core::value k_nearest_neighbor_classify_weighted(const core::value &test_tuple, const core::array_t &array, const core::array_t &results, size_t k, Distance measure, Weight weight)
        {
            using namespace std;

            if (array.size() != results.size())
                throw core::error("cppdatalib::experimental::k_nearest_neighbor_classify_weighted - sample and result arrays are not the same size");

            // Limit k to dataset size
            if (array.size() < k)
                k = array.size();

            // Generate list of distances, with indexes tied to them
            core::value distances;
            for (size_t idx = 0; idx < array.size(); ++idx)
                distances.push_back(core::object_t{{"distance", distance<R, Distance>(test_tuple, array[idx], measure)}, {"index", idx}});

            auto lambda = [](const core::value &lhs, const core::value &rhs){return lhs["distance"].operator R() < rhs["distance"].operator R();};

            // Sort to find the k closest points
            n_selection_sort(distances.get_array_ref().begin(),
                             distances.get_array_ref().end(),
                             lambda,
                             k);

            // Accumulate weights for each class
            core::value result = core::object_t();
            for (size_t idx = 0; idx < k; ++idx)
            {
                R temp_distance = distances[idx]["distance"].operator R();
                result.member(results[distances[idx]["index"].as_uint()]).get_real_ref() += weight(temp_distance);
            }

            std::cout << result << std::endl;

            // Find total weight
            core::real_t total_weight = 0;
            for (const auto &item: result.get_object_unchecked())
                total_weight += item.second.as_real();

            // If total weight is infinite, we have an exact match (or more than one, if we have an unusual weight function)
            // Remove other possibilities from result
            if (isinf(total_weight))
            {
                core::value new_result = core::object_t();

                for (const auto &item: result.get_object_unchecked())
                    if (isinf(item.second.as_real()))
                        new_result.member(item.first) = 1.0;

                return new_result;
            }
            // If total weight is zero, we have no idea what should match (we MUST have an unusual weight function!)
            // Everything is equally likely, so we decay to normal KNN search
            else if (total_weight == 0)
            {
                result = core::object_t();
                for (size_t idx = 0; idx < k; ++idx)
                    result.member(results[distances[idx]["index"].as_uint()]).get_real_ref() += 1.0 / k;
            }
            else
            {
                // Normalize weights to probabilities
                for (auto &item: result.get_object_ref())
                    item.second.get_real_ref() /= total_weight;
            }

            return result;
        }
    }
}

#endif // CPPDATALIB_KNEAREST_H
