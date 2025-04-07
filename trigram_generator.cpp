#include "trigram_generator.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <functional>
#include <iostream>

// Helper function: cleans the input string (keeping only alphanumeric and space characters).
static std::string cleanString(const std::string& input) {
    std::string output;
    std::copy_if(input.begin(), input.end(), std::back_inserter(output), [](char c) {
        return std::isalnum(static_cast<unsigned char>(c)) || std::isspace(static_cast<unsigned char>(c));
    });
    return output;
}

// Implementation of trigram_generator:
// Pads the cleaned input with two spaces at the beginning and end, then extracts all 3-character substrings.
std::set<Trigram> trigram_generator(const std::string& input) {
    std::set<Trigram> trigrams;
    std::string cleaned = cleanString(input);
    std::string padded = "  " + cleaned + "  ";
    for (size_t i = 0; i <= padded.size() - 3; ++i)
        trigrams.insert(padded.substr(i, 3));
    return trigrams;
}

// From a LIKE pattern (e.g. "%foo%ame%"), extract the literal segments and then
// their trigrams. These are the trigrams that must appear in any candidate.
std::set<Trigram> getRequiredTrigrams(const std::string &pattern) {
    std::set<Trigram> required;
    size_t pos = 0;
    while (pos < pattern.size()) {
        // Skip wildcard characters.
        while (pos < pattern.size() && pattern[pos] == '%')
            pos++;
        if (pos >= pattern.size()) break;
        // Find next wildcard position.
        size_t start = pos;
        while (pos < pattern.size() && pattern[pos] != '%')
            pos++;
        std::string segment = pattern.substr(start, pos - start);
        // Extract trigrams for this literal segment.
        auto segTrigrams = trigram_generator(segment);
        required.insert(segTrigrams.begin(), segTrigrams.end());
    }
    return required;
}

// Implementation of trgm2int:
// For each distinct trigram, pack its 3 characters into a 32-bit integer and return them in a vector.
std::unique_ptr<TrigramArray> trgm2int(const std::string& input, int32_t* nentries) {
    *nentries = 0;
    // Generate the set of distinct trigrams.
    std::set<Trigram> trigrams = trigram_generator(input);
    int32_t trglen = trigrams.size();
    if (trglen == 0)
        return std::make_unique<TrigramArray>();
    
    auto entries = std::make_unique<TrigramArray>();
    entries->reserve(trglen);
    
    for (const auto& t : trigrams) {
        // We assume each trigram has exactly 3 characters.
        if (t.size() < 3)
            continue;
        
        uint32_t val = 0;
        // Pack the three characters into val.
        val |= static_cast<unsigned char>(t[0]);
        val <<= 8;
        val |= static_cast<unsigned char>(t[1]);
        val <<= 8;
        val |= static_cast<unsigned char>(t[2]);
        
        entries->push_back(static_cast<int32_t>(val));
    }
    
    *nentries = trglen;
    return entries;
}