#include "entry_tree.h"

// Create an entry tree from a collection of IndexTuple pointers.
// We assume that each IndexTuple contains at least one key in its datums vector,
// and we use the first datum as the key for insertion.
EntryTree* createEntryTreeFromTuples(const std::vector<IndexTuple*>& tuples) {
    // Choose a minimum degree for the B-tree; for example, t = 3.
    EntryTree* tree = new EntryTree(3);
    for (IndexTuple* tuple : tuples) {
        if (!tuple->datums.empty()) {
            // Use the first element of datums as the key.
            tree->insert(tuple->datums[0], tuple);
        }
    }
    return tree;
}
