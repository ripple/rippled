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

#ifndef RIPPLE_ORDERBOOKDB_H_INCLUDED
#define RIPPLE_ORDERBOOKDB_H_INCLUDED

namespace ripple {

// VFALCO TODO Add Javadoc comment explaining what this class does
class BookListeners
{
public:
    typedef std::shared_ptr<BookListeners> pointer;

    BookListeners ();
    void addSubscriber (InfoSub::ref sub);
    void removeSubscriber (uint64 sub);
    void publish (Json::Value const& jvObj);

private:
    typedef RippleRecursiveMutex LockType;
    typedef std::lock_guard <LockType> ScopedLockType;
    LockType mLock;

    // VFALCO TODO Use a typedef for the uint64
    //             Use a typedef for the container
    ripple::unordered_map<uint64, InfoSub::wptr> mListeners;
};

//------------------------------------------------------------------------------

// VFALCO TODO Add Javadoc comment explaining what this class does
class OrderBookDB
    : public beast::Stoppable
    , public beast::LeakChecked <OrderBookDB>
{
public:
    explicit OrderBookDB (Stoppable& parent);

    void setup (Ledger::ref ledger);
    void update (Ledger::pointer ledger);
    void invalidate ();

    void addOrderBook(
        Currency const& takerPaysCurrency, Currency const& takerGetsCurrency,
        Account const& takerPaysIssuer, Account const& takerGetsIssuer);

    // return list of all orderbooks that want this issuerID and currencyID
    void getBooksByTakerPays (Account const& issuerID, Currency const& currencyID,
                              std::vector<OrderBook::pointer>& bookRet);
    void getBooksByTakerGets (Account const& issuerID, Currency const& currencyID,
                              std::vector<OrderBook::pointer>& bookRet);

    bool isBookToXRP (Account const& issuerID, Currency const& currencyID);

    BookListeners::pointer getBookListeners (Currency const& currencyPays, Currency const& currencyGets,
            Account const& issuerPays, Account const& issuerGets);

    BookListeners::pointer makeBookListeners (Currency const& currencyPays, Currency const& currencyGets,
            Account const& issuerPays, Account const& issuerGets);

    // see if this txn effects any orderbook
    void processTxn (Ledger::ref ledger, const AcceptedLedgerTx& alTx, Json::Value const& jvObj);

private:
    typedef ripple::unordered_map <Issue, std::vector<OrderBook::pointer>>
            AssetToOrderBook;

    // by ci/ii
    AssetToOrderBook mSourceMap;

    // by co/io
    AssetToOrderBook mDestMap;

    // does an order book to XRP exist
    ripple::unordered_set <Issue> mXRPBooks;

    typedef RippleRecursiveMutex LockType;
    typedef std::lock_guard <LockType> ScopedLockType;
    LockType mLock;

    typedef ripple::unordered_map <Book, BookListeners::pointer>
    BookToListenersMap;

    BookToListenersMap mListeners;

    uint32 mSeq;

};

} // ripple

#endif
