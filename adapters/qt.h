/*
 * qt.h (Qt is a trademark of the Qt Company)
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
 *
 * Disclaimer:
 * Trademarked product names referred to in this file are the property of their
 * respective owners. These trademark owners are not affiliated with the author
 * or copyright holder(s) of this file in any capacity, and do not endorse this
 * software nor the authorship and existence of this file.
 */

#ifndef CPPDATALIB_QT_H
#define CPPDATALIB_QT_H

#include <QVector>
#include <QList>
#include <QLinkedList>
#include <QStack>
#include <QQueue>
#include <QSet>
#include <QMap>
#include <QMultiMap>
#include <QHash>
#include <QMultiHash>
#include <QStringList>
#include <QByteArrayList>

#include <QString>
#include <QByteArray>
#include <QVariant>

#include <QDateTime>
#include <QUuid>

#include "../core/value_parser.h"

// -------
//  QPair
// -------

template<typename... Ts>
class cast_template_to_cppdatalib<QPair, Ts...>
{
    const QPair<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const QPair<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({cppdatalib::core::value(bind.first), cppdatalib::core::value(bind.second)}, cppdatalib::core::normal);
    }
};

template<typename T1, typename T2>
class cast_template_from_cppdatalib<QPair, T1, T2>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QPair<T1, T2>() const
    {
        QPair<T1, T2> result;
        convert(result);
        return result;
    }
    void convert(QPair<T1, T2> &dest) const
    {
        dest = {bind.element(0).operator T1(), bind.element(1).operator T2()};
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<QPair, Ts...> : public generic_stream_input
    {
    protected:
        const QPair<Ts...> &bind;
        size_t idx;

    public:
        template_parser(const QPair<Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {idx = 0;}

        void write_one_()
        {
            if (was_just_reset())
            {
                get_output()->begin_array(core::array_t(), 2);
                compose_parser(bind.first);
            }
            else if (++idx == 1)
                compose_parser(bind.second);
            else
                get_output()->end_array(core::array_t());
        }
    };
}}

// ------------
//  QByteArray
// ------------

template<>
class cast_to_cppdatalib<QByteArray>
{
    const QByteArray &bind;
public:
    cast_to_cppdatalib(const QByteArray &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_string(bind.toStdString(), cppdatalib::core::blob);
    }
};

template<>
class cast_from_cppdatalib<QByteArray>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QByteArray() const
    {
        QByteArray result;
        convert(result);
        return result;
    }
    void convert(QByteArray &dest)
    {
        dest = QByteArray::fromStdString(bind.as_string());
    }
};

// ---------------
//  QLatin1String
// ---------------

template<>
class cast_to_cppdatalib<QLatin1String>
{
    const QLatin1String &bind;
public:
    cast_to_cppdatalib(const QLatin1String &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest)
    {
        dest = cppdatalib::core::value(bind.data(), bind.size(), cppdatalib::core::clob, true);
    }
};

// -------
//  QChar
// -------

template<>
class cast_to_cppdatalib<QChar>
{
    const QChar &bind;
public:
    cast_to_cppdatalib(const QChar &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_string(QString(bind).toStdString(), cppdatalib::core::normal);
    }
};

// ---------
//  QString
// ---------

template<>
class cast_to_cppdatalib<QString>
{
    const QString &bind;
public:
    cast_to_cppdatalib(const QString &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_string(bind.toStdString(), cppdatalib::core::normal);
    }
};

template<>
class cast_from_cppdatalib<QString>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QString() const
    {
        QString result;
        convert(result);
        return result;
    }
    void convert(QString &dest) const
    {
        dest = QString::fromStdString(bind.as_string());
    }
};

// -------
//  QDate
// -------

template<>
class cast_to_cppdatalib<QDate>
{
    const QDate &bind;
public:
    cast_to_cppdatalib(const QDate &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_string(bind.toString().toStdString(), cppdatalib::core::date);
    }
};

template<>
class cast_from_cppdatalib<QDate>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QDate() const
    {
        QDate result;
        convert(result);
        return result;
    }
    void convert(QDate &dest) const
    {
        dest = QDate::fromString(QString::fromStdString(bind.as_string()));
    }
};

// -------
//  QTime
// -------

template<>
class cast_to_cppdatalib<QTime>
{
    const QTime &bind;
public:
    cast_to_cppdatalib(const QTime &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_string(bind.toString().toStdString(), cppdatalib::core::time);
    }
};

template<>
class cast_from_cppdatalib<QTime>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QTime() const
    {
        QTime result;
        convert(result);
        return result;
    }
    void convert(QTime &dest) const
    {
        dest = QTime::fromString(QString::fromStdString(bind.as_string()));
    }
};

