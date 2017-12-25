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

#include "../core/value.h"

// -------
//  QPair
// -------

template<typename... Ts>
class cast_template_to_cppdatalib<QPair, Ts...>
{
    const QPair<Ts...> &bind;
public:
    cast_template_to_cppdatalib(const QPair<Ts...> &bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::array_t{cppdatalib::core::value(bind.first), cppdatalib::core::value(bind.second)};}
};

template<typename T1, typename T2>
class cast_template_from_cppdatalib<QPair, T1, T2>
{
    const cppdatalib::core::value &bind;
public:
    cast_template_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QPair<T1, T2>() const {return QPair<T1, T2>{bind.element(0).operator T1(), bind.element(1).operator T2()};}
};

// ------------
//  QByteArray
// ------------

template<>
class cast_to_cppdatalib<QByteArray>
{
    const QByteArray &bind;
public:
    cast_to_cppdatalib(const QByteArray &bind) : bind(bind) {}
    operator cppdatalib::core::value() const {return cppdatalib::core::value(bind.toStdString(), cppdatalib::core::blob);}
};

template<>
class cast_from_cppdatalib<QByteArray>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QByteArray() const {return QByteArray::fromStdString(bind.as_string());}
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
    operator cppdatalib::core::value() const {return cppdatalib::core::value(cppdatalib::core::string_t(bind.data(), bind.size()), cppdatalib::core::clob);}
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
    operator cppdatalib::core::value() const {return QString(bind).toStdString();}
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
    operator cppdatalib::core::value() const {return bind.toStdString();}
};

template<>
class cast_from_cppdatalib<QString>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QString() const {return QString::fromStdString(bind.as_string());}
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
    operator cppdatalib::core::value() const {return cppdatalib::core::value(bind.toString().toStdString(), cppdatalib::core::date);}
};

template<>
class cast_from_cppdatalib<QDate>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QDate() const {return QDate::fromString(QString::fromStdString(bind.as_string()));}
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
    operator cppdatalib::core::value() const {return cppdatalib::core::value(bind.toString().toStdString(), cppdatalib::core::time);}
};

template<>
class cast_from_cppdatalib<QTime>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QTime() const {return QTime::fromString(QString::fromStdString(bind.as_string()));}
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
    operator cppdatalib::core::value() const {return cppdatalib::core::value(bind.toString().toStdString(), cppdatalib::core::datetime);}
};

template<>
class cast_from_cppdatalib<QDateTime>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QDateTime() const {return QDateTime::fromString(QString::fromStdString(bind.as_string()));}
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
    operator cppdatalib::core::value() const {return cppdatalib::core::value(bind.toString().toStdString(), cppdatalib::core::uuid);}
};

template<>
class cast_from_cppdatalib<QUuid>
{
    const cppdatalib::core::value &bind;
public:
    cast_from_cppdatalib(const cppdatalib::core::value &bind) : bind(bind) {}
    operator QUuid() const {return QUuid(QString::fromStdString(bind.as_string()));}
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(item);
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item);
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        QLinkedList<T> result;
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.push_back(item.operator T());
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::array_t();
        for (const auto &item: bind)
            result.push_back(cppdatalib::core::value(item));
        return result;
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
        if (bind.is_array())
            for (const auto &item: bind.get_array_unchecked())
                result.insert(item.operator T());
        return result;
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
        cppdatalib::core::value result = cppdatalib::core::object_t();
        for (auto it = bind.begin(); it != bind.end(); ++it)
            result.add_member(cppdatalib::core::value(it.key()),
                              cppdatalib::core::value(it.value()));
        return result;
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
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                result.insert(item.first.operator K(), item.second.operator V());
        return result;
    }
};

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
        cppdatalib::core::value result = cppdatalib::core::object_t();
        for (auto it = bind.begin(); it != bind.end(); ++it)
            result.add_member(cppdatalib::core::value(it.key()),
                              cppdatalib::core::value(it.value()));
        return result;
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
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                result.insert(item.first.operator K(), item.second.operator V());
        return result;
    }
};

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
        cppdatalib::core::value result = cppdatalib::core::object_t();
        for (auto it = bind.begin(); it != bind.end(); ++it)
            result.add_member(cppdatalib::core::value(it.key()),
                              cppdatalib::core::value(it.value()));
        return result;
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
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                result.insert(item.first.operator K(), item.second.operator V());
        return result;
    }
};

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
        cppdatalib::core::value result = cppdatalib::core::object_t();
        for (auto it = bind.begin(); it != bind.end(); ++it)
            result.add_member(cppdatalib::core::value(it.key()),
                              cppdatalib::core::value(it.value()));
        return result;
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
        if (bind.is_object())
            for (const auto &item: bind.get_object_unchecked())
                result.insert(item.first.operator K(), item.second.operator V());
        return result;
    }
};

