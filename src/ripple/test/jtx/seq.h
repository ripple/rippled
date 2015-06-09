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

#ifndef RIPPLE_TEST_JTX_SEQ_H_INCLUDED
#define RIPPLE_TEST_JTX_SEQ_H_INCLUDED

#include <ripple/test/jtx/Env.h>
#include <ripple/test/jtx/tags.h>
#include <boost/logic/tribool.hpp>

namespace ripple {
namespace test {
namespace jtx {

/** Set the sequence number on a JTx. */
struct seq
{
private:
    std::uint32_t v_;
    boost::tribool b_ =
        boost::logic::indeterminate;

public:
    explicit
    seq (autofill_t)
        : b_(true)
    {
    }

    explicit
    seq (none_t)
        : b_(false)
    {
    }

    explicit
    seq (std::uint32_t v)
        : v_(v)
    {
    }

    void
    operator()(Env const&, JTx& jt) const;
};

} // jtx
} // test
} // ripple

#endif
