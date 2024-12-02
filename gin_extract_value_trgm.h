#ifndef GIN_EXTRACT_VALUE_TRGM
#define GIN_EXTRACT_VALUE_TRGM

#include <string>
#include <vector>
#include <memory>

// Type definitions
using Datum = int32_t;                    // PostgreSQL Datum equivalent
using TrigramArray = std::vector<Datum>;  // Array of trigram integers

// Function to generate trigram entries from a string
// - input: The input string to process
// - nentries: Pointer to store the number of trigrams generated
// - Returns: A smart pointer to a vector containing trigram integers
std::unique_ptr<TrigramArray> gin_extract_value_trgm(const std::string& input, int32_t* nentries);

#endif 
