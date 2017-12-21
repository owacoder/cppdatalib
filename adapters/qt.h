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

// ------------
//  QByteArray
// ------------

void to_cppdatalib(QByteArray v, cppdatalib::core::value &result)
{
    result.set_string(v.toStdString(), cppdatalib::core::blob);
}

void from_cppdatalib(const cppdatalib::core::value &v, QByteArray &result)
{
    result = QByteArray::fromStdString(v.as_string());
}

// ---------------
//  QLatin1String
// ---------------

void to_cppdatalib(QLatin1String v, cppdatalib::core::value &result)
{
    result.set_string(cppdatalib::core::string_t(v.data(), v.size()), cppdatalib::core::clob);
}

// -------
//  QChar
// -------

void to_cppdatalib(QChar v, cppdatalib::core::value &result)
{
    result = QString(v).toUtf8().toStdString();
}

// ---------
//  QString
// ---------

void to_cppdatalib(QString v, cppdatalib::core::value &result)
{
    result = v.toUtf8().toStdString();
}

void from_cppdatalib(const cppdatalib::core::value &v, QString &result)
{
    result = QString::fromUtf8(QByteArray::fromStdString(v.as_string()));
}

// -------
//  QDate
// -------

void to_cppdatalib(QDate v, cppdatalib::core::value &result)
{
    result.set_string(v.toString().toUtf8().toStdString(), cppdatalib::core::date);
}

void from_cppdatalib(const cppdatalib::core::value &v, QDate &result)
{
    result = QDate::fromString(QString::fromUtf8(QByteArray::fromStdString(v.as_string())));
}

// -------
//  QTime
// -------

void to_cppdatalib(QTime v, cppdatalib::core::value &result)
{
    result.set_string(v.toString().toUtf8().toStdString(), cppdatalib::core::time);
}

void from_cppdatalib(const cppdatalib::core::value &v, QTime &result)
{
    result = QTime::fromString(QString::fromUtf8(QByteArray::fromStdString(v.as_string())));
}

// -----------
//  QDateTime
// -----------

void to_cppdatalib(QDateTime v, cppdatalib::core::value &result)
{
    result.set_string(v.toString().toUtf8().toStdString(), cppdatalib::core::datetime);
}

void from_cppdatalib(const cppdatalib::core::value &v, QDateTime &result)
{
    result = QDateTime::fromString(QString::fromUtf8(QByteArray::fromStdString(v.as_string())));
}

// -------
//  QUuid
// -------

void to_cppdatalib(QUuid v, cppdatalib::core::value &result)
{
    result.set_string(v.toString().toUtf8().toStdString(), cppdatalib::core::uuid);
}

void from_cppdatalib(const cppdatalib::core::value &v, QUuid &result)
{
    result = QUuid(QString::fromUtf8(QByteArray::fromStdString(v.as_string())));
}

// ----------
//  QVariant
// ----------

void to_cppdatalib(QVariant v, cppdatalib::core::value &result)
{
    switch (v.type())
    {
        default:
        case QVariant::Invalid: result.set_null(); break;
        case QVariant::Bool: result = v.toBool(); break;
        case QVariant::ByteArray: result = v.toByteArray(); break;
        case QVariant::Char: result = v.toChar(); break;
        case QVariant::Date: result = v.toDate(); break;
        case QVariant::DateTime: result = v.toDateTime(); break;
        case QVariant::Double: result = v.toDouble(); break;
        case QVariant::Hash: result = v.toHash(); break;
        case QVariant::Int: result = v.toInt(); break;
        case QVariant::List: result = v.toList(); break;
        case QVariant::LongLong: result = v.toLongLong(); break;
        case QVariant::Map: result = v.toMap(); break;
        case QVariant::String: result = v.toString(); break;
        case QVariant::StringList: result = v.toStringList(); break;
        case QVariant::Time: result = v.toTime(); break;
        case QVariant::UInt: result = v.toUInt(); break;
        case QVariant::ULongLong: result = v.toULongLong(); break;
        case QVariant::Uuid: result = v.toUuid(); break;
    }
}

