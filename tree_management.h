#ifndef TREE_MANAGEMENT_H
#define TREE_MANAGEMENT_H

#include <vector>
#include <string>
#include <iostream>
#include <stdexcept>
#include <algorithm>
#include "ginbuild_updated.h"
// Represents a TID (tuple ID)
// struct TID {
//     int pageId;
//     int rowId;

//     TID(int page, int row) : pageId(page), rowId(row) {}

//     std::string toString() const {
//         return std::to_string(pageId) + "," + std::to_string(rowId);
//     }
// };

// Represents a segment of a posting list
// struct GinPostingList {
//     std::vector<TID> tids;

//     GinPostingList(std::vector<TID> items) : tids(std::move(items)) {}

//     size_t size() const { return tids.size(); }
// };

// // Represents a page in the GIN posting tree
// struct GinPage {
//     std::vector<GinPostingList> postingLists;
//     GinPage* rightLink = nullptr; // Pointer to the next page

//     size_t getFreeSpace(size_t maxPageSize) const {
//         size_t usedSpace = 0;
//         for (const auto& segment : postingLists) {
//             usedSpace += segment.tids.size() * sizeof(TID);
//         }
//         return maxPageSize > usedSpace ? maxPageSize - usedSpace : 0;
//     }
// };

// GIN tree statistics
struct GinStatsData {
    size_t dataPages = 0; // Number of data pages created
};

// Function to create a new GIN posting tree
GinPage* createPostingTree(const std::vector<TID>& items, size_t maxPageSize, GinStatsData* stats);

// Function to insert additional TIDs into an existing posting tree
void insertIntoPostingTree(GinPage* root, const std::vector<TID>& items, size_t maxPageSize);

#endif // GIN_POSTING_TREE_H
