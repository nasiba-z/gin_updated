// main_art.cpp
#include <iostream>
#include <vector>
#include <tuple>
#include <unordered_map>
#include <map>
#include <set>
#include <algorithm>
#include "read_db.h"               // Contains read_db() function.
#include "gin_index_art.h"             // Contains GinIndex, GinFormTuple, IndexTuple, etc.
#include "gin_state.h"             // Contains GinState definition.
#include "tid.h"                   // Contains TID definition.
#include "trigram_generator.h"     // Contains trigram_generator() function.
#include "art.h"                   // Contains ART node classes.

using namespace std;

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

    // 3. Aggregate posting data by trigram key.
    // We build a map from trigram (string) to a vector of TIDs.
    unordered_map<string, vector<TID>> postingMap;
    int processedRows = 0;
    for (const auto& trow : table) {
        // Assume trigram_generator returns a set<string> of unique trigrams for the row.
        set<string> trigramSet = trigram_generator(trow.data);
        for (const auto& tri : trigramSet) {
            postingMap[tri].push_back(TID(trow.id));
        }
        processedRows++;
        if (processedRows % 10000 == 0) {
            cout << "Processed " << processedRows << " rows." << endl;
        }
    }
    
    cout << "Number of unique trigrams: " << postingMap.size() << endl;
    GinState state(true, 256); // Reduced maxItemSize for testing posting trees.

    // 4. Create IndexTuples from each posting list.
    // Each IndexTuple holds the trigram key (as a string) and a pointer
    // to the posting data (either inline or as a posting tree).
    vector<IndexTuple*> tuples;
    // We assume GinFormTuple takes (GinState*, datum, postingData, errorTooBig)
    // where here we pass the trigram (string) as datum.
    // (For testing, GinState can be null or a dummy.)
    for (auto& entry : postingMap) {
        sort(entry.second.begin(), entry.second.end(), [](const TID& a, const TID& b) {
            return a.rowId < b.rowId;
        });
    }
    std::map<datum, vector<TID>> sortedPostingMap(postingMap.begin(), postingMap.end());

    for (const auto& kv : sortedPostingMap) {
        const string& trigram = kv.first;
        const vector<TID>& tids = kv.second;
        IndexTuple* tup = GinFormTuple_ART(&state, trigram, tids, true);
        if (tup != nullptr)
            tuples.push_back(tup);
        // Print a counter each 1000th key
        static int counter = 0;
        counter++;
        if (counter % 1000 == 0) {
            cout << "Inserted key: " << trigram << " with " << tids.size() << " TIDs." << endl;
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

    // 5. Convert the IndexTuples into ART items.
    // Each ART item is a pair: { key (vector<unsigned char>), value (IndexTuple*) }.
    // We convert the trigram string (tup->key) into a vector of unsigned char.
    vector<pair<vector<unsigned char>, IndexTuple*>> artItems;
    for (const auto* tup : tuples) {
        // Here we assume tup->key is a string.
        string trigram = tup->key;
        vector<unsigned char> keyVec(trigram.begin(), trigram.end());
        artItems.push_back({keyVec, const_cast<IndexTuple*>(tup)});
    }
    // Sort artItems lexicographically by key.
    sort(artItems.begin(), artItems.end(), [](const auto &p1, const auto &p2) {
        return p1.first < p2.first;
    });

    // 6. Bulk-load the ART tree using ART_bulkLoad.
    // The resulting ART tree maps each trigram (as a byte vector)
    // to the corresponding IndexTuple pointer (which holds the posting list/tree).
    ARTNode* artRoot = ART_bulkLoad(artItems, 0);
    // ARTNode* artRoot = ART_bulkLoad(artItems, 0);
    cout << "ART tree built via bulk loading on " << artItems.size() << " unique trigrams." << endl;

    // 7. Test search on the ART tree.
    // if (!artItems.empty()) {
    //     const auto &firstKey = artItems.front().first;
    //     // We assume that ARTLeaf now stores an IndexTuple* as its value,
    //     // so the search returns a pointer to that value.
    //     IndexTuple** foundTuple = reinterpret_cast<IndexTuple**>(artRoot->search(firstKey.data(), firstKey.size(), 0));
    //     if (foundTuple && *foundTuple) {
    //         cout << "Search found key \"";
    //         for (auto ch : firstKey)
    //             cout << ch;
    //         cout << "\" with posting size " << (*foundTuple)->postingSize << endl;
    //     } else {
    //         cout << "Search did not find the first key." << endl;
    //     }
    // }

    // Additional tests: search for some specific trigrams.
    vector<string> testTrigrams = {"cho", "yel", "low"};
    for (const auto& tri : testTrigrams) {
        vector<unsigned char> keyVec(tri.begin(), tri.end());
        IndexTuple** result = reinterpret_cast<IndexTuple**>(artRoot->search(keyVec.data(), keyVec.size(), 0));
        if (result && *result) {
            cout << "Trigram \"" << tri << "\" found with posting size " << (*result)->postingSize << endl;
        } 
        else {
            cout << "Trigram \"" << tri << "\" not found." << endl;
        }
    }

    // 8. Cleanup.
    
    // Depending on ownership semantics, you may need to delete the IndexTuples.
    // For this test, assume the ART tree has taken ownership.
    for (auto* tup : tuples) {
        if (tup->postingTree)
            delete tup->postingTree;
        delete tup;
    }
    delete artRoot;
    return 0;
}
