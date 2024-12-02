#ifndef TRIGRAM_GENERATOR_H
#define TRIGRAM_GENERATOR_H

#include <iostream>
#include <vector>
#include <string>
#include <set>
#include <algorithm>
#include <cctype>

// Type alias for trigrams
using Trigram = std::string;

// Function to clean input string (keep only alphanumeric characters)
std::string cleanString(const std::string& input);

// Function to pad and generate trigrams from a string
std::set<Trigram> trigram_generator(const std::string& input);

// Function to display trigrams
void displayTrigrams(const std::set<Trigram>& trigrams);

#endif 
