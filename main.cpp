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

    // 3. Create a GinState with desired parameters.
    GinState state(true, 256); // Reduced maxItemSize for testing posting trees.

    // 4. Create a GinIndex object.
    GinIndex gin(state);

    // 5. Aggregate posting data by trigram key.
    // We'll use an unordered_map where key is a trigram (datum)
    // and the value is a vector of TIDs (row IDs) for that trigram.
    unordered_map<datum, vector<TID>> postingMap;

    for (const auto& trow : table) {
        int32_t nentries = 0;
        // trgm2int() returns a unique_ptr to a vector of datum (trigram keys).
        auto trigramArray = trgm2int(trow.data, &nentries);
        if (!trigramArray || trigramArray->empty()) {
            continue;
        }
        // Process every trigram for this row.
        for (datum trigramKey : *trigramArray) {
            // Append the row's TID (constructed from trow.id) to the vector for this trigram.
            // Here we use TID(trow.id) because your TID takes one integer.
            postingMap[trigramKey].push_back(TID(trow.id));
        }
        for (const auto& key : *trigramArray) {
            // Sorting row.ids for each trigram key
            std::sort(postingMap[key].begin(), postingMap[key].end(), [](const TID& a, const TID& b) {
                return a.rowId < b.rowId;
            });

    }}

    
    std::map<datum, vector<TID>> sortedPostingMap(postingMap.begin(), postingMap.end());
    // cout << "Number of unique trigrams: " << sortedPostingMap.size() << endl;

    
    // Debug: Print the number of unique trigrams
    // for (const auto& entry : postingMap) {
    //     cout << "Trigram: " << entry.first << " -> TIDs: ";
    //     for (const auto& tid : entry.second) {
    //         cout << tid.rowId << " ";
    //     }
    //     cout << endl;
    // }

    
    //6. Form final IndexTuples from the GinIndex.
    vector<IndexTuple*> tuples;
    for (const auto& kv : sortedPostingMap) {
        datum key = kv.first;
        const vector<TID>& tids = kv.second;
        IndexTuple* tup = GinFormTuple(&state, key, tids, true);
        if (tup != nullptr)
            tuples.push_back(tup);
        cout << "Inserted key: " << key << " with " << tids.size() << " TIDs.\n";
    }
    // Debug: Print the number of IndexTuples formed
    cout << "Number of IndexTuples formed: " << tuples.size() << endl;

    // 7. Print the final IndexTuples.
    for (const auto* tup : tuples) {
        cout << "IndexTuple key: " << tup->key << "\n";
        cout << "Posting Size (number of TIDs): " << tup->postingSize << "\n";
        if (tup->postingList) {
            cout << "Inline Posting List TIDs: ";
            for (const auto& tid : tup->postingList->tids) {
                cout << "(" << tid.rowId << ") ";
            }
            cout << "\n";
        } else if (tup->postingTree) {
            cout << "Posting tree present. Total tree size (simulated): " 
                 << tup->postingTree->getTotalSize() << " bytes.\n";
        }
        cout << "-------------------------\n";
    }

    // // 8. Cleanup: Delete all allocated IndexTuples and associated PostingTrees.
    for (auto* tup : tuples) {
        if (tup->postingTree)
            delete tup->postingTree;
        delete tup;
    }

     return 0;

}