void from_cppdatalib(const cppdatalib::core::value &v, QVariant &result)
{
    switch (v.get_type())
    {
        case cppdatalib::core::null: result = QVariant(); break;
        case cppdatalib::core::boolean: result = v.get_bool_unchecked(); break;
        case cppdatalib::core::integer: result = qlonglong(v.get_int_unchecked()); break;
        case cppdatalib::core::uinteger: result = qulonglong(v.get_uint_unchecked()); break;
        case cppdatalib::core::real: result = v.get_real_unchecked(); break;
        case cppdatalib::core::string:
        {
            switch (v.get_subtype())
            {
                case cppdatalib::core::blob:
                case cppdatalib::core::clob:
                    result = QByteArray::fromStdString(v.get_string_unchecked());
                    break;
                case cppdatalib::core::date: result = QDate::fromString(QString::fromUtf8(QByteArray::fromStdString(v.get_string_unchecked()))); break;
                case cppdatalib::core::time: result = QTime::fromString(QString::fromUtf8(QByteArray::fromStdString(v.get_string_unchecked()))); break;
                case cppdatalib::core::datetime: result = QDateTime::fromString(QString::fromUtf8(QByteArray::fromStdString(v.get_string_unchecked()))); break;
                case cppdatalib::core::uuid: result = QUuid(QString::fromUtf8(QByteArray::fromStdString(v.get_string_unchecked()))); break;
                default: result = QString::fromUtf8(QByteArray::fromStdString(v.get_string_unchecked())); break;
            }
            break;
        }
        case cppdatalib::core::array: result = cppdatalib::from_cppdatalib<QVariantList>(v); break;
        case cppdatalib::core::object:
        {
            if (v.get_subtype() == cppdatalib::core::hash)
                result = cppdatalib::from_cppdatalib<QVariantHash>(v);
            else
                result = cppdatalib::from_cppdatalib<QVariantMap>(v);
            break;
        }
    }
}

// ----------------
//  QByteArrayList
// ----------------

void to_cppdatalib(QByteArrayList v, cppdatalib::core::value &result)
{
    result = cppdatalib::core::array_t();
    for (const auto &item: v)
        result.push_back(item);
}

void from_cppdatalib(const cppdatalib::core::value &v, QByteArrayList &result)
{
    result.clear();
    if (v.is_array())
        for (const auto &item: v.get_array_unchecked())
            result.push_back(item);
}

// -------------
//  QStringList
// -------------

void to_cppdatalib(QStringList v, cppdatalib::core::value &result)
{
    result = cppdatalib::core::array_t();
    for (const auto &item: v)
        result.push_back(item);
}

void from_cppdatalib(const cppdatalib::core::value &v, QStringList &result)
{
    result.clear();
    if (v.is_array())
        for (const auto &item: v.get_array_unchecked())
            result.push_back(item);
}

// ---------
//  QVector
// ---------

template<typename... Ts> void to_cppdatalib(QVector<Ts...> v, cppdatalib::core::value &result)
{
    result = cppdatalib::core::array_t();
    for (const auto &item: v)
        result.push_back(item);
}

template<typename... Ts> void from_cppdatalib(const cppdatalib::core::value &v, QVector<Ts...> &result)
{
    result.clear();
    if (v.is_array())
        for (const auto &item: v.get_array_unchecked())
            result.push_back(item);
}

// -------
//  QList
// -------

template<typename... Ts> void to_cppdatalib(QList<Ts...> v, cppdatalib::core::value &result)
{
    result = cppdatalib::core::array_t();
    for (const auto &item: v)
        result.push_back(item);
}

template<typename... Ts> void from_cppdatalib(const cppdatalib::core::value &v, QList<Ts...> &result)
{
    result.clear();
    if (v.is_array())
        for (const auto &item: v.get_array_unchecked())
            result.push_back(item);
}

// -------------
//  QLinkedList
// -------------

template<typename... Ts> void to_cppdatalib(QLinkedList<Ts...> v, cppdatalib::core::value &result)
{
    result = cppdatalib::core::array_t();
    for (const auto &item: v)
        result.push_back(item);
}

