#include <iostream>
#include <vector>
#include "tid.h" // Ensure this file contains the definition of TID

int main() {
    // Check the size of a single TID
    size_t tidSize = sizeof(TID);
    std::cout << "Size of a single TID: " << tidSize << " bytes" << std::endl;

    // Check the size of a vector of TIDs
    std::vector<TID> tids(100); // Example vector with 100 TIDs
    size_t vectorSize = sizeof(tids) + tids.capacity() * sizeof(TID);
    std::cout << "Size of a vector of 100 TIDs: " << vectorSize << " bytes" << std::endl;

    return 0;
}