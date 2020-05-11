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

#include <ripple/basics/Log.h>
#include <ripple/basics/contract.h>
#include <ripple/core/DatabaseCon.h>
#include <ripple/core/SociDB.h>
#include <boost/algorithm/string.hpp>
#include <boost/format.hpp>
#include <memory>

namespace ripple {

DatabaseCon::Setup
setup_DatabaseCon(Config const& c)
{
    DatabaseCon::Setup setup;

    setup.startUp = c.START_UP;
    setup.standAlone = c.standalone();
    setup.dataDir = c.legacy("database_path");
    if (!setup.standAlone && setup.dataDir.empty())
    {
        Throw<std::runtime_error>("database_path must be set.");
    }

    if (!setup.CommonPragma)
    {
        setup.CommonPragma = [&c]() {
            auto const& sqlite = c.section("sqlite");
            auto result = std::make_unique<std::vector<std::string>>();
            result->reserve(3);

            // defaults
            std::string safety_level = "high";
            std::string journal_mode = "wal";
            std::string synchronous = "normal";
            std::string temp_store = "file";

            if (c.LEDGER_HISTORY < SQLITE_TUNING_CUTOFF &&
                set(safety_level, "safety_level", sqlite))
            {
                if (boost::iequals(safety_level, "low"))
                {
                    // low safety defaults
                    journal_mode = "memory";
                    synchronous = "off";
                    temp_store = "memory";
                }
                else if (!boost::iequals(safety_level, "high"))
                {
                    Throw<std::runtime_error>(
                        "Invalid safety_level value: " + safety_level);
                }
            }

            // #journal_mode Valid values : delete, truncate, persist, memory,
            // wal, off
            if (c.LEDGER_HISTORY < SQLITE_TUNING_CUTOFF)
                set(journal_mode, "journal_mode", sqlite);
            if (boost::iequals(journal_mode, "delete") ||
                boost::iequals(journal_mode, "truncate") ||
                boost::iequals(journal_mode, "persist") ||
                boost::iequals(journal_mode, "memory") ||
                boost::iequals(journal_mode, "wal") ||
                boost::iequals(journal_mode, "off"))
            {
                result->emplace_back(boost::str(
                    boost::format(CommonDBPragmaJournal) % journal_mode));
            }
            else
            {
                Throw<std::runtime_error>(
                    "Invalid journal_mode value: " + journal_mode);
            }

            //#synchronous Valid values : off, normal, full, extra
            if (c.LEDGER_HISTORY < SQLITE_TUNING_CUTOFF)
                set(synchronous, "synchronous", sqlite);
            if (boost::iequals(synchronous, "off") ||
                boost::iequals(synchronous, "normal") ||
                boost::iequals(synchronous, "full") ||
                boost::iequals(synchronous, "extra"))
            {
                result->emplace_back(boost::str(
                    boost::format(CommonDBPragmaSync) % synchronous));
            }
            else
            {
                Throw<std::runtime_error>(
                    "Invalid synchronous value: " + synchronous);
            }

            // #temp_store Valid values : default, file, memory
            if (c.LEDGER_HISTORY < SQLITE_TUNING_CUTOFF)
                set(temp_store, "temp_store", sqlite);
            if (boost::iequals(temp_store, "default") ||
                boost::iequals(temp_store, "file") ||
                boost::iequals(temp_store, "memory"))
            {
                result->emplace_back(
                    boost::str(boost::format(CommonDBPragmaTemp) % temp_store));
            }
            else
            {
                Throw<std::runtime_error>(
                    "Invalid temp_store value: " + temp_store);
            }

            assert(result->size() == 3);
            return result;
        }();
    }

    return setup;
}

std::unique_ptr<std::vector<std::string> const>
    DatabaseCon::Setup::CommonPragma;
const std::vector<std::string> DatabaseCon::Setup::NoCommonPragma;

void
DatabaseCon::setupCheckpointing(JobQueue* q, Logs& l)
{
    if (!q)
        Throw<std::logic_error>("No JobQueue");
    checkpointer_ = makeCheckpointer(session_, *q, l);
}

}  // namespace ripple
