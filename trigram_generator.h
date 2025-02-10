#ifndef TRIGRAM_H
#define TRIGRAM_H

#include <string>
#include <set>
#include <vector>
#include <memory>
#include <cstdint>

// Type alias for a trigram: a simple string (we assume trigrams are 3-character sequences).
using Trigram = std::string;

// This function takes an input string, pads it, and returns a set of distinct trigrams.
std::set<Trigram> trigram_generator(const std::string& input);

// Type alias for Datum and TrigramArray.
using Datum = int32_t;
using TrigramArray = std::vector<Datum>;

// This function converts trigrams into integers using bit‐packing logic.
// It returns a unique_ptr to a vector of Datum (i.e. 32‑bit integers).
// The parameter nentries is set to the number of distinct trigrams.
std::unique_ptr<TrigramArray> trgm2int(const std::string& input, int32_t* nentries);

#endif // TRIGRAM_H

