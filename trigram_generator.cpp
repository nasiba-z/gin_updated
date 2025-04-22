#include "trigram_generator.h"
#include <sstream>
#include <algorithm>
#include <cctype>
#include <unordered_set>
#include <functional>
#include <iostream>

// // Helper function: cleans the input string (keeping only alphanumeric and space characters).
// static std::string cleanString(const std::string& input) {
//     std::string output;
//     std::copy_if(input.begin(), input.end(), std::back_inserter(output), [](char c) {
//         return std::isalnum(static_cast<unsigned char>(c)) || std::isspace(static_cast<unsigned char>(c));
//     });
//     return output;
// }

std::string normalize(const std::string& in)
{
    std::string out;
    bool lastWasSpace = false;
    for (unsigned char c : in)
    {
        if (std::isalnum(c))
        {
            out.push_back(std::tolower(c));
            lastWasSpace = false;
        }
        else if (std::isspace(c) && !lastWasSpace)
        {
            out.push_back(' ');
            lastWasSpace = true;
        }
        /* else skip char */
    }
    // trim leading/trailing blanks
    if (!out.empty() && out.front() == ' ') out.erase(out.begin());
    if (!out.empty() && out.back()  == ' ') out.pop_back();
    return out;
}

// Implementation of trigram_generator:
// Pads the cleaned input with two spaces at the beginning and end, then extracts all 3-character substrings.
std::set<std::string> trigram_generator(const std::string& input)
{
    std::set<std::string> res;
    std::string txt = normalize(input);

    std::istringstream iss(txt);
    std::string word;
    while (iss >> word)
    {
        std::string padded = "  " + word + " ";
        for (size_t i = 0; i + 2 < padded.size(); ++i)
            res.insert(padded.substr(i,3));
    }
    return res;
}



// From a LIKE pattern (e.g. "%foo%ame%"), extract the literal segments and then
// their trigrams. These are the trigrams that must appear in any candidate.
std::vector<Trigram> getRequiredTrigrams(const std::string &pattern) {
    std::vector<Trigram> ordered;            // result in pattern order
    std::unordered_set<Trigram> seen;        // fast “already added?” check
    size_t pos = 0;
    while (pos < pattern.size())
    {
        /* ---------- find next literal segment between % … % ------------ */
        while (pos < pattern.size() && pattern[pos] == '%')
            ++pos;
        if (pos >= pattern.size())
            break;

        size_t start = pos;
        while (pos < pattern.size() && pattern[pos] != '%')
            ++pos;

        std::string segment = pattern.substr(start, pos - start);
        std::string cleaned = normalize(segment);

        /* ---------- decide padding (same logic you had) ----------------- */
        bool leftPad  = !(start > 0               && pattern[start - 1] == '%');
        bool rightPad = !(pos   < pattern.size()  && pattern[pos]      == '%');

        std::string padded = (leftPad ? "  " : "") + cleaned + (rightPad ? "  " : "");

        /* ---------- extract trigrams in *text* order -------------------- */
        if (padded.size() < 3)          // too short ⇒ no trigram can be forced
            continue;

        for (size_t i = 0; i + 3 <= padded.size(); ++i)
        {
            Trigram tri = padded.substr(i, 3);
            if (seen.insert(tri).second)        // first time we see it
                ordered.push_back(std::move(tri));
        }
    }
    return ordered;      // <‑‑ preserves left‑to‑right order
}

// Pack a trigram string (assumed to be exactly 3 characters) into an int32_t,
// using the same method as in trgm2int().
int32_t packTrigram(const std::string &tri) {
    if (tri.size() < 3) return 0;
    uint32_t val = 0;
    val |= static_cast<unsigned char>(tri[0]);
    val <<= 8;
    val |= static_cast<unsigned char>(tri[1]);
    val <<= 8;
    val |= static_cast<unsigned char>(tri[2]);
    return static_cast<int32_t>(val);
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