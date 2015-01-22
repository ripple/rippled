//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2015 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef RIPPLE_SOCIDB_H_INCLUDED
#define RIPPLE_SOCIDB_H_INCLUDED

/** An embedded database wrapper with an intuitive, type-safe interface.

    This collection of classes let's you access embedded SQLite databases
    using C++ syntax that is very similar to regular SQL.

    This module requires the @ref beast_sqlite external module.
*/

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning( \
    disable : 4355)  // 'this' : used in base member initializer list
#endif

#include <ripple/basics/Log.h>
#define SOCI_USE_BOOST
#include <core/soci.h>
#include <string>
#include <cstdint>
#include <vector>

namespace ripple {
template <class T, class C>
T rangeCheckedCast (C c)
{
    if ((c > std::numeric_limits<T>::max ()) ||
        (!std::numeric_limits<T>::is_signed && c < 0) ||
        (std::numeric_limits<T>::is_signed && 
         std::numeric_limits<C>::is_signed && 
         c < std::numeric_limits<T>::lowest ()))
    {
        WriteLog (lsERROR, RangeCheckedCast)
            << "Range error. Min: " << std::numeric_limits<T>::lowest ()
            << " Max: " << std::numeric_limits<T>::max () << " Got: " << c;
    }
    return static_cast<T>(c);
}
}

namespace soci {
template <>
inline std::uint8_t row::get<std::uint8_t>(std::size_t pos) const
{
    using xt = std::int32_t;
    xt const r = get<xt>(pos);
    return ripple::rangeCheckedCast<std::uint8_t>(r);
}

template <>
inline std::uint8_t row::get<std::uint8_t>(std::size_t pos,
                                           std::uint8_t const& nullValue) const
{
    assert (holders_.size () >= pos + 1);

    if (i_null == *indicators_[pos])
    {
        return nullValue;
    }
    return get<std::uint8_t>(pos);
}

template <>
inline std::uint16_t row::get<std::uint16_t>(std::size_t pos) const
{
    using xt = std::int32_t;
    xt const r = get<xt>(pos);
    return ripple::rangeCheckedCast<std::uint16_t>(r);
}

template <>
inline std::uint16_t row::get<std::uint16_t>(
    std::size_t pos,
    std::uint16_t const& nullValue) const
{
    assert (holders_.size () >= pos + 1);

    if (i_null == *indicators_[pos])
    {
        return nullValue;
    }
    return get<std::uint16_t>(pos);
}

template <>
inline std::uint32_t row::get<std::uint32_t>(std::size_t pos) const
{
    using xt = std::int32_t;
    // can't check range for int
    return get<xt>(pos);
}

template <>
inline std::uint32_t row::get<std::uint32_t>(
    std::size_t pos,
    std::uint32_t const& nullValue) const
{
    assert (holders_.size () >= pos + 1);

    if (i_null == *indicators_[pos])
    {
        return nullValue;
    }
    return get<std::uint32_t>(pos);
}

template <>
inline std::uint64_t row::get<std::uint64_t>(std::size_t pos) const
{
    using xt = std::int64_t;
    return get<xt>(pos);
}

template <>
inline std::uint64_t row::get<std::uint64_t>(
    std::size_t pos,
    std::uint64_t const& nullValue) const
{
    assert (holders_.size () >= pos + 1);

    if (i_null == *indicators_[pos])
    {
        return nullValue;
    }
    return get<std::uint64_t>(pos);
}
}

namespace ripple {
class BasicConfig;

/**
 *  SociConfig is used when a client wants to delay opening a soci::session after
 *  parsing the config parameters. If a client want to open a session immediately,
 *  use the free function "open" below.
 */
class SociConfig final
{
    std::string connectionString_;
    soci::backend_factory const& backendFactory_;
    SociConfig(std::pair<std::string, soci::backend_factory const&> init);
public:
    SociConfig(BasicConfig const& config,
               std::string const& dbName);
    std::string connectionString () const;
    void open(soci::session& s) const;
};

/**
 *  Open a soci session.
 *
 *  @param s Session to open.
 *  @param config Parameters to pick the soci backend and how to connect to that
 *                backend.
 *  @param dbName Name of the database. This has different meaning for different backends.
 *                Sometimes it is part of a filename (sqlite3), othertimes it is a
 *                database name (postgresql).
 */
void open(soci::session& s,
          BasicConfig const& config,
          std::string const& dbName);

/**
 *  Open a soci session.
 *
 *  @param s Session to open.
 *  @param beName Backend name.
 *  @param connectionString Connection string to forward to soci::open.
 *         see the soci::open documentation for how to use this.
 *
 */
void open(soci::session& s,
          std::string const& beName,
          std::string const& connectionString);

size_t getKBUsedAll (soci::session& s);
size_t getKBUsedDB (soci::session& s);

void convert(soci::blob /*const*/& from, std::vector<std::uint8_t>& to);
void convert(soci::blob /*const*/& from, std::string& to);
void convert(std::vector<std::uint8_t> const& from, soci::blob& to);
}


#if _MSC_VER
#pragma warning(pop)
#endif

#endif
