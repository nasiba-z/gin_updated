#ifndef ENTRY_TREE_H
#define ENTRY_TREE_H

#include <vector>
#include <string>
#include "gin_index.h"

// Define a maximum capacity for an EntryTree leaf node.
// (For simplicity, we use the same capacity for internal nodes.)
// 20 keys to mimic fitting a index page of 8kB as in original GIN:20(21)=420(slightly less than 8000/(4+8)=666 key and pointers
constexpr size_t EntryLeafMaxCount = 20;

// -------------------------------------------------------------------
// EntryTreeNode for EntryTree:
// This node stores trigram integers as keys. 
// For leaf nodes, the 'values' vector stores pointers to IndexTuple (which in turn
// hold pointers to postingList or postingTree). Internal nodes do not use values.
// -------------------------------------------------------------------
struct EntryTreeNode {
    bool leaf;                              // True if node is a leaf.
    std::vector<int32_t> keys;              // Keys stored in the node.
    std::vector<EntryTreeNode*> children;   // Child pointers (empty if leaf).
    // For leaf nodes, we store associated IndexTuple pointers.
    std::vector<IndexTuple*> values;         

    EntryTreeNode(bool isLeaf) : leaf(isLeaf) {}

    ~EntryTreeNode() {
        for (auto child : children)
            delete child;
        for (auto val : values)
            delete val;
    }
};

// -------------------------------------------------------------------
// EntryTree class: a full B⁺-tree for keys built via bulk-loading or 
// incremental insertion for online data only.
// -------------------------------------------------------------------
class EntryTree {
public:
    EntryTreeNode* root;  // Pointer to the root node.

    EntryTree() : root(nullptr) {}
    ~EntryTree();

    // Bulk-load the tree from a sorted vector of IndexTuple pointers.
    // Assumes the tuples are sorted by key.
    void bulkLoad(const std::vector<IndexTuple*>& tuples);

    // Incremental insertion: insert a key and its associated IndexTuple into the tree.
    void insert(int32_t key, IndexTuple* tuple);

    // Search for a key in the tree.
    bool search(int32_t key) const;

    // Returns the total "size" of the tree (e.g. total number of keys).
    size_t getTotalSize() const;

    // Helper functions for bulk loading.
    std::vector<EntryTreeNode*> buildLeafNodes(const std::vector<IndexTuple*>& tuples);
    EntryTreeNode* buildInternalLevel(const std::vector<EntryTreeNode*>& children);

private:
    // Helper for incremental insertion: insert key/tuple into a node assumed not full.
    void insertNonFull(EntryTreeNode* node, int32_t key, IndexTuple* tuple);

    // Helper: Split a full child node at index 'i' of a given parent.
    void splitChild(EntryTreeNode* parent, size_t i);
};

#endif // ENTRY_TREE_H
