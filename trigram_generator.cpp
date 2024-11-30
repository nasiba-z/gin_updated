#include "trigram_generator.h"

// Function to clean input string (keep only alphanumeric characters)
std::string cleanString(const std::string& input) {
    std::string output;
    std::copy_if(input.begin(), input.end(), std::back_inserter(output), [](char c) {
        return std::isalnum(c) || std::isspace(c);
    });
    return output;
}

// Function to pad and generate trigrams from a string
std::set<Trigram> trigram_generator(const std::string& input) {
    std::set<Trigram> trigrams;

    // Add padding to the string (two spaces before and one after)
    std::string padded = "  " + input + "  ";

    // Generate trigrams
    for (size_t i = 0; i <= padded.size() - 3; ++i) {
        trigrams.insert(padded.substr(i, 3));
    }

    return trigrams;
}

// Function to display trigrams
void displayTrigrams(const std::set<Trigram>& trigrams) {
    std::cout << "Generated Trigrams:\n";
    for (const auto& trigram : trigrams) {
        std::cout << "\"" << trigram << "\" ";
    }
    std::cout << std::endl;
}