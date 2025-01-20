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
    int pageId;
    int rowId;

    TID(int page, int row) : pageId(page), rowId(row) {}

    std::string toString() const {
        return std::to_string(pageId) + "," + std::to_string(rowId);
    }

    static TID fromString(const std::string& str) {
        size_t commaPos = str.find(',');
        int page = std::stoi(str.substr(0, commaPos));
        int row = std::stoi(str.substr(commaPos + 1));
        return TID(page, row);
    }

    std::string toString() const {
        return "TID(" + std::to_string(pageId) + "," + std::to_string(rowId) + ")";
    }
};

// Represents the GIN index state

struct GinState {
    bool isBuild;           // Indicates if the index is being built
    size_t maxItemSize;     // Maximum size allowed for an index item

    GinState(bool build = true, size_t maxSize = 8192)
        : isBuild(build), maxItemSize(maxSize) {}
};

// Represents the GIN index tuple

struct IndexTuple {
    std::vector<std::string> datums; // Holds keys and posting list metadata
    size_t postingOffset = 0;        // Offset to the posting list in the tuple
    size_t postingSize = 0;          // Size of the posting list

    // Add a new posting list to the tuple
    void addPostingList(const std::vector<TID>& tids) {
        for (const auto& tid : tids) {
            datums.push_back(tid.toString());
        }
        postingOffset = datums.size() - tids.size();
        postingSize = tids.size();
    }

    // Retrieve the current posting list from the tuple
    std::vector<TID> getPostingList() const {
        std::vector<TID> postingList;
        for (size_t i = postingOffset; i < postingOffset + postingSize; ++i) {
            postingList.push_back(TID::fromString(datums[i])); // Convert string back to TID
        }
        return postingList;
    }

    // Get the size of the tuple in bytes (approximation)
    size_t getTupleSize() const {
        return datums.size() * sizeof(std::string) + sizeof(postingOffset) + sizeof(postingSize);
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