// -----------
//  QDateTime
// -----------

template<>
class cast_to_cppdatalib<QDateTime>
{
    const QDateTime &bind;
public:
    cast_to_cppdatalib(const QDateTime &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_string(bind.toString().toStdString(), cppdatalib::core::datetime);
    }
};

template<>
class cast_from_cppdatalib<QDateTime>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QDateTime() const
    {
        QDateTime result;
        convert(result);
        return result;
    }
    void convert(QDateTime &dest) const
    {
        dest = QDateTime::fromString(QString::fromStdString(bind.as_string()));
    }
};

// -------
//  QUuid
// -------

template<>
class cast_to_cppdatalib<QUuid>
{
    const QUuid &bind;
public:
    cast_to_cppdatalib(const QUuid &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_string(bind.toString().toStdString(), cppdatalib::core::uuid);
    }
};

template<>
class cast_from_cppdatalib<QUuid>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QUuid() const
    {
        QUuid result;
        convert(result);
        return result;
    }
    void convert(QUuid &dest) const
    {
        dest = QUuid(QString::fromStdString(bind.as_string()));
    }
};

// -------------
//  QStringList
// -------------

template<>
class cast_to_cppdatalib<QStringList>
{
    const QStringList &bind;
public:
    cast_to_cppdatalib(const QStringList &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<>
class cast_from_cppdatalib<QStringList>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QStringList() const
    {
        QStringList result;
        convert(result);
        return result;
    }
    void convert(QStringList &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator QString());
    }
};

// ---------
//  QVector
// ---------

template<typename... Ts>
class cast_template_to_cppdatalib<QVector, Ts...>
{
    const QVector<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const QVector<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename T>
class cast_template_from_cppdatalib<QVector, T>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QVector<T>() const
    {
        QVector<T> result;
        convert(result);
        return result;
    }
    void convert(QVector<T> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
    }
};

// -------
//  QList
// -------

template<typename... Ts>
class cast_template_to_cppdatalib<QList, Ts...>
{
    const QList<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const QList<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename T>
class cast_template_from_cppdatalib<QList, T>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QList<T>() const
    {
        QList<T> result;
        convert(result);
        return result;
    }
    void convert(QList<T> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
    }
};

// -------------
//  QLinkedList
// -------------

template<typename... Ts>
class cast_template_to_cppdatalib<QLinkedList, Ts...>
{
    const QLinkedList<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const QLinkedList<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename T>
class cast_template_from_cppdatalib<QLinkedList, T>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QLinkedList<T>() const
    {
        QLinkedList result;
        convert(result);
        return result;
    }
    void convert(QLinkedList<T> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
    }
};

// --------
//  QStack
// --------

template<typename... Ts>
class cast_template_to_cppdatalib<QStack, Ts...>
{
    const QStack<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const QStack<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename T>
class cast_template_from_cppdatalib<QStack, T>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QStack<T>() const
    {
        QStack<T> result;
        convert(result);
        return result;
    }
    void convert(QStack<T> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
    }
};

// --------
//  QQueue
// --------

template<typename... Ts>
class cast_template_to_cppdatalib<QQueue, Ts...>
{
    const QQueue<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const QQueue<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename T>
class cast_template_from_cppdatalib<QQueue, T>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QQueue<T>() const
    {
        QQueue<T> result;
        convert(result);
        return result;
    }
    void convert(QQueue<T> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.push_back(item.operator T());
    }
};

// ------
//  QSet
// ------

template<typename... Ts>
class cast_template_to_cppdatalib<QSet, Ts...>
{
    const QSet<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const QSet<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_array({}, cppdatalib::core::normal);
        for (const auto &item: bind)
            dest.push_back(cppdatalib::core::value(item));
    }
};

template<typename T>
class cast_template_from_cppdatalib<QSet, T>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QSet<T>() const
    {
        QSet<T> result;
        convert(result);
        return result;
    }
    void convert(QSet<T> &dest) const
    {
        dest.clear();
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                dest.insert(item.operator T());
    }
};

// ------
//  QMap
// ------

template<typename... Ts>
class cast_template_to_cppdatalib<QMap, Ts...>
{
    const QMap<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const QMap<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_object({}, cppdatalib::core::normal);
        for (auto it = bind.begin(); it != bind.end(); ++it)
            dest.add_member_at_end(cppdatalib::core::value(it.key()),
                                   cppdatalib::core::value(it.value()));
    }
};

