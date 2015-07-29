//------------------------------------------------------------------------------
/*
    This file is part of rippled: https://github.com/ripple/rippled
    Copyright (c) 2014 Ripple Labs Inc.

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
#include <ripple/app/tx/impl/CancelTicket.h>
#include <ripple/basics/Log.h>
#include <ripple/protocol/Indexes.h>
#include <ripple/ledger/View.h>

namespace ripple {

TER
CancelTicket::preflight (PreflightContext const& ctx)
{
#if ! RIPPLE_ENABLE_TICKETS
    if (! (ctx.flags & tapENABLE_TESTING))
        return temDISABLED;
#endif
    return Transactor::preflight(ctx);
}

TER
CancelTicket::doApply ()
{
    uint256 const ticketId = tx().getFieldH256 (sfTicketID);

    // VFALCO This is highly suspicious, we're requiring that the
    //        transaction provide the return value of getTicketIndex?
    SLE::pointer sleTicket = view().peek (keylet::ticket(ticketId));

    if (!sleTicket)
        return tecNO_ENTRY;

    auto const ticket_owner =
        sleTicket->getAccountID (sfAccount);

    bool authorized =
        account_ == ticket_owner;

    // The target can also always remove a ticket
    if (!authorized && sleTicket->isFieldPresent (sfTarget))
        authorized = (account_ == sleTicket->getAccountID (sfTarget));

    // And finally, anyone can remove an expired ticket
    if (!authorized && sleTicket->isFieldPresent (sfExpiration))
    {
        std::uint32_t const expiration = sleTicket->getFieldU32 (sfExpiration);

        if (view().parentCloseTime() >= expiration)
            authorized = true;
    }

    if (!authorized)
        return tecNO_PERMISSION;

    std::uint64_t const hint (sleTicket->getFieldU64 (sfOwnerNode));

    TER const result = dirDelete (ctx_.view (), false, hint,
        getOwnerDirIndex (ticket_owner), ticketId, false, (hint == 0));

    adjustOwnerCount(view(), view().peek(
        keylet::account(ticket_owner)), -1);
    ctx_.view ().erase (sleTicket);

    return result;
}

}
