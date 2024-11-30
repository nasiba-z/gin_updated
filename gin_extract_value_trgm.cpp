#include <iostream>
#include <vector>
#include <set>
#include <string>
#include <functional> 
#include <memory>    
#include "trigram_generator.h"


// Simulate PostgreSQL's Int32GetDatum with int32_t
using Datum = int32_t;

// Function to simulate PostgreSQL's trgm2int
int32_t trgm2int(const std::string& trigram) {
    // Use std::hash for simplicity (convert string to unique integer)
    return static_cast<int32_t>(std::hash<std::string>{}(trigram) & 0x7FFFFFFF); // Ensure non-negative value
}

// C++ equivalent of gin_extract_value_trgm
std::unique_ptr<std::vector<Datum>> gin_extract_value_trgm(const std::string& input) {
    std::set<std::string> trigrams = trigram_generator(input);  // Call the trigram generator
    auto entries = std::make_unique<std::vector<Datum>>();      

    for (const auto& trigram : trigrams) {
        entries->push_back(trgm2int(trigram)); // Convert trigram to int and store in entries
    }

    return entries; // Return pointer to vector of Datum
}

// Function to display the extracted trigrams (as integers)
void displayTrigrams(const std::vector<Datum>& entries) {
    std::cout << "Extracted Trigrams (as integers):\n";
    for (const auto& entry : entries) {
        std::cout << entry << " ";
    }
    std::cout << std::endl;
}

// Main function for testing
// int main() {
//     std::string testInput = "Hello World";

//     std::cout << "Input: " << testInput << std::endl;

//     // Call gin_extract_value_trgm and get the extracted trigrams
//     auto extractedTrigrams = gin_extract_value_trgm(testInput);

//     // Display the trigrams (as integers)
//     displayTrigrams(*extractedTrigrams);

//     return 0;
// }