template<typename K, typename V>
class cast_template_from_cppdatalib<QMap, K, V>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QMap<K, V>() const
    {
        QMap<K, V> result;
        convert(result);
        return result;
    }
    void convert(QMap<K, V> &dest) const
    {
        dest.clear();
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                dest.insert(item.first.operator K(), item.second.operator V());
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<QMap, Ts...> : public generic_stream_input
    {
    protected:
        const QMap<Ts...> bind;
        decltype(bind.begin()) iterator;
        bool parsingKey;

    public:
        template_parser(const QMap<Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {iterator = bind.begin(); parsingKey = true;}

        void write_one_()
        {
            if (was_just_reset())
            {
                get_output()->begin_object(core::object_t(), bind.size());
                if (iterator != bind.end())
                    compose_parser(iterator->first);
            }
            else if (iterator != bind.end())
            {
                if (parsingKey)
                    compose_parser(iterator->second);
                else
                    compose_parser((iterator++)->first);
                parsingKey = !parsingKey;
            }
            else
                get_output()->end_object(core::object_t());
        }
    };
}}

// -----------
//  QMultiMap
// -----------

template<typename... Ts>
class cast_template_to_cppdatalib<QMultiMap, Ts...>
{
    const QMultiMap<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const QMultiMap<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_object({}, cppdatalib::core::normal);
        for (auto it = bind.begin(); it != bind.end(); ++it)
            dest.add_member_at_end(cppdatalib::core::value(it.key()),
                                   cppdatalib::core::value(it.value()));
    }
};

template<typename K, typename V>
class cast_template_from_cppdatalib<QMultiMap, K, V>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QMultiMap<K, V>() const
    {
        QMultiMap<K, V> result;
        convert(result);
        return result;
    }
    void convert(QMultiMap<K, V> &dest) const
    {
        dest.clear();
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                dest.insert(item.first.operator K(), item.second.operator V());
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<QMultiMap, Ts...> : public generic_stream_input
    {
    protected:
        const QMultiMap<Ts...> bind;
        decltype(bind.begin()) iterator;
        bool parsingKey;

    public:
        template_parser(const QMultiMap<Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {iterator = bind.begin(); parsingKey = true;}

        void write_one_()
        {
            if (was_just_reset())
            {
                get_output()->begin_object(core::object_t(), bind.size());
                if (iterator != bind.end())
                    compose_parser(iterator->first);
            }
            else if (iterator != bind->end())
            {
                if (parsingKey)
                    compose_parser(iterator->second);
                else
                    compose_parser((iterator++)->first);
                parsingKey = !parsingKey;
            }
            else
                get_output()->end_object(core::object_t());
        }
    };
}}

// -------
//  QHash
// -------

template<typename... Ts>
class cast_template_to_cppdatalib<QHash, Ts...>
{
    const QHash<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const QHash<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_object({}, cppdatalib::core::hash);
        for (auto it = bind.begin(); it != bind.end(); ++it)
            dest.add_member(cppdatalib::core::value(it.key()),
                            cppdatalib::core::value(it.value()));
    }
};

template<typename K, typename V>
class cast_template_from_cppdatalib<QHash, K, V>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QHash<K, V>() const
    {
        QHash<K, V> result;
        convert(result);
        return result;
    }
    void convert(QHash<K, V> &dest) const
    {
        dest.clear();
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                dest.insert(item.first.operator K(), item.second.operator V());
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<QHash, Ts...> : public generic_stream_input
    {
    protected:
        const QHash<Ts...> bind;
        decltype(bind.begin()) iterator;
        bool parsingKey;

    public:
        template_parser(const QHash<Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {iterator = bind->begin(); parsingKey = true;}

        void write_one_()
        {
            if (was_just_reset())
            {
                get_output()->begin_object(core::value(core::object_t(), core::hash), bind->size());
                if (iterator != bind.end())
                    compose_parser(iterator->first);
            }
            else if (iterator != bind.end())
            {
                if (parsingKey)
                    compose_parser(iterator->second);
                else
                    compose_parser((iterator++)->first);
                parsingKey = !parsingKey;
            }
            else
                get_output()->end_object(core::object_t());
        }
    };
}}

// ------------
//  QMultiHash
// ------------

template<typename... Ts>
class cast_template_to_cppdatalib<QMultiHash, Ts...>
{
    const QMultiHash<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const QMultiHash<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest) const
    {
        dest.set_object({}, cppdatalib::core::hash);
        for (auto it = bind.begin(); it != bind.end(); ++it)
            dest.add_member(cppdatalib::core::value(it.key()),
                            cppdatalib::core::value(it.value()));
    }
};

template<typename K, typename V>
class cast_template_from_cppdatalib<QMultiHash, K, V>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QMultiHash<K, V>() const
    {
        QMultiHash<K, V> result;
        convert(result);
        return result;
    }
    void convert(QMultiHash<K, V> &dest) const
    {
        dest.clear();
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                dest.insert(item.first.operator K(), item.second.operator V());
    }
};

