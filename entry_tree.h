#ifndef ENTRY_TREE_H
#define ENTRY_TREE_H

#include <vector>
#include <string>
#include "gin_index.h"

// BTreeNode for EntryTree:
// This node stores trigram integers as keys. 
// The values store store a pointer to the entire IndexTuple
// -------------------------------------------------------------------
struct EntryTreeNode {
    bool leaf;                              // True if node is a leaf.
    std::vector<int32_t> keys;          // Keys stored in the node.
    std::vector<EntryTreeNode*> children;   // Child pointers (empty if leaf).
    // For leaf nodes, we store values (pointers to IndexTuple).
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
// EntryTree class: a full B-tree for TIDs built via bulk-loading.
// -------------------------------------------------------------------

class EntryTree {
    public:
        EntryTreeNode* root;  // Pointer to the root node.
    
        EntryTree();
        ~EntryTree();
    
        // Bulk-load the tree from a sorted vector of IndexTuple pointers.
        // This function builds leaf nodes and then builds the tree bottom-up.
        void bulkLoad(const std::vector<IndexTuple*>& tuples);
    
        // Insert a key and its associated index tuple into the tree.
        void insert(int32_t key, IndexTuple* tuple);
    
        // Search for a key in the tree (optional).
        bool search(int32_t key) const;
    
        size_t getTotalSize() const;  // Returns the total "size" of the tree.
    
    private:
        std::vector<EntryTreeNode*> buildLeafNodes(const std::vector<IndexTuple*>& tuples);
        EntryTreeNode* buildInternalLevel(const std::vector<EntryTreeNode*>& children);
    };
    
    #endif // ENTRY_TREE_H