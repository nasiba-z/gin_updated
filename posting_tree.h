#ifndef POSTING_TREE_H
#define POSTING_TREE_H

#include "tid.h"  // Defines TID.
#include <vector>
#include <cstddef>
#include <cstdint>


// Constants for posting tree segment sizes, in bytes.
constexpr size_t GinPostingListSegmentMaxSize   =  1600;// 80000;
constexpr size_t GinPostingListSegmentTargetSize = 800;//;30000
constexpr size_t GinPostingListSegmentMinSize   = 400;//;//2000024000;

// Derive target, max, and min TID counts per leaf node based on TID size.
constexpr size_t LeafTargetCount = GinPostingListSegmentTargetSize / sizeof(TID);
constexpr size_t LeafMaxCount    = GinPostingListSegmentMaxSize   / sizeof(TID);
constexpr size_t LeafMinCount    = GinPostingListSegmentMinSize   / sizeof(TID);

constexpr size_t GinMaxItemSize = 20000;
// -------------------------------------------------------------------
// BTreeNode for PostingTree:
// This node stores TIDs as keys. In a leaf node, keys represent the actual TIDs;
// in an internal node, keys serve as separators.  
// -------------------------------------------------------------------
struct BTreeNode {
    bool leaf;                         // True if node is a leaf.
    std::vector<TID> keys;             // Keys stored in the node.
    std::vector<BTreeNode*> children;  // Child pointers (empty if leaf).

    BTreeNode(bool isLeaf);
    ~BTreeNode();
};

// Forward declaration of BTreeNode.
struct BTreeNode;

// -------------------------------------------------------------------
// PostingTree class: a full B-tree for TIDs built via bulk-loading.
// -------------------------------------------------------------------
class PostingTree {
public:
    BTreeNode* root;  // Pointer to the root node.

    PostingTree();
    ~PostingTree();

    // Bulk-load the tree from a sorted vector of TIDs.
    // This function first builds leaf nodes based on the target number of TIDs per leaf
    // (derived from GinPostingListSegmentTargetSize) and then builds the tree bottom-up.
    void bulkLoad(const std::vector<TID>& sortedTIDs);
    // Incremental insertion: insert a single TID into the tree.
    void insert(const TID& tid);
    // Traverse the tree and print out the keys.
    // void traverse(BTreeNode* x) const;
    // Search for a key in the tree (optional).
    bool search(const TID& k) const;
    // Helper: Build leaf nodes from sorted TIDs.
    std::vector<BTreeNode*> buildLeafNodes(const std::vector<TID>& sortedTIDs);
    size_t getTotalSize() const;      // <--- Declaration here.
    // Helper: Build internal nodes (bottom-up) from a vector of child nodes.
    BTreeNode* buildInternalLevel(const std::vector<BTreeNode*>& children);
    void createFromVector(const std::vector<TID>& items);
    std::vector<TID> getTIDs() const;
    private:
    // Helper for incremental insertion: insert tid into a node assumed not to be full.
    void insertNonFull(BTreeNode* node, const TID& tid);

    // Helper: Split a full child node at index 'i' of a given parent.
    void splitChild(BTreeNode* parent, size_t i);
};

#endif // POSTING_TREE_H
