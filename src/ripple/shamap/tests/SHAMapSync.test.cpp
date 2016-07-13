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
#include <ripple/shamap/SHAMap.h>
#include <ripple/shamap/SHAMapItem.h>
#include <ripple/shamap/tests/common.h>
#include <ripple/basics/random.h>
#include <ripple/basics/StringUtilities.h>
#include <ripple/beast/unit_test.h>

namespace ripple {
namespace tests {

class sync_test : public beast::unit_test::suite
{
public:
    static std::shared_ptr<SHAMapItem> makeRandomAS ()
    {
        Serializer s;

        for (int d = 0; d < 3; ++d)
            s.add32 (rand_int<std::uint32_t>());

        return std::make_shared<SHAMapItem>(
            s.getSHA512Half(), s.peekData ());
    }

    bool confuseMap (SHAMap& map, int count)
    {
        // add a bunch of random states to a map, then remove them
        // map should be the same
        SHAMapHash beforeHash = map.getHash ();

        std::list<uint256> items;

        for (int i = 0; i < count; ++i)
        {
            std::shared_ptr<SHAMapItem> item = makeRandomAS ();
            items.push_back (item->key());

            if (!map.addItem (std::move(*item), false, false))
            {
                log << "Unable to add item to map\n";
                return false;
            }
        }

        for (auto const& item : items)
        {
            if (!map.delItem (item))
            {
                log << "Unable to remove item from map\n";
                return false;
            }
        }

        if (beforeHash != map.getHash ())
        {
            log <<
                "Hashes do not match " << beforeHash <<
                " " << map.getHash () << std::endl;
            return false;
        }

        return true;
    }

    void run()
    {
        log << "Run, version 1\n" << std::endl;
        run(SHAMap::version{1});

        log << "Run, version 2\n" << std::endl;
        run(SHAMap::version{2});
    }

    void run(SHAMap::version v)
    {
        beast::Journal const j; // debug journal
        TestFamily f(j), f2(j);
        SHAMap source (SHAMapType::FREE, f, v);
        SHAMap destination (SHAMapType::FREE, f2, v);

        int items = 10000;
        for (int i = 0; i < items; ++i)
        {
            source.addItem (std::move(*makeRandomAS ()), false, false);
            if (i % 100 == 0)
                source.invariants();
        }

        source.invariants();
        expect (confuseMap (source, 500), "ConfuseMap");
        source.invariants();

        source.setImmutable ();

        int count = 0;
        source.visitLeaves([&count](auto const& item)
            {
                ++count;
            });
        expect(count == items, "These must be equal");

        std::vector<SHAMapMissingNode> missingNodes;
        source.walkMap(missingNodes, 2048);
        expect(missingNodes.empty(), "should be empty");

        std::vector<SHAMapNodeID> nodeIDs, gotNodeIDs;
        std::vector< Blob > gotNodes;
        std::vector<uint256> hashes;

        std::vector<SHAMapNodeID>::iterator nodeIDIterator;
        std::vector< Blob >::iterator rawNodeIterator;

        destination.setSynching ();

        {
            std::vector<SHAMapNodeID> gotNodeIDs_a;
            std::vector<Blob> gotNodes_a;

            expect (source.getNodeFat (
                SHAMapNodeID (),
                gotNodeIDs_a,
                gotNodes_a,
                rand_bool(),
                rand_int(2)), "getNodeFat (1)");

            unexpected (gotNodes_a.size () < 1, "NodeSize");

            expect (destination.addRootNode (
                source.getHash(),
                *gotNodes_a.begin (),
                snfWIRE,
                nullptr).isGood(), "addRootNode");
        }

        do
        {
            f.clock().advance(std::chrono::seconds(1));

            // get the list of nodes we know we need
            auto nodesMissing = destination.getMissingNodes (2048, nullptr);

            if (nodesMissing.empty ())
                break;

            // get as many nodes as possible based on this information
            std::vector<SHAMapNodeID> gotNodeIDs_b;
            std::vector<Blob> gotNodes_b;

            for (auto& it : nodesMissing)
            {
                expect (source.getNodeFat (
                    it.first,
                    gotNodeIDs_b,
                    gotNodes_b,
                    rand_bool(),
                    rand_int(2)), "getNodeFat (2)");
            }

            expect (gotNodeIDs_b.size () == gotNodes_b.size (), "Size mismatch");
            expect (!gotNodeIDs_b.empty (), "Didn't get NodeID");

            for (std::size_t i = 0; i < gotNodeIDs_b.size(); ++i)
            {
                expect (
                    destination.addKnownNode (
                        gotNodeIDs_b[i],
                        gotNodes_b[i],
                        nullptr).isUseful (), "addKnownNode");
            }
        }
        while (true);

        destination.clearSynching ();

        expect (source.deepCompare (destination), "Deep Compare");

        std::cerr << "Checking destination invariants" << std::endl;
        destination.invariants();
    }
};

BEAST_DEFINE_TESTSUITE(sync,shamap,ripple);

} // tests
} // ripple
