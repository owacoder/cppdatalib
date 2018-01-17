/*
 * algorithm.h
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

#ifndef CPPDATALIB_ALGORITHM_H
#define CPPDATALIB_ALGORITHM_H

#include <cstdlib>
#include <memory>
#include <cstring>

namespace cppdatalib
{
    namespace experimental
    {
        // Iterators must satisfy the forward-iterator requirements
        template<typename Iterator>
        size_t hamming(Iterator lhs, Iterator rhs, size_t size)
        {
            size_t distance = 0;

            while (size-- > 0)
                distance += *lhs++ != *rhs++;

            return distance;
        }

        // Code adapted from https://en.wikipedia.org/wiki/Levenshtein_distance
        // Iterators must satisfy the forward-iterator requirements
        template<typename Iterator>
        size_t levenshtein(const Iterator lhs, const Iterator rhs, const size_t lhs_size, const size_t rhs_size)
        {
            std::unique_ptr<size_t []> v0 = std::unique_ptr<size_t []>(new size_t[rhs_size + 1]);
            std::unique_ptr<size_t []> v1 = std::unique_ptr<size_t []>(new size_t[rhs_size + 1]);
            size_t result;

            Iterator i_lhs = lhs, i_rhs;

            for (size_t i = 0; i <= rhs_size; ++i)
                v0[i] = i;

            for (size_t i = 0; i < lhs_size; ++i, ++i_lhs)
            {
                v1[0] = i + 1;

                i_rhs = rhs;
                for (size_t j = 0; j < rhs_size; ++j, ++i_rhs)
                {
                    size_t deletion_cost = v0[j + 1] + 1;
                    size_t insertion_cost = v1[j] + 1;
                    size_t substitute_cost = v0[j] + !(*i_lhs == *i_rhs);

                    v1[j + 1] = std::min(deletion_cost, std::min(insertion_cost, substitute_cost));
                }

                memcpy(v0.get(), v1.get(), (rhs_size + 1) * sizeof(size_t));
            }

            result = v0[rhs_size];

            return result;
        }
    }
}

#endif // CPPDATALIB_ALGORITHM_H
