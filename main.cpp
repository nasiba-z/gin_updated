#include <iostream>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <map>
#include <algorithm>
#include "read_db.h"               // Contains read_db() function.
#include "gin_index.h"             // Contains GinIndex, GinFormTuple, IndexTuple, etc.
#include "gin_state.h"             // Contains GinState definition.
#include "tid.h"                   // Contains TID definition.
#include "trigram_generator.h"     // Contains trgm2int() function.
#include "entry_tree.h"            // Contains EntryTree definition.
#include "posting_tree.h"          // Contains PostingTree definition.

using namespace std;

// Helper function to recursively print the entry tree.
void printEntryTree(EntryTreeNode* node, int depth = 0) {
    if (!node) return;
    // Indentation for readability.
    for (int i = 0; i < depth; i++) cout << "  ";
    if (node->leaf) {
        cout << "Leaf: ";
        for (const auto &k : node->keys) {
            cout << k << " ";
        }
        cout << "\n";
    } else {
        cout << "Internal: ";
        for (const auto &k : node->keys) {
            cout << k << " ";
        }
        cout << "\n";
        for (auto child : node->children) {
            printEntryTree(child, depth + 1);
        }
    }
}

// Helper function to compute the depth of leaves (should be same for all leaves)
void computeLeafDepth(EntryTreeNode* node, int currentDepth, vector<int>& depths) {
    if (!node) return;
    if (node->leaf) {
        depths.push_back(currentDepth);
    } else {
        for (auto child : node->children) {
            computeLeafDepth(child, currentDepth + 1, depths);
        }
    }
}
int main() {
    // 1. Read the database rows from file "part.tbl".
    vector<Row> database = read_db("part.tbl");

    // 2. Transform the database rows into TableRow format.
    vector<TableRow> table;
    for (const auto& row : database) {
        int id = std::get<0>(row);          // p_partkey
        string data = std::get<1>(row);       // p_name
        table.push_back({id, data});
    }

    // 3. Create a GinState with desired parameters.
    GinState state(true, 256); // Reduced maxItemSize for testing posting trees.

    

    // 5. Aggregate posting data by trigram key.
    unordered_map<datum, vector<TID>> postingMap;
    int processedRows = 0;
    for (const auto& trow : table) {
        int32_t nentries = 0;
        auto trigramArray = trgm2int(trow.data, &nentries);
        if (!trigramArray || trigramArray->empty()) {
            continue;
        }
        // Process every trigram for this row.
        for (datum trigramKey : *trigramArray) {
            // Append the row's TID (constructed from trow.id) to the vector for this trigram.
            postingMap[trigramKey].push_back(TID(trow.id));
        }
        // processedRows++;
        // if (processedRows % 10000 == 0) {
        //     cout << "Processed " << processedRows << " rows." << endl;
        // }
    }
    // Now sort each posting list once.
    for (auto& entry : postingMap) {
        sort(entry.second.begin(), entry.second.end(), [](const TID& a, const TID& b) {
            return a.rowId < b.rowId;
        });
    }

    // 6. Convert postingMap to a sorted map for debugging.
    std::map<datum, vector<TID>> sortedPostingMap(postingMap.begin(), postingMap.end());

    // Debug: Print the number of unique trigrams.
    // cout << "Number of unique trigrams: " << sortedPostingMap.size() << endl;
    // for (const auto& entry : sortedPostingMap) {
    //     cout << "Trigram: " << entry.first << " -> TIDs: ";
    //     for (const auto& tid : entry.second) {
    //         cout << tid.rowId << " ";
    //     }
    //     cout << endl;
    // }

    // 7. Form final IndexTuples from the GinIndex.
    vector<IndexTuple*> tuples;
    for (const auto& kv : sortedPostingMap) {
        datum key = kv.first;
        const vector<TID>& tids = kv.second;
        IndexTuple* tup = GinFormTuple(&state, key, tids, true);
        if (tup != nullptr)
            tuples.push_back(tup);

        // Print a counter each 1000th key
        static int counter = 0;
        counter++;
        if (counter % 1000 == 0) {
            cout << "Inserted key: " << key << " with " << tids.size() << " TIDs." << endl;
        }
    }

    // 8. Print the final IndexTuples.
    for (const auto* tup : tuples) {
        cout << "IndexTuple key: " << tup->key << "\n";
        cout << "Posting Size (number of TIDs): " << tup->postingSize << "\n";
        if (tup->postingList) {
            // cout << "Inline Posting List TIDs: ";
            // for (const auto& tid : tup->postingList->tids) {
            //     cout << "(" << tid.rowId << ") ";
            // }
            // cout << "\n";
        } else if (tup->postingTree) {
            cout << "Posting tree present. Total tree size (simulated): " 
                 << tup->postingTree->getTotalSize() << " bytes.\n";
        }
        cout << "-------------------------\n";
    }
    cout << "Number of IndexTuples formed: " << tuples.size() << endl;
    // ---------------------------------------------------------------------
    // Additional Test: Build Posting Trees via Incremental Insertion.
    // For a few keys that have a substantial number of TIDs, we'll build a posting tree
    // by inserting TIDs one-by-one (standard building) and print its simulated size.
    // ---------------------------------------------------------------------
    // cout << "\nTesting incremental insertion for posting trees:\n";
    // int testCount = 0;
    // for (const auto& kv : sortedPostingMap) {
    //     const datum key = kv.first;
    //     const vector<TID>& tids = kv.second;
    //     // Only test keys with more than 7 TIDs (adjust threshold as desired)
    //     if (tids.size() > 7) {
    //         PostingTree incrementalTree;
    //         for (const TID& tid : tids) {
    //             incrementalTree.insert(tid);
    //         }
    //         cout << "Key " << key 
    //              << " (with " << tids.size() << " TIDs) incremental tree built. "
    //              << "Total tree size (simulated): " 
    //              << incrementalTree.getTotalSize() << " bytes.\n";
    //         testCount++;
    //     }
    //     if (testCount >= 5) // test only the first 5 keys for brevity
    //         break;
    // }


    // 9. Build and Test EntryTree
    // For the EntryTree, we assume that each IndexTuple->key is an int32_t.
    // First, sort the tuples by key.
    vector<IndexTuple*> sortedTuples = tuples;
    sort(sortedTuples.begin(), sortedTuples.end(), [](const IndexTuple* a, const IndexTuple* b) {
        return a->key < b->key;
    });

    // Build the entry tree using bulk-loading.
    // Initial Build of Entry Tree:
    // GIN uses a bulk-loading method—extracting, de-duplicating, and sorting the keys, 
    // then building the tree bottom-up.

    // Subsequent Updates:
    // For online operations, incremental insertion is used if new keys need to be added. 
    // However, the initial creation of the entry tree is entirely based on bulk loading.

    // References in the PostgreSQL Source Code:

    // In ginbuild() and ginHeapTupleBulkInsert() in src/backend/access/gin/gininsert.c, 
    //  the system accumulates entries and later processes them in bulk.
    // The routines that build the entry tree are relatively straightforward compared to the 
    // more complex handling of posting trees (see ginpostingtree.c), because the entry tree only needs 
    // to maintain a sorted, static list of keys with pointers to posting data.
    // This separation of concerns (bulk-loading for static keys in the entry tree and a hybrid approach 
    // for large posting lists) is one of the key design decisions in the GIN index that allows it to be 
    // both efficient and scalable.

    EntryTree entryTree;
    entryTree.bulkLoad(sortedTuples);
    cout << "EntryTree built. Total keys in tree: " << entryTree.getTotalSize() << endl;
    // Test: search for key 
    if (!sortedTuples.empty()) {
        int32_t searchKey = 7106423;
        if (entryTree.search(searchKey))
            cout << "Search for key " << searchKey << " succeeded." << endl;
        else
            cout << "Search for key " << searchKey << " failed." << endl;
    }
    // 10 Check that all leaves are at the same depth.
    vector<int> leafDepths;
    computeLeafDepth(entryTree.root, 0, leafDepths);
    if (!leafDepths.empty()) {
        int expectedDepth = leafDepths.front();
        bool balanced = all_of(leafDepths.begin(), leafDepths.end(), [expectedDepth](int d) {
            return d == expectedDepth;
        });
        cout << "All leaves are at depth " << expectedDepth 
             << (balanced ? " (balanced)" : " (not balanced)") << endl;
    }

    // 11. Cleanup: Delete all allocated IndexTuples and associated PostingTrees.
    for (auto* tup : tuples) {
        if (tup->postingTree)
            delete tup->postingTree;
        delete tup;
    }

    return 0;
}