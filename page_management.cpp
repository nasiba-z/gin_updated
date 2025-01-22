#include <iostream>
#include <vector>
#include <unordered_map>
#include <string>
#include <set>
#include <stdexcept>
#include "gin_extract_value_trgm.h"
#include "ginbuild_updated.h"

//  Monitoring the size of each "page" and splitting it 
//when it exceeds the maximum allowed size for organizing entry pages
// struct GinPage {
//     std::vector<IndexTuple> tuples;  // Tuples stored in this page
//     GinPage* nextPage = nullptr;     // Pointer to the next page (for linked list of pages)

//     size_t getCurrentSize() const {
//         size_t size = 0;
//         for (const auto& tuple : tuples) {
//             size += tuple.datums.size() * sizeof(std::string); // Size of keys
//             size += tuple.postingSize * sizeof(TID);          // Size of posting lists
//         }
//         return size;
//     }
// };


//Function redistributes keys and posting lists between the original page and a new page.
void splitPage(GinPage& leftPage, GinPage& rightPage) {
    // Determine the midpoint for splitting
    size_t midIndex = leftPage.tuples.size() / 2;

    // Move tuples to the right page
    rightPage.tuples.insert(rightPage.tuples.end(), //position to append, at the beginning it's same as start
                            leftPage.tuples.begin() + midIndex, // beg of range to insert
                            leftPage.tuples.end()); // end of range to insert

    // Remove the moved tuples from the left page
    leftPage.tuples.erase(leftPage.tuples.begin() + midIndex, leftPage.tuples.end());
}


//Function detects when a page exceeds its size limit and triggers the splitting process.
void insertIntoPage(GinPage& page, const IndexTuple& tuple, size_t pageMaxSize) {
    // Check if the key already exists in the page
    bool keyExists = false;

    for (auto& existingTuple : page.tuples) {
        if (existingTuple.datums[0] == tuple.datums[0]) {  // Compare keys
            // Append to the existing posting list
            existingTuple.addPostingList(tuple.getPostingList());
            keyExists = true;
            break;
        }
    }

    if (!keyExists) {
        // Add a new tuple to the page
        page.tuples.push_back(tuple);
    }

    // Check for page overflow
    if (page.getCurrentSize() > pageMaxSize) {
        // Create a new page
        GinPage newPage;

        // Split the current page
        splitPage(page, newPage);

        std::cout << "Page split performed. Keys redistributed.\n";
    }
}

