//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012, 2013 Ripple Labs Inc.

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

#ifndef RIPPLE_TEST_JSON_H_INCLUDED
#define RIPPLE_TEST_JSON_H_INCLUDED

#include <ripple/test/jtx/any.h>
#include <ripple/test/jtx/tags.h>
#include <ripple/test/Account.h>
#include <ripple/json/json_value.h>

namespace ripple {
namespace test {

namespace jtx {

/** Add and/or remove flag. */
Json::Value
fset (Account const& account,
    std::uint32_t on, std::uint32_t off = 0);

/** Remove account flag. */
inline
Json::Value
fclear (Account const& account,
    std::uint32_t off)
{
    return fset(account, 0, off);
}

/** Create a payment. */
Json::Value
pay (Account const& account,
    Account const& to,
        AnyAmount amount);

/** Create an offer. */
Json::Value
offer (Account const& account,
    STAmount const& in, STAmount const& out);

/** Set a transfer rate. */
Json::Value
rate (Account const& account,
    double multiplier);

/** Disable the regular key. */
Json::Value
regkey (Account const& account,
    disabled_t);

/** Set a regular key. */
Json::Value
regkey (Account const& account,
    Account const& signer);
/** The null transaction. */
inline
Json::Value
noop (Account const& account)
{
    return fset(account, 0);
}

/** Modify a trust line. */
Json::Value
trust (Account const& account,
    STAmount const& amount);

} // jtx

} // test
} // ripple

#endif
