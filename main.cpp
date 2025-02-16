#include <iostream>
#include <vector>
#include <string>
#include <tuple>
#include <memory>
#include "trigram_generator.h"
#include "read_db.h"
#include "gin_index.h"
#include "gin_state.h"
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
    EntryTree entryTree(2);
    for (auto& tuple : tuples) {
        if (!tuple.datums.empty())
            entryTree.insert(tuple.datums[0], &tuple);
    }

    return 0;
}