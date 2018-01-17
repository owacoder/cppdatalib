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
#include <cmath>

namespace cppdatalib
{
    namespace experimental
    {
        // euclidean(): returns the euclidean distance between two points in N-dimensional space
        // The number of dimensions is specified by the number of entries in lhs and rhs
        //
        // Iterators must satisfy the forward-iterator requirements
        // Distance is calculated by subtraction of iterator values
        template<typename R, template<typename...> class List, typename... Args>
        R euclidean(const List<Args...> &lhs, const List<Args...> &rhs)
        {
            using namespace std;

            R result = 0;
            size_t size = std::min(lhs.size(), rhs.size());

            auto lIt = lhs.begin();
            auto rIt = rhs.begin();

            for (size_t idx = 0; idx < size; ++idx, ++lIt, ++rIt)
            {
                R temp = *lIt - *rIt;
                result += temp * temp;
            }

            return sqrt(result);
        }

        // euclidean(): returns the euclidean distance between two points in N-dimensional space
        // The number of dimensions is specified by the number of entries in lhs and rhs
        //
        // Iterators must satisfy the forward-iterator requirements
        // Distance should be a functor that takes two iterator values and returns the distance between them
        template<typename R, typename Distance, template<typename...> class List, typename... Args>
        R euclidean(const List<Args...> &lhs, const List<Args...> &rhs, Distance distance)
        {
            using namespace std;

            R result = 0;
            size_t size = std::min(lhs.size(), rhs.size());

            auto lIt = lhs.begin();
            auto rIt = rhs.begin();

            for (size_t idx = 0; idx < size; ++idx, ++lIt, ++rIt)
            {
                R temp = distance(*lIt, *rIt);
                result += temp * temp;
            }

            return sqrt(result);
        }

        // hamming(): returns the Hamming distance of two sequences
        // The Hamming distance is equal to the number of unequal values in two sequences of the same length
        //
        // Iterators must satisfy the forward-iterator requirements
        template<typename Iterator>
        size_t hamming(Iterator lhs, Iterator rhs, size_t size)
        {
            size_t distance = 0;

            while (size-- > 0)
                distance += *lhs++ != *rhs++;

            return distance;
        }

        // levenshtein(): returns the Levenshtein distance of two sequences (doesn't have to be strings)
        // The Levenshtein distance calculates the minimum number of deletions, insertions, or substitutions
        // required to transform one sequence to another
        //
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

                v0.swap(v1);
            }

            result = v0[rhs_size];

            return result;
        }

        // insertion_sort(): simple insertion sort
        //
        // Iterator must be bidirectional, but does not need to be random-access
        // Elements must be less-than comparable and have swap(element, element) defined
        template<typename Iterator>
        void insertion_sort(Iterator begin, Iterator end)
        {
            using namespace std;
            auto i = begin;

            if (begin == end)
                return;

            for (++i; i != end; ++i) {
                auto j = i;
                auto j_1 = j; // To be set to j - 1 later

                while (j != begin) {
                    --j_1;
                    if (!(*j_1 < *j))
                        break;

                    swap(*j_1, *j);
                    --j;
                }
            }
        }

        // insertion_sort(): simple insertion sort
        //
        // Iterator must be bidirectional, but does not need to be random-access
        // Elements must have swap(element, element) defined
        // `compare` is used to sort elements
        template<typename Iterator, typename Predicate>
        void insertion_sort(Iterator begin, Iterator end, Predicate compare)
        {
            using namespace std;
            auto i = begin;

            if (begin == end)
                return;

            for (++i; i != end; ++i) {
                auto j = i;
                auto j_1 = j; // To be set to j - 1 later

                while (j != begin) {
                    --j_1;
                    if (!compare(*j_1, *j))
                        break;

                    swap(*j_1, *j);
                    --j;
                }
            }
        }

        // selection_sort(): simple selection sort
        //
        // Iterator must be bidirectional, but does not need to be random-access
        // Elements must have swap(element, element) defined
        // `compare` is used to sort elements
        template<typename Iterator, typename Predicate>
        void selection_sort(Iterator begin, Iterator end, Predicate compare)
        {
            using namespace std;
            auto i = begin;
            auto last = end;

            if (begin == end)
                return;

            for (--last; i != last; ++i) {
                auto least = i;
                auto j = least;

                for (++j; j != end; ++j) {
                    if (compare(*j, *least))
                        least = j;
                }

                if (i != least)
                    swap(*i, *least);
            }
        }

        // n_selection_sort(): simple selection sort, only sorting the first min(N, end-begin) elements (if N is zero, nothing is sorted)
        //
        // Iterator must be bidirectional, but does not need to be random-access
        // Elements must have swap(element, element) defined
        // `compare` is used to sort elements
        template<typename Iterator, typename Predicate>
        void n_selection_sort(Iterator begin, Iterator end, Predicate compare, size_t N)
        {
            using namespace std;
            auto i = begin;
            auto last = begin;

            if (begin == last)
                return;

            ++last;
            for (size_t copy = N; copy != 0 && last != end; --copy)
                ++last;

            for (--last; i != last; ++i) {
                auto least = i;
                auto j = least;

                for (++j; j != end; ++j) {
                    if (compare(*j, *least))
                        least = j;
                }

                if (i != least)
                    swap(*i, *least);
            }
        }

        // n_selection_sort(): simple selection sort
        //
        // Iterator must be bidirectional, but does not need to be random-access
        // Elements be less-than comparable and must have swap(element, element) defined
        template<typename Iterator>
        void selection_sort(Iterator begin, Iterator end)
        {
            std::less<decltype(*begin)> less;
            selection_sort<Iterator, decltype(less)>(begin, end, less);
        }

        // n_selection_sort(): simple selection sort, only sorting the first min(N, end-begin) elements (if N is zero, nothing is sorted)
        //
        // Iterator must be bidirectional, but does not need to be random-access
        // Elements be less-than comparable and must have swap(element, element) defined
        template<typename Iterator>
        void n_selection_sort(Iterator begin, Iterator end, size_t N)
        {
            std::less<decltype(*begin)> less;
            n_selection_sort<Iterator, decltype(less)>(begin, end, less, N);
        }
    }
}

#endif // CPPDATALIB_ALGORITHM_H
