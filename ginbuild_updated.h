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
//store compressed lists of TIDs to optimize storage
struct GinPostingList {
    std::vector<TID> tids; // The list of TIDs
    size_t maxSize;        // Maximum allowed size for this posting list

    // Constructor
    GinPostingList(size_t max = 256) : maxSize(max) {}

    // Add a TID to the posting list
    void addTID(const TID& tid) {
        if (tids.size() < maxSize) {
            tids.push_back(tid);
        } else {
            throw std::runtime_error("Exceeded maximum size of posting list");
        }
    }

    // Compress the posting list (dummy compression for demonstration)
    std::string compress() const {
        std::string compressed;
        for (const auto& tid : tids) {
            compressed += tid.toString() + ";"; // Example of simple serialization
        }
        return compressed; // This could be replaced with real compression logic
    }

    // Decompress the posting list
    static GinPostingList decompress(const std::string& compressed) {
        GinPostingList list;
        size_t start = 0;
        size_t end = compressed.find(';');
        while (end != std::string::npos) {
            std::string tidStr = compressed.substr(start, end - start);
            list.tids.push_back(TID::fromString(tidStr));
            start = end + 1;
            end = compressed.find(';', start);
        }
        return list;
    }

    // Get size in bytes
    size_t getSizeInBytes() const {
        return tids.size() * sizeof(TID);
    }

    // Check if the list is full
    bool isFull() const {
        return tids.size() >= maxSize;
    }
};

// Represents an index tuple
struct IndexTuple {
    std::vector<std::string> datums; // Holds keys and metadata
    size_t postingOffset = 0;        // Offset to the posting list in the tuple
    size_t postingSize = 0;          // Size of the posting list
    GinPostingList postingList;      // Compressed posting list

    // Add a new posting list to the tuple
    void addPostingList(const GinPostingList& list) {
        // Store the compressed posting list
        postingList = list;

        // Add TIDs from the posting list to datums
        for (const auto& tid : list.tids) {
            datums.push_back(tid.toString());
        }

        // Update offset and size
        postingOffset = datums.size() - list.tids.size();
        postingSize = list.tids.size();
    }

    // Retrieve the current posting list as a decompressed list of TIDs
    std::vector<TID> getPostingListAsTIDs() const {
        return postingList.tids;
    }

    // Get the size of the tuple in bytes
    size_t getTupleSize() const {
        size_t datumsSize = datums.size() * sizeof(std::string);
        size_t postingListSize = postingList.getSizeInBytes();
        return datumsSize + sizeof(postingOffset) + sizeof(postingSize) + postingListSize;
    }

    // Print tuple for debugging
    void printTuple() const {
        std::cout << "Datums: ";
        for (const auto& datum : datums) {
            std::cout << datum << " ";
        }
        std::cout << "\nPosting Offset: " << postingOffset
                  << "\nPosting Size: " << postingSize
                  << "\nPosting List (compressed): " << postingList.compress() << "\n";
    }
};

// Represents a page in the GIN index
struct GinPage {
    std::vector<IndexTuple> tuples;  // Tuples stored in this page
    std::vector<GinPostingList> postingLists; // Posting lists stored in this page
    GinPage* rightLink = nullptr;         // Pointer to the next page (for linked list of pages)

    size_t getCurrentSize() const {
        size_t size = 0;
        for (const auto& tuple : tuples) {
            size += tuple.getTupleSize(); // Use the size calculation from IndexTuple
        }
        for (const auto& postingList : postingLists) {
            size += postingList.getSizeInBytes(); // Include size of posting lists
        }
        return size;
    }
     // Calculate the free space available in the page
    size_t getFreeSpace(size_t maxPageSize) const {
        size_t currentSize = getCurrentSize();
        return (currentSize < maxPageSize) ? (maxPageSize - currentSize) : 0;
    }
    // Add a posting list to the page
    void addPostingList(const GinPostingList& postingList) {
        postingLists.push_back(postingList);
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
std::vector<GinPage> ginBuild_updated(const Table& table, const GinState& ginstate);
#endif // GINBUILD_H
