#include "trigram_generator.h" 
#include <iostream>
#include <vector>
#include <tuple>
#include <algorithm>

int main() {
	int32_t nentries = 0;
	// trgm2int() returns a unique_ptr to a vector of datum (trigram keys).
	auto trigram = trgm2int("yel", &nentries);
	std::cout << trigram->at(0) << std::endl;
	return 0;
}