template<typename... Ts> void from_cppdatalib(const cppdatalib::core::value &v, QLinkedList<Ts...> &result)
{
    result.clear();
    if (v.is_array())
        for (const auto &item: v.get_array_unchecked())
            result.push_back(item);
}

// --------
//  QStack
// --------

template<typename... Ts> void to_cppdatalib(QStack<Ts...> v, cppdatalib::core::value &result)
{
    result = cppdatalib::core::array_t();
    for (const auto &item: v)
        result.push_back(item);
}

template<typename... Ts> void from_cppdatalib(const cppdatalib::core::value &v, QStack<Ts...> &result)
{
    result.clear();
    if (v.is_array())
        for (const auto &item: v.get_array_unchecked())
            result.push(item);
}

// --------
//  QQueue
// --------

template<typename... Ts> void to_cppdatalib(QQueue<Ts...> v, cppdatalib::core::value &result)
{
    result = cppdatalib::core::array_t();
    for (const auto &item: v)
        result.push_back(item);
}

template<typename... Ts> void from_cppdatalib(const cppdatalib::core::value &v, QQueue<Ts...> &result)
{
    result.clear();
    if (v.is_array())
        for (const auto &item: v.get_array_unchecked())
            result.push(item);
}

// ------
//  QSet
// ------

template<typename... Ts> void to_cppdatalib(QSet<Ts...> v, cppdatalib::core::value &result)
{
    result = cppdatalib::core::array_t();
    for (const auto &item: v)
        result.push_back(item);
}

template<typename... Ts> void from_cppdatalib(const cppdatalib::core::value &v, QSet<Ts...> &result)
{
    result.clear();
    if (v.is_array())
        for (const auto &item: v.get_array_unchecked())
            result.push(item);
}

// ------
//  QMap
// ------

template<typename... Ts> void to_cppdatalib(QMap<Ts...> v, cppdatalib::core::value &result)
{
    result = cppdatalib::core::object_t();
    for (auto it = v.cbegin(); it != v.cend(); ++it)
        result.add_member(it.key(), it.value());
}

template<typename... Ts> void from_cppdatalib(const cppdatalib::core::value &v, QMap<Ts...> &result)
{
    result.clear();
    if (v.is_object())
        for (const auto &item: v.get_object_unchecked())
            result.insert(item.first, item.second);
}

// -----------
//  QMultiMap
// -----------

template<typename... Ts> void to_cppdatalib(QMultiMap<Ts...> v, cppdatalib::core::value &result)
{
    result = cppdatalib::core::object_t();
    for (auto it = v.cbegin(); it != v.cend(); ++it)
        result.add_member(it.key(), it.value());
}

template<typename... Ts> void from_cppdatalib(const cppdatalib::core::value &v, QMultiMap<Ts...> &result)
{
    result.clear();
    if (v.is_object())
        for (const auto &item: v.get_object_unchecked())
            result.insert(item.first, item.second);
}

// -------
//  QHash
// -------

template<typename... Ts> void to_cppdatalib(QHash<Ts...> v, cppdatalib::core::value &result)
{
    result.set_object(cppdatalib::core::object_t(), cppdatalib::core::hash);
    for (auto it = v.cbegin(); it != v.cend(); ++it)
        result.add_member(it.key(), it.value());
}

template<typename... Ts> void from_cppdatalib(const cppdatalib::core::value &v, QHash<Ts...> &result)
{
    result.clear();
    if (v.is_object())
        for (const auto &item: v.get_object_unchecked())
            result.insert(item.first, item.second);
}

// ------------
//  QMultiHash
// ------------

template<typename... Ts> void to_cppdatalib(QMultiHash<Ts...> v, cppdatalib::core::value &result)
{
    result.set_object(cppdatalib::core::object_t(), cppdatalib::core::hash);
    for (auto it = v.cbegin(); it != v.cend(); ++it)
        result.add_member(it.key(), it.value());
}

template<typename... Ts> void from_cppdatalib(const cppdatalib::core::value &v, QMultiHash<Ts...> &result)
{
    result.clear();
    if (v.is_object())
        for (const auto &item: v.get_object_unchecked())
            result.insert(item.first, item.second);
}

#endif // CPPDATALIB_QT_H