// ----------
//  QVariant
// ----------

template<>
class cast_to_cppdatalib<QVariant>
{
    const QVariant &bind;
public:
    cast_to_cppdatalib(const QVariant &bind) : bind(bind) {}
    operator cppdatalib::core::value() const
    {
        cppdatalib::core::value result;
        switch (bind.type())
        {
            default:
            case QVariant::Invalid: result.set_null(); break;
            case QVariant::Bool: result = bind.toBool(); break;
            case QVariant::ByteArray: result = bind.toByteArray(); break;
            case QVariant::Char: result = bind.toChar(); break;
            case QVariant::Date: result = bind.toDate(); break;
            case QVariant::DateTime: result = bind.toDateTime(); break;
            case QVariant::Double: result = bind.toDouble(); break;
            case QVariant::Hash: result = bind.toHash(); break;
            case QVariant::Int: result = bind.toInt(); break;
            case QVariant::List: result = bind.toList(); break;
            case QVariant::LongLong: result = bind.toLongLong(); break;
            case QVariant::Map: result = bind.toMap(); break;
            case QVariant::String: result = bind.toString(); break;
            case QVariant::StringList: result = bind.toStringList(); break;
            case QVariant::Time: result = bind.toTime(); break;
            case QVariant::UInt: result = bind.toUInt(); break;
            case QVariant::ULongLong: result = bind.toULongLong(); break;
            case QVariant::Uuid: result = bind.toUuid(); break;
        }
        return result;
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
        switch (bind.get_type())
        {
            default:
            case cppdatalib::core::null: result = QVariant(); break;
            case cppdatalib::core::boolean: result = bind.get_bool_unchecked(); break;
            case cppdatalib::core::integer: result = qlonglong(bind.get_int_unchecked()); break;
            case cppdatalib::core::uinteger: result = qulonglong(bind.get_uint_unchecked()); break;
            case cppdatalib::core::real: result = bind.get_real_unchecked(); break;
            case cppdatalib::core::string:
            {
                switch (bind.get_subtype())
                {
                    case cppdatalib::core::blob:
                    case cppdatalib::core::clob:
                        result = QByteArray::fromStdString(bind.get_string_unchecked());
                        break;
                    case cppdatalib::core::date: result = QDate::fromString(QString::fromStdString(bind.get_string_unchecked())); break;
                    case cppdatalib::core::time: result = QTime::fromString(QString::fromStdString(bind.get_string_unchecked())); break;
                    case cppdatalib::core::datetime: result = QDateTime::fromString(QString::fromStdString(bind.get_string_unchecked())); break;
                    case cppdatalib::core::uuid: result = QUuid(QString::fromStdString(bind.get_string_unchecked())); break;
                    default: result = QString::fromStdString(bind.get_string_unchecked()); break;
                }
                break;
            }
            case cppdatalib::core::array: result = bind.operator QVariantList(); break;
            case cppdatalib::core::object:
            {
                if (bind.get_subtype() == cppdatalib::core::hash)
                    result = bind.operator QVariantHash();
                else
                    result = bind.operator QVariantMap();
                break;
            }
        }
        return result;
    }
};

#endif // CPPDATALIB_QT_H
