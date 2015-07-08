//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2012-2014 Ripple Labs Inc.

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

#ifndef RIPPLE_RPC_LOOKUPLEDGER_H_INCLUDED
#define RIPPLE_RPC_LOOKUPLEDGER_H_INCLUDED

#include <ripple/rpc/Status.h>

namespace ripple {

class ReadView;

namespace RPC {

class Context;

/** Look up a ledger from a request and fill a Json::Result with either
    an error, or data representing a ledger.

    If there is no error in the return value, then the ledger pointer will have
    been filled.
*/
Json::Value lookupLedgerDeprecated (Ledger::pointer&, Context&);
Json::Value lookupLedger (std::shared_ptr<ReadView const>&, Context&);

/** Look up a ledger from a request and fill a Json::Result with the data
    representing a ledger.

    If the returned Status is OK, the ledger pointer will have been filled. */
Status lookupLedger (
    std::shared_ptr<ReadView const>&, Context&, Json::Value& result);

} // RPC
} // ripple

#endif
