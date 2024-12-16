#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <set>
#include <stdexcept>
#include "gin_extract_value_trgm.h"
#include "ginbuild.h"



// GIN tuple construction
IndexTuple ginFormTupleCpp(const GinState& ginstate, const std::string& key, const std::vector<TID>& postingList) {
    IndexTuple tuple;
    tuple.datums.push_back(key);

    if (!postingList.empty()) {
        tuple.addPostingList(postingList);

        size_t tupleSize = tuple.datums.size() * sizeof(std::string);
        if (tupleSize > ginstate.maxItemSize) {
            throw std::runtime_error("Index row size exceeds maximum limit");
        }
    }

    return tuple;
}

std::vector<IndexTuple> ginBuild(const Table& table, const GinState& ginstate) {
    KeyToPostingList index; // GIN index map
    std::vector<IndexTuple> tuples; // Resulting GIN tuples

    // Iterate over the table rows
    for (const auto& row : table) {
        // Use gin_extract_value_trgm to get trigram integers
        int32_t numTrigrams = 0;
        auto trigramIntegers = gin_extract_value_trgm(row.data, &numTrigrams);

        if (!trigramIntegers || numTrigrams == 0) {
            // Skip rows with no trigrams
            continue;
        }

        // Add each trigram integer to the index with the current row's TID
        for (int i = 0; i < numTrigrams; ++i) {
            int32_t trigram = (*trigramIntegers)[i];
            TID tid{0, row.id}; // Simulate TID
            std::string trigramKey = std::to_string(trigram);

            // Add to the index
            index[trigramKey].push_back(tid);
        }
    }

    // Build GIN tuples from the index
    for (const auto& pair : index) {
        const std::string& key = pair.first;
        const std::vector<TID>& postingList = pair.second;

        IndexTuple tuple = ginFormTupleCpp(ginstate, key, postingList);
        tuples.push_back(tuple);
    }


    return tuples;
}