/*
 * cpp11.h
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

#ifndef CPP11_H
#define CPP11_H

#include "global.h"

#ifndef CPPDATALIB_CPP11
#include <iostream>

namespace stdx
{
    enum memory_order
    {
        memory_order_relaxed,
        memory_order_consume,
        memory_order_acquire,
        memory_order_release,
        memory_order_acq_rel,
        memory_order_seq_cst
    };

    // TODO: The atomic class is not currently thread-safe (or atomic :P). Use caution when invoking an instance of this class.
    template<typename T>
    class atomic
    {
        volatile T value;

        atomic(const atomic &) {}
        void operator=(const atomic &) {}

    public:
        atomic() : value(0) {}

        atomic &operator=(T value) {store(value); return *this;}

        void store(T value, memory_order order = memory_order_seq_cst) {this->value = value; (void) order;}
        T load(memory_order order = memory_order_seq_cst) const {(void) order; return this->value;}

        T exchange(T desired, memory_order order = memory_order_seq_cst)
        {
            (void) order;
            T temp = value;
            value = desired;
            return temp;
        }

        T fetch_add(T val, memory_order order = memory_order_seq_cst)
        {
            (void) order;
            T temp = value;
            value += val;
            return temp;
        }

        T fetch_sub(T val, memory_order order = memory_order_seq_cst)
        {
            (void) order;
            T temp = value;
            value -= val;
            return temp;
        }

        T fetch_and(T val, memory_order order = memory_order_seq_cst)
        {
            (void) order;
            T temp = value;
            value &= val;
            return temp;
        }

        T fetch_or(T val, memory_order order = memory_order_seq_cst)
        {
            (void) order;
            T temp = value;
            value |= val;
            return temp;
        }

        T fetch_xor(T val, memory_order order = memory_order_seq_cst)
        {
            (void) order;
            T temp = value;
            value ^= val;
            return temp;
        }

        T operator++() {return fetch_add(1)+1;}
        T operator++(int) {return fetch_add(1);}
        T operator--() {return fetch_sub(1)-1;}
        T operator--(int) {return fetch_sub(1);}

        T operator+=(T val) {return fetch_add(val) + val;}
        T operator-=(T val) {return fetch_sub(val) - val;}
        T operator&=(T val) {return fetch_and(val) & val;}
        T operator|=(T val) {return fetch_or(val) | val;}
        T operator^=(T val) {return fetch_xor(val) ^ val;}

        operator T() const {return load();}
    };

    // TODO: This is a thread unsafe version of shared_ptr. Make thread safe?
    template<typename T>
    class unique_ptr
    {
    public:
        unique_ptr() : d_(NULL) {}

        // Take ownership of pointer
        explicit unique_ptr(T *ptr) : d_(ptr) {}
        template<typename Y>
        unique_ptr(Y *ptr) : d_(ptr) {}

        // Copy ownership of pointer
        unique_ptr(const unique_ptr<T> &other) : d_(new T(*other.get())) {}
        template<typename Y>
        unique_ptr(const unique_ptr<Y> &other) : d_(new T(*other.get())) {}

        // Delete pointer as needed
        ~unique_ptr() {delete d_;}

        unique_ptr &operator=(const unique_ptr<T> &other)
        {
            unique_ptr<T> newPtr(other);
            swap(newPtr);
            return *this;
        }

        template<typename Y>
        void reset(Y *ptr = NULL)
        {
            unique_ptr<T> newPtr(ptr);
            swap(newPtr);
        }
        void swap(unique_ptr<T> &other)
        {
            T *temp = other.d_;
            other.d_ = d_;
            d_ = temp;
        }

        T *get() const {return d_;}

        T &operator*() {return *d_;}
        const T &operator*() const {return *d_;}
        T *operator->() {return d_;}
        const T *operator->() const {return d_;}

        operator bool() const {return d_;}

    private:
        T *d_;
    };

    namespace impl
    {
        struct shared_ptr_data
        {
            shared_ptr_data(void *item) : item(item) {}

            void *item;
            atomic<unsigned long> count; // Contains <number of shared_ptrs>-1 (i.e. if one shared_ptr owns the object, 0 is stored here)
        };

        struct shared_ptr_extractor;
    }

    // TODO: This is a thread unsafe version of shared_ptr. Make thread safe?
    template<typename T>
    class shared_ptr
    {
        friend class shared_ptr_extractor;

    public:
        shared_ptr() : d_(NULL) {}

        // Take ownership of pointer
        explicit shared_ptr(T *ptr) : d_(ptr? new impl::shared_ptr_data(ptr): NULL) {}
        template<typename Y>
        shared_ptr(Y *ptr) : d_(ptr? new impl::shared_ptr_data(ptr): NULL) {}

        // Copy ownership of pointer
        shared_ptr(const shared_ptr<T> &other) : d_(other.d_) {if (d_) ++(d_->count);}
        template<typename Y>
        shared_ptr(const shared_ptr<Y> &other) : d_(other.internal_())
        {
            // Verify that the types are compatible
            static_cast<T*>(other.get());

            if (d_)
                ++(d_->count);
        }

        // Delete pointer as needed
        ~shared_ptr()
        {
            if (d_ && d_->count-- == 0)
            {
                delete get();
                delete d_; d_ = NULL;
            }
        }

        long use_count() const {return d_? d_->count + 1: 0;}

        shared_ptr &operator=(const shared_ptr<T> &other)
        {
            shared_ptr<T> newPtr(other);
            swap(newPtr);
            return *this;
        }

        template<typename Y>
        void reset(Y *ptr = NULL)
        {
            shared_ptr<T> newPtr(ptr);
            swap(newPtr);
        }
        void swap(shared_ptr<T> &other)
        {
            impl::shared_ptr_data *temp = other.d_;
            other.d_ = d_;
            d_ = temp;
        }

        T *get() const {return d_? static_cast<T*>(d_->item): NULL;}

        T &operator*() {return *static_cast<T*>(d_->item);}
        const T &operator*() const {return *static_cast<T*>(d_->item);}
        T *operator->() {return d_->item;}
        const T *operator->() const {return d_->item;}

        operator bool() const {return d_ && d_->item;}

        impl::shared_ptr_data *internal_() const {return d_;}

    private:
        impl::shared_ptr_data *d_;
    };

    template<typename T>
    shared_ptr<T> make_shared() {return shared_ptr<T>(new T());}
    template<typename T, typename Arg1>
    shared_ptr<T> make_shared(Arg1 arg1) {return shared_ptr<T>(new T(arg1));}
    template<typename T, typename Arg1, typename Arg2>
    shared_ptr<T> make_shared(Arg1 arg1, Arg2 arg2) {return shared_ptr<T>(new T(arg1, arg2));}
    template<typename T, typename Arg1, typename Arg2, typename Arg3>
    shared_ptr<T> make_shared(Arg1 arg1, Arg2 arg2, Arg3 arg3) {return shared_ptr<T>(new T(arg1, arg2, arg3));}
    template<typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4>
    shared_ptr<T> make_shared(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4) {return shared_ptr<T>(new T(arg1, arg2, arg3, arg4));}
    template<typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5>
    shared_ptr<T> make_shared(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5) {return shared_ptr<T>(new T(arg1, arg2, arg3, arg4, arg5));}
    template<typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6>
    shared_ptr<T> make_shared(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6) {return shared_ptr<T>(new T(arg1, arg2, arg3, arg4, arg5, arg6));}
    template<typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7>
    shared_ptr<T> make_shared(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7) {return shared_ptr<T>(new T(arg1, arg2, arg3, arg4, arg5, arg6, arg7));}
    template<typename T, typename Arg1, typename Arg2, typename Arg3, typename Arg4, typename Arg5, typename Arg6, typename Arg7, typename Arg8>
    shared_ptr<T> make_shared(Arg1 arg1, Arg2 arg2, Arg3 arg3, Arg4 arg4, Arg5 arg5, Arg6 arg6, Arg7 arg7, Arg8 arg8) {return shared_ptr<T>(new T(arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8));}

    template<typename T, T v>
    struct integral_constant
    {
        typedef T value_type;
        typedef integral_constant<T, v> type;

        static const T value = v;

        operator value_type() const {return v;}
    };

    struct true_type : public integral_constant<bool, true> {};
    struct false_type : public integral_constant<bool, false> {};

    template<typename T, typename U> struct is_same : public false_type {};
    template<typename T> struct is_same<T, T> : public true_type {};

    template<typename T> struct is_floating_point : public integral_constant<bool,
            is_same<T, float>::value ||
            is_same<T, double>::value ||
            is_same<T, long double>::value> {};
    template<typename T> struct is_integral : public integral_constant<bool,
            is_same<T, bool>::value ||
            is_same<T, signed char>::value ||
            is_same<T, unsigned char>::value ||
            is_same<T, char>::value ||
            is_same<T, signed int>::value ||
            is_same<T, unsigned int>::value ||
            is_same<T, signed long>::value ||
            is_same<T, unsigned long>::value ||
            is_same<T, signed long long>::value ||
            is_same<T, unsigned long long>::value ||
            is_same<T, char16_t>::value ||
            is_same<T, char32_t>::value ||
            is_same<T, wchar_t>::value> {};
    template<typename T> struct is_arithmetic : public integral_constant<bool, is_integral<T>::value || is_floating_point<T>::value> {};

    template<bool pred, typename T> struct enable_if {};
    template<typename T> struct enable_if<true, T> {typedef T type;};

    template<typename T> struct is_class : public integral_constant<bool, !is_arithmetic<T>::value> {};

    template<typename T> struct is_unsigned : public integral_constant<bool, T(0) < T(-1)> {};
    template<typename T> struct is_signed : public integral_constant<bool, T(-1) < T(0)> {};

    template<typename T> struct remove_reference {typedef T type;};
    template<typename T> struct remove_reference<T &> {typedef T type;};

    template<typename T> struct remove_const {typedef T type;};
    template<typename T> struct remove_const<const T> {typedef T type;};

    template<typename T> struct remove_volatile {typedef T type;};
    template<typename T> struct remove_volatile<volatile T> {typedef T type;};

    template<typename T> struct remove_cv {typedef typename remove_const<typename remove_volatile<T>::type>::type type;};

    typedef int64_t streamsize;

    template<typename T> const T &move(const T &r) {return r;}

    template<typename T> std::string to_string(T number)
    {
#ifdef CPPDATALIB_WATCOM
        std::ostrstream stream;
#else
        std::ostringstream stream;
#endif
        stream.precision(CPPDATALIB_REAL_DIG);
        stream << number;
        return stream.str();
    }

    template<typename T> T *addressof(T &item) {return &item;}

    template<typename T> T abs(T n) {return n < 0? -n: n;}

    struct skipws_t
    {
        bool b_;

    public:
        skipws_t(bool b = true) : b_(b) {}
        operator bool() const {return b_;}
    };

    skipws_t skipws() {return skipws_t(true);}
    skipws_t noskipws() {return skipws_t(false);}
}

#define nullptr NULL
#define CPPDATALIB_DISABLE_TEMP_STRING
#define CPPDATALIB_OPTIMIZE_FOR_NUMERIC_SPACE
#else
#include <atomic>
#include <memory>

namespace stdx
{
    inline int abs(int n) {return std::abs(n);}
    inline long abs(long n) {return std::abs(n);}
    inline long long abs(long long n) {return std::abs(n);}

    template<typename T> using atomic = std::atomic<T>;
    template<typename... T> using unique_ptr = std::unique_ptr<T...>;
    template<typename T> using shared_ptr = std::shared_ptr<T>;
    template<typename RetType, typename... Args>
    auto make_shared(Args&&... args) -> decltype(std::make_shared<RetType>(std::forward<Args>(args)...)) {return std::make_shared<RetType>(std::forward<Args>(args)...);}

    template<typename T, T v> using integral_constant = std::integral_constant<T, v>;
    template<typename T, typename U> using is_same = std::is_same<T, U>;
    template<typename T> using is_floating_point = std::is_floating_point<T>;
    template<typename T> using is_unsigned = std::is_unsigned<T>;
    template<typename T> using is_signed = std::is_signed<T>;
    template<typename T> using is_integral = std::is_integral<T>;
    template<typename T> using is_arithmetic = std::is_arithmetic<T>;
    template<typename T> using is_class = std::is_class<T>;
    template<bool b, typename T> using enable_if = std::enable_if<b, T>;

    template<typename T> using remove_reference = std::remove_reference<T>;
    template<typename T> using remove_const = std::remove_const<T>;
    template<typename T> using remove_volatile = std::remove_volatile<T>;
    template<typename T> using remove_cv = std::remove_cv<T>;
    template<typename T> typename remove_reference<T>::type &&move(T &&r) {return static_cast<typename remove_reference<T>::type &&>(r);}

    using streamsize = std::streamsize;

    template<typename... Args>
    auto to_string(Args&&... args) -> decltype(std::to_string(std::forward<Args>(args)...)) {return std::to_string(std::forward<Args>(args)...);}

    template<typename... Args>
    auto addressof(Args&&... args) -> decltype(std::addressof(std::forward<Args>(args)...)) {return std::addressof(std::forward<Args>(args)...);}

    template<typename... Args>
    auto skipws(Args&&... args) -> decltype(std::skipws(std::forward<Args>(args)...)) {return std::skipws(std::forward<Args>(args)...);}

    template<typename... Args>
    auto noskipws(Args&&... args) -> decltype(std::noskipws(std::forward<Args>(args)...)) {return std::noskipws(std::forward<Args>(args)...);}
}
#endif

#endif // CPP11_H
