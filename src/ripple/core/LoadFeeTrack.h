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

#ifndef RIPPLE_CORE_LOADFEETRACK_H_INCLUDED
#define RIPPLE_CORE_LOADFEETRACK_H_INCLUDED

#include <ripple/json/json_value.h>
#include <beast/utility/Journal.h>
#include <cstdint>

namespace ripple {

/** Manages the current fee schedule.

    The "base" fee is the cost to send a reference transaction under no load,
    expressed in millionths of one XRP.

    The "load" fee is how much the local server currently charges to send a
    reference transaction. This fee fluctuates based on the load of the
    server.
*/
// VFALCO TODO Rename "load" to "current".
class LoadFeeTrack
{
public:
    /** Create a new tracker.
    */
    static LoadFeeTrack* New (bool standAlone, beast::Journal journal);

    virtual ~LoadFeeTrack () { }

    // Scale from fee units to drops
    virtual std::uint64_t scaleFeeBase (std::uint64_t fee, std::uint64_t baseFee,
                                        std::uint32_t referenceFeeUnits) const = 0;

    // Scale using load as well as base rate
    virtual std::uint64_t scaleFeeLoad (std::uint64_t fee, std::uint64_t baseFee,
                                        std::uint32_t referenceFeeUnits,
                                        bool bAdmin) = 0;

    // Get transaction scaling factor
    virtual std::uint64_t scaleTxnFee (std::uint64_t fee) = 0;

    // Get load factor to report to clients
    virtual std::uint64_t getTxnFeeReport () = 0;

    // Set minimum transactions per ledger before fee escalation
    virtual int setMinimumTx (int minimumTx) = 0;

    // A new open ledger has been built
    virtual void onLedger (
        std::size_t openCount,
        std::vector<int> const& feesPaid,
        bool healthy) = 0;

    // A transaction has been accepted into the open ledger
    virtual void onTx (std::uint64_t feeRatio) = 0;

    virtual std::uint32_t getLocalLevel () = 0;
    virtual std::uint32_t getClusterLevel () = 0;

    virtual std::uint32_t getLoadBase () = 0;
    virtual std::uint32_t getLoadFactor () = 0;

    virtual int getMedianFee() = 0;
    virtual int getExpectedLedgerSize() = 0;

    virtual void setClusterLevel (std::uint32_t) = 0;
    virtual bool raiseLocalLevel () = 0;
    virtual bool lowerLocalLevel () = 0;
    virtual bool isLoadedLocal () = 0;
    virtual bool isLoadedCluster () = 0;
};

} // ripple

#endif
