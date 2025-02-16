#include <iostream>
#include <vector>
#include <tuple>
#include <algorithm>
#include "read_db.h"       // Contains read_db() function.
#include "gin_index.h"     // Contains GinFormTuple(), IndexTuple, etc.
#include "gin_state.h"     // Contains GinState definition.
#include "tid.h"           // Contains TID definition.
#include "trigram_generator.h"  // Contains trgm2int() function.
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
    GinState state(true, 21); // Reduced the maxItemSize as the data was much smaller in order to have any posting trees.

    // 4. Create a GinIndex object.
    GinIndex gin(state);

    // 5. For each TableRow, extract trigram keys from p_name and build an IndexTuple.
    // For testing, we simply take the first trigram from each TableRow.
    vector<IndexTuple*> tuples;
    for (const auto& trow : table) {
        int32_t nentries = 0;
        auto trigramArray = trgm2int(trow.data, &nentries);
        if (!trigramArray || trigramArray->empty()) {
            continue;
        }
        // Process every trigram for this row.
        for (datum trigramKey : *trigramArray) {
            // Create posting data: a vector containing one TID, using trow.id.
            vector<TID> postingData = { TID(trow.id) };
            // Insert into the GinIndex. This should merge with any existing posting list for the same trigram.
            gin.insert(trigramKey, postingData);
        }
    }

    // 5. Print the results for each IndexTuple.
    // for (const auto* tup : tuples) {
    //     if (tup->postingTree) {
    //         cout << "IndexTuple key: " << tup->key << "\n";
    //         cout << "Posting Size (number of TIDs): " << tup->postingSize << "\n";
    //         cout << "Posting tree present. Total tree size (simulated): " 
    //              << tup->postingTree->getTotalSize() << " bytes.\n";
    //         cout << "-------------------------\n";
    //     }
    // }
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

    // 6. Cleanup: Delete all allocated IndexTuples and associated PostingTrees.
    for (auto* tup : tuples) {
        if (tup->postingTree)
            delete tup->postingTree;
        delete tup;
    }

    return 0;
}
