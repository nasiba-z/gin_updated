#include <string>
#include <vector>
#include "pattern_match.h"

/** return true iff  every element of @lits appears in @text
     *  left‑to‑right order (not necessarily contiguously). */
bool literalsAppearInOrder(const std::string& text,
    const std::vector<std::string>& lits){
    std::string::size_type pos = 0;

    for (const std::string& lit : lits){
        pos = text.find(lit, pos);          // forward search
        if (pos == std::string::npos)       // any literal missing
        return false;
        pos += 1;                  // continue after match
    }
    return true;
}