/*
 * cppdatalib.h
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

#ifndef CPPDATALIB_H
#define CPPDATALIB_H

#include "core/core.h"
#include "json/json_pointer.h"
#include "json/json_patch.h"
#include "json/json.h"
#include "bencode/bencode.h"
#include "bjson/bjson.h"
#include "property_list/plain_text.h"
#include "property_list/xml.h"
#include "rpc/xml.h"
#include "csv/csv.h"
#include "binn/binn.h"
#include "ubjson/ubjson.h"
#include "xls/xml.h"
#include "netstrings/netstrings.h"
#include "message_pack/message_pack.h"
#include "mysql/mysql.h"

#ifdef CPPDATALIB_ENABLE_BOOST_COMPUTE
#include "adapters/boost_compute.h"
#endif

#ifdef CPPDATALIB_ENABLE_BOOST_CONTAINER
#include "adapters/boost_container.h"
#endif

#ifdef CPPDATALIB_ENABLE_QT
#include "adapters/qt.h"
#endif

#ifdef CPPDATALIB_ENABLE_POCO
#include "adapters/poco.h"
#endif

#ifdef CPPDATALIB_ENABLE_ETL
#include "adapters/etl.h"
#endif

#ifdef CPPDATALIB_ENABLE_STL
#include "adapters/stl.h"
#endif

#endif // CPPDATALIB_H
