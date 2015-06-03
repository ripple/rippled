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

#include <BeastConfig.h>
#include <ripple/app/ledger/impl/ConsensusImp.h>
#include <ripple/app/ledger/impl/LedgerConsensusImp.h>
#include <beast/utility/Journal.h>

namespace ripple {

ConsensusImp::ConsensusImp (NetworkOPs& netops)
    : journal_ (deprecatedLogs().journal("Consensus"))
    , netops_ (netops)
    , proposing_ (false)
    , validating_ (false)
    , lastCloseProposers_ (0)
    , lastCloseConvergeTook_ (1000 * LEDGER_IDLE_INTERVAL)
    , lastValidationTimestamp_ (0)
    , lastCloseTime_ (0)
{
}

bool
ConsensusImp::isProposing () const
{
    return proposing_;
}

bool
ConsensusImp::isValidating () const
{
    return validating_;
}

int
ConsensusImp::getLastCloseProposers () const
{
    return lastCloseProposers_;
}

int
ConsensusImp::getLastCloseDuration () const
{
    return lastCloseConvergeTook_;
}

std::shared_ptr<LedgerConsensus>
ConsensusImp::startRound (
    InboundTransactions& inboundTransactions,
    LocalTxs& localtx,
    LedgerHash const &prevLCLHash,
    Ledger::ref previousLedger,
    std::uint32_t closeTime,
    FeeVote& feeVote)
{
    return make_LedgerConsensus (*this, lastCloseProposers_,
        lastCloseConvergeTook_, inboundTransactions, localtx,
        prevLCLHash, previousLedger, closeTime, feeVote);
}


void
ConsensusImp::setProposing (bool p, bool v)
{
    proposing_ = p;
    validating_ = v;
}

STValidation::ref
ConsensusImp::getLastValidation () const
{
    return lastValidation_;
}

void
ConsensusImp::setLastValidation (STValidation::ref v)
{
    lastValidation_ = v;
}

void
ConsensusImp::newLCL (
    int proposers,
    int convergeTime,
    uint256 const& ledgerHash)
{
    lastCloseProposers_ = proposers;
    lastCloseConvergeTook_ = convergeTime;
    lastCloseHash_ = ledgerHash;
}

std::uint32_t
ConsensusImp::validationTimestamp ()
{
    std::uint32_t vt = netops_.getNetworkTimeNC ();

    if (vt <= lastValidationTimestamp_)
        vt = lastValidationTimestamp_ + 1;

    lastValidationTimestamp_ = vt;
    return vt;
}

std::uint32_t
ConsensusImp::getLastCloseTime () const
{
    return lastCloseTime_;
}

void
ConsensusImp::setLastCloseTime (std::uint32_t t)
{
    lastCloseTime_ = t;
}

//==============================================================================

std::unique_ptr<Consensus>
make_Consensus (NetworkOPs& netops)
{
    return std::make_unique<ConsensusImp> (netops);
}

}
