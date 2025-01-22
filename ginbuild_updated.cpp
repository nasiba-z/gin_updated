#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <set>
#include <stdexcept>
#include "gin_extract_value_trgm.h"
#include "ginbuild_updated.h"



// GIN tuple construction
IndexTuple ginFormTupleCpp(const GinState& ginstate, const std::string& key, const std::vector<TID>& postingList) {
    IndexTuple tuple;
    tuple.datums.push_back(key);

    if (!postingList.empty()) {
        // Convert the std::vector<TID> to GinPostingList
        GinPostingList ginPostingList;
        for (const auto& tid : postingList) {
            ginPostingList.addTID(tid); // Add each TID to the GinPostingList
        }

        // Add the GinPostingList to the tuple
        tuple.addPostingList(ginPostingList);

        size_t tupleSize = tuple.datums.size() * sizeof(std::string);
        if (tupleSize > ginstate.maxItemSize) {
            throw std::runtime_error("Index row size exceeds maximum limit");
        }
    }

    return tuple;
}
// GIN index construction with page management
std::vector<GinPage> ginBuild_updated(const Table& table, const GinState& ginstate) {
    KeyToPostingList index;         // GIN index map
    std::vector<GinPage> pages;     // Collection of GIN pages
    GinPage currentPage;            // Current page being built

    // Iterate over the table rows to extract trigrams and build the index map
    for (const auto& row : table) {
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

    // Build GIN pages from the index
    for (const auto& pair : index) {
        const std::string& key = pair.first;
        const std::vector<TID>& postingList = pair.second;

        IndexTuple tuple = ginFormTupleCpp(ginstate, key, postingList);

        // Check if the current page can accommodate the tuple
        if (currentPage.getCurrentSize() + tuple.datums.size() * sizeof(std::string) +
            tuple.postingSize * sizeof(TID) > ginstate.maxItemSize) {
            // Current page is full, split and create a new page
            pages.push_back(currentPage); // Save the current page
            currentPage = GinPage();     // Start a new page
        }

        currentPage.tuples.push_back(tuple); // Add the tuple to the current page
    }

    // Add the last page if it contains any tuples
    if (!currentPage.tuples.empty()) {
        pages.push_back(currentPage);
    }

    return pages;
}