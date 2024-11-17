#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <cctype>

// Type alias for trigrams
using Trigram = std::string;

// Function to clean input string (keep only alphanumeric characters)
std::string cleanString(const std::string& input) {
    std::string output;
    std::copy_if(input.begin(), input.end(), std::back_inserter(output), [](char c) {
        return std::isalnum(c) || std::isspace(c);
    });
    return output;
}

// Function to pad and generate trigrams from a string
std::set<Trigram> generateTrgm(const std::string& input) {
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

// Test function to verify `generateTrgm`
void testGenerateTrgm() {
    std::vector<std::string> testInputs = {
        "chiffon smoke firebrick cornsilk almond",
        "azure sienna royal papaya lace",
        "steel maroon rose spring salmon",
        ""
    };

    for (const auto& input : testInputs) {
        std::cout << "\nInput: \"" << input << "\"" << std::endl;

        // Clean and generate trigrams
        std::string cleanedInput = cleanString(input);
        std::set<Trigram> trigrams = generateTrgm(cleanedInput);

        // Display trigrams
        displayTrigrams(trigrams);
    }
}

// Main function
int main() {
    std::cout << "Testing Trigram Generation:\n";
    testGenerateTrgm();
    return 0;
}
