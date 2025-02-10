#include "tree_management.h"




void insertIntoPostingTree(GinPage* root, const std::vector<TID>& items, size_t maxPageSize) {
    if (!root) {
        throw std::invalid_argument("Root page cannot be null.");
    }

    GinPage* current = root;
    while (current->rightLink) {
        current = current->rightLink;
    }

    GinPostingList currentSegment({});

    for (const auto& item : items) {
        currentSegment.tids.push_back(item);
        size_t segmentSize = currentSegment.tids.size() * sizeof(TID);

        if (segmentSize >= maxPageSize) {
            if (current->getFreeSpace(maxPageSize) >= segmentSize) {
                current->postingLists.push_back(currentSegment);
            } else {
                auto* newPage = new GinPage();
                newPage->postingLists.push_back(currentSegment);
                current->rightLink = newPage;
                current = newPage;
            }
            currentSegment = GinPostingList({});
        }
    }

    if (!currentSegment.tids.empty()) {
        if (current->getFreeSpace(maxPageSize) >= currentSegment.tids.size() * sizeof(TID)) {
            current->postingLists.push_back(currentSegment);
        } else {
            auto* newPage = new GinPage();
            newPage->postingLists.push_back(currentSegment);
            current->rightLink = newPage;
        }
    }
}