namespace cppdatalib { namespace core {
    template<typename... Ts>
    class template_parser<QMultiHash, Ts...> : public generic_stream_input
    {
    protected:
        const QMultiHash<Ts...> bind;
        decltype(bind.begin()) iterator;
        bool parsingKey;

    public:
        template_parser(const QMultiHash<Ts...> &bind, generic_parser &parser)
            : generic_stream_input(parser)
            , bind(bind)
        {
            reset();
        }

    protected:
        void reset_() {iterator = bind.begin(); parsingKey = true;}

        void write_one_()
        {
            if (was_just_reset())
            {
                get_output()->begin_object(core::value(core::object_t(), core::hash), bind->size());
                if (iterator != bind.end())
                    compose_parser(iterator->first);
            }
            else if (iterator != bind.end())
            {
                if (parsingKey)
                    compose_parser(iterator->second);
                else
                    compose_parser((iterator++)->first);
                parsingKey = !parsingKey;
            }
            else
                get_output()->end_object(core::object_t());
        }
    };
}}

// ----------
//  QVariant
// ----------

// TODO: implement generic_parser helper specialization for QVariant

template<>
class cast_to_cppdatalib<QVariant>
{
    const QVariant &bind;
public:
    cast_to_cppdatalib(const QVariant &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        convert(result);
        return result;
    }
    void convert(cppdatalib::core::value &dest)
    {
        switch (bind.type())
        {
            default:
            case QVariant::Invalid: dest.set_null(cppdatalib::core::normal); break;
            case QVariant::Bool: dest = bind.toBool(); break;
            case QVariant::ByteArray: dest = bind.toByteArray(); break;
            case QVariant::Char: dest = bind.toChar(); break;
            case QVariant::Date: dest = bind.toDate(); break;
            case QVariant::DateTime: dest = bind.toDateTime(); break;
            case QVariant::Double: dest = bind.toDouble(); break;
            case QVariant::Hash: dest = bind.toHash(); break;
            case QVariant::Int: dest = bind.toInt(); break;
            case QVariant::List: dest = bind.toList(); break;
            case QVariant::LongLong: dest = bind.toLongLong(); break;
            case QVariant::Map: dest = bind.toMap(); break;
            case QVariant::String: dest = bind.toString(); break;
            case QVariant::StringList: dest = bind.toStringList(); break;
            case QVariant::Time: dest = bind.toTime(); break;
            case QVariant::UInt: dest = bind.toUInt(); break;
            case QVariant::ULongLong: dest = bind.toULongLong(); break;
            case QVariant::Uuid: dest = bind.toUuid(); break;
        }
    }
};

template<>
class cast_from_cppdatalib<QVariant>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QVariant() const
    {
        QVariant result;
        convert(result);
        return result;
    }
    void convert(QVariant &dest) const
    {
        switch (bind.get_type())
        {
            case cppdatalib::core::link:
            case cppdatalib::core::null: dest = QVariant(); break;
            case cppdatalib::core::boolean: dest = bind.get_bool_unchecked(); break;
            case cppdatalib::core::integer: dest = qlonglong(bind.get_int_unchecked()); break;
            case cppdatalib::core::uinteger: dest = qulonglong(bind.get_uint_unchecked()); break;
            case cppdatalib::core::real: dest = bind.get_real_unchecked(); break;
            case cppdatalib::core::string:
            {
                switch (bind.get_subtype())
                {
                    case cppdatalib::core::blob:
                    case cppdatalib::core::clob:
                        dest = QByteArray::fromStdString(bind.get_string_unchecked());
                        break;
                    case cppdatalib::core::date: dest = QDate::fromString(QString::fromStdString(bind.get_string_unchecked())); break;
                    case cppdatalib::core::time: dest = QTime::fromString(QString::fromStdString(bind.get_string_unchecked())); break;
                    case cppdatalib::core::datetime: dest = QDateTime::fromString(QString::fromStdString(bind.get_string_unchecked())); break;
                    case cppdatalib::core::uuid: dest = QUuid(QString::fromStdString(bind.get_string_unchecked())); break;
                    default: dest = QString::fromStdString(bind.get_string_unchecked()); break;
                }
                break;
            }
            case cppdatalib::core::array: dest = bind.operator QVariantList(); break;
            case cppdatalib::core::object:
            {
                if (bind.get_subtype() == cppdatalib::core::hash)
                    dest = bind.operator QVariantHash();
                else
                    dest = bind.operator QVariantMap();
                break;
            }
        }
    }
};

#endif // CPPDATALIB_QT_H
