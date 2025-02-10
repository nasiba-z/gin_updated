#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <memory>
#include "trigram_generator.h"
#include "read_db.h"
#include "gin_index.h"
#include "gin_state.h"
#include "gin_posting_list.h"
#include "entry_tree.h"


int main() {
    // Read the database rows from file "part.tbl"
    std::vector<Row> database = read_db("part.tbl");

    // Transform the database into TableRow format for GIN index building.
    std::vector<TableRow> table;
    for (const auto& row : database) {
        int id = std::get<0>(row);          // p_partkey
        std::string data = std::get<1>(row); // p_name
        table.push_back({id, data});
    }

    // Build the GinIndex from the TableRow data.
    // For each TableRow, extract trigrams from p_name.
    // Each trigram (converted to string) becomes a key;
    // we associate with it a posting list containing one TID: (p_partkey, trigram integer).
    GinIndex gin;
    for (const auto& trow : table) {
        int32_t nentries = 0;
        auto entries = trgm2int(trow.data, &nentries);
        if (entries->empty())
            continue;
        for (int trigram : *entries) {
            std::vector<TID> tids = { TID(trow.id) };
            gin.insert(trigram, tids);
        }
    }

    // Form final IndexTuples from the GinIndex.
    GinState state(true, 8192); // Maximum inline posting list size.
    std::vector<IndexTuple> tuples = ginFormTuple(gin, state);

    // Print out each IndexTuple.
    for (const auto& tuple : tuples) {
        std::cout << "Key: " << (tuple.datums.empty() ? "None" : tuple.datums[0]) << "\n";
        std::cout << "Tuple size (bytes): " << tuple.getTupleSize() << "\n";
        if (tuple.postingTree) {
            std::cout << "Posting Tree: ";
            // For demonstration, print the size of the first posting list stored in the tree.
            if (!tuple.postingTree->segments.empty()) {
                auto plist = tuple.postingTree->segments.front();
                auto tids = decodeGinPostingList(plist);
                std::cout << "Tree posting list contains " << tids.size() << " TIDs. ";
            }
            std::cout << "\n";
        } else if (tuple.postingList) {
            auto tids = decodeGinPostingList(tuple.postingList.get());
            std::cout << "Inline Posting List TIDs: ";
            for (const auto& tid : tids)
                std::cout << "(" << tid.rowId << ") ";
            std::cout << "\n";
        }
        std::cout << "Datums: ";
        for (const auto& d : tuple.datums)
            std::cout << d << " ";
        std::cout << "\n\n";
    }

    // For testing, print two cases where posting trees were created.
    int printed = 0;
    for (const auto& tuple : tuples) {
        if (tuple.postingTree != nullptr) {
            std::cout << "Posting tree for trigram key: " << tuple.datums[0] << "\n";
            if (!tuple.postingTree->segments.empty()) {
                auto plist = tuple.postingTree->segments.front();
                auto tids = decodeGinPostingList(plist);
                std::cout << "Tree posting list TIDs: ";
                for (const auto& tid : tids)
                    std::cout << "(" <<tid.rowId << ") ";
                std::cout << "\n";
            }
            std::cout << "\n";
            printed++;
            if (printed >= 2)
                break;
        }
    }

    // // Cleanup: (In this in-memory example, we rely on destructors for GinIndex;
    // // any posting trees allocated via new should be deleted here.)
    // for (auto& tuple : tuples) {
    //     if (tuple.postingTreeRoot) {
    //         delete tuple.postingTreeRoot;
    //         tuple.postingTreeRoot = nullptr;
    //     }
    // }
    // Build the entry tree (B-tree) using the keys from the IndexTuples.
    // We'll use a minimum degree t=2.
    EntryTree entryTree(2);
    for (auto& tuple : tuples) {
        if (!tuple.datums.empty())
            entryTree.insert(tuple.datums[0], &tuple);
    }

    // For demonstration, search for two keys in the entry tree and print the associated IndexTuple.
    std::vector<std::string> searchKeys = {"1426799312", "2087793379"}; 
    for (const auto& key : searchKeys) {
        IndexTuple* foundNode = entryTree.search(key);
        if (foundNode) {
            std::cout << "Found key in entry tree: " << key << "\n";
            std::cout << "IndexTuple datums: ";
            for (const auto& d : foundNode->datums)
                std::cout << d << " ";
            std::cout << "\n";
        } else {
            std::cout << "Key " << key << " not found in entry tree.\n";
        }
    }


    return 0;
}