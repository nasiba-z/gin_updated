#ifndef GINBUILD_H
#define GINBUILD_H

#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <set>
#include <stdexcept>
#include "gin_extract_value_trgm.h"

// ItemPointer equivalent
struct TID {
    int blockNumber;  // Block number in the simulated database
    int rowNumber;    // Row number in the block

    std::string toString() const {
        return "TID(" + std::to_string(blockNumber) + "," + std::to_string(rowNumber) + ")";
    }
};

// Represents the GIN index state
struct GinState {
    bool oneCol;        // Single-column index flag
    size_t maxItemSize; // Maximum allowed index tuple size
};

// Represents an index tuple
struct IndexTuple {
    std::vector<std::string> datums; // Holds keys and posting list metadata
    size_t postingOffset = 0;        // Offset to the posting list in the tuple
    size_t postingSize = 0;          // Size of the posting list

    void addPostingList(const std::vector<TID>& tids) {
        for (const auto& tid : tids) {
            datums.push_back(tid.toString());
        }
        postingOffset = datums.size() - tids.size();
        postingSize = tids.size();
    }
};

// Represents a row in the table
struct TableRow {
    int id;               // Row ID
    std::string data;     // The column data
};

using Table = std::vector<TableRow>;                        // Table representation
using KeyToPostingList = std::unordered_map<std::string, std::vector<TID>>; // GIN index map

/**
 * Constructs an IndexTuple from a key and its associated posting list.
 *
 * @param ginstate The GIN index state.
 * @param key The key for the index tuple.
 * @param postingList The posting list associated with the key.
 * @return The constructed IndexTuple.
 * @throws std::runtime_error If the index row size exceeds the maximum allowed size.
 */
IndexTuple ginFormTupleCpp(const GinState& ginstate, const std::string& key, const std::vector<TID>& postingList);

/**
 * Builds GIN index tuples from the given table using the provided GIN state.
 *
 * @param table The table containing rows to index.
 * @param ginstate The GIN index state.
 * @return A vector of constructed GIN IndexTuples.
 */
std::vector<IndexTuple> ginBuild(const Table& table, const GinState& ginstate);

#endif // GINBUILD_H
