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

#ifndef RIPPLE_TX_APPLYCONTEXT_H_INCLUDED
#define RIPPLE_TX_APPLYCONTEXT_H_INCLUDED

#include <ripple/app/main/Application.h>
#include <ripple/basics/XRPAmount.h>
#include <ripple/beast/utility/Journal.h>
#include <ripple/core/Config.h>
#include <ripple/ledger/ApplyViewImpl.h>
#include <ripple/protocol/STTx.h>
#include <optional>
#include <utility>

namespace ripple {

/** State information when applying a tx. */
class ApplyContext
{
public:
    explicit ApplyContext(
        Application& app,
        OpenView& base,
        STTx const& tx,
        TER preclaimResult,
        FeeUnit64 baseFee,
        ApplyFlags flags,
        beast::Journal = beast::Journal{beast::Journal::getNullSink()});

    Application& app;
    STTx const& tx;
    TER const preclaimResult;
    FeeUnit64 const baseFee;
    beast::Journal const journal;

    ApplyView&
    view()
    {
        return *view_;
    }

    ApplyView const&
    view() const
    {
        return *view_;
    }

    // VFALCO Unfortunately this is necessary
    RawView&
    rawView()
    {
        return *view_;
    }

    /** Sets the DeliveredAmount field in the metadata */
    void
    deliver(STAmount const& amount)
    {
        view_->deliver(amount);
    }

    /** Discard changes and start fresh. */
    void
    discard();

    /** Apply the transaction result to the base. */
    void apply(TER);

    /** Get the number of unapplied changes. */
    std::size_t
    size();

    /** Visit unapplied changes. */
    void
    visit(std::function<void(
              uint256 const& key,
              bool isDelete,
              std::shared_ptr<SLE const> const& before,
              std::shared_ptr<SLE const> const& after)> const& func);

    void
    destroyXRP(XRPAmount const& fee)
    {
        view_->rawDestroyXRP(fee);
    }

    /** Applies all invariant checkers one by one.

        @param result the result generated by processing this transaction.
        @param fee the fee charged for this transaction
        @return the result code that should be returned for this transaction.
     */
    TER
    checkInvariants(TER const result, XRPAmount const fee);

private:
    TER
    failInvariantCheck(TER const result);

    template <std::size_t... Is>
    TER
    checkInvariantsHelper(
        TER const result,
        XRPAmount const fee,
        std::index_sequence<Is...>);

    OpenView& base_;
    ApplyFlags flags_;
    std::optional<ApplyViewImpl> view_;
};

}  // namespace ripple

#endif
