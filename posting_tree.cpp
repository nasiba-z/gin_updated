#include "posting_tree.h"
#include <algorithm>
#include <cassert>
#include <functional>

// ------------------------------------------------------------------
// BTreeNode Implementation
// ------------------------------------------------------------------
BTreeNode::BTreeNode(bool isLeaf) : leaf(isLeaf) {
    // keys and children are default-initialized.
}

BTreeNode::~BTreeNode() {
    for (auto child : children)
        delete child;
}

// ------------------------------------------------------------------
// PostingTree Implementation
// ------------------------------------------------------------------
PostingTree::PostingTree() : root(nullptr) {}

PostingTree::~PostingTree() {
    if (root)
        delete root;
}

// ------------------------------------------------------------------
// Helper function: Build leaf nodes from sorted TIDs.
// ------------------------------------------------------------------
std::vector<BTreeNode*> PostingTree::  buildLeafNodes(const std::vector<TID>& sortedTIDs) {
    std::vector<BTreeNode*> leaves;
    size_t n = sortedTIDs.size();
    size_t i = 0;
    while (i < n) {
        // Determine how many TIDs to put in this leaf node.
        // We aim for a node size close to LeafTargetCount, but we cannot exceed LeafMaxCount.
        size_t count = LeafTargetCount;
        if (i + count > n)
            count = n - i;
        // If count is less than the minimum and leaves already exist, merge it with the previous leaf.
        if (!leaves.empty() && count < LeafMinCount) {
            BTreeNode* lastLeaf = leaves.back();
            lastLeaf->keys.insert(lastLeaf->keys.end(), sortedTIDs.begin() + i, sortedTIDs.end());
            break;
        }
        // Ensure we do not exceed the maximum.
        if (count > LeafMaxCount)
            count = LeafMaxCount;
        BTreeNode* leaf = new BTreeNode(true);
        leaf->keys.insert(leaf->keys.end(), sortedTIDs.begin() + i, sortedTIDs.begin() + i + count);
        leaves.push_back(leaf);
        i += count;
    }
    return leaves;
}

// ------------------------------------------------------------------
// Helper function: Build internal nodes from a vector of child nodes.
// ------------------------------------------------------------------
BTreeNode* PostingTree:: buildInternalLevel(const std::vector<BTreeNode*>& children) {
    // If there's only one child, that's the tree.
    if (children.size() == 1)
        return children[0];

    std::vector<BTreeNode*> parents;
    // Use a fixed branching factor for internal nodes.
    // For example, let’s use a branching factor B.
    const size_t B = 16; 
    size_t n = children.size();
    size_t i = 0;
    while (i < n) {
        size_t count = B;
        if (i + count > n)
            count = n - i;
        BTreeNode* parent = new BTreeNode(false);
        // Set parent's children from children[i] to children[i+count-1].
        parent->children.insert(parent->children.end(), children.begin() + i, children.begin() + i + count);
        // The parent's keys are the first key of each child except the first.
        for (size_t j = 1; j < count; j++) {
            parent->keys.push_back(children[i + j]->keys.front());
        }
        parents.push_back(parent);
        i += count;
    }
    return buildInternalLevel(parents);
}

// ------------------------------------------------------------------
// bulkLoad: Build the PostingTree from a sorted vector of TIDs.
// ------------------------------------------------------------------
void PostingTree::bulkLoad(const std::vector<TID>& sortedTIDs) {
    // Build the leaf nodes.
    std::vector<BTreeNode*> leaves = buildLeafNodes(sortedTIDs);
    // Build the internal levels.
    root = buildInternalLevel(leaves);
}
// ------------------------------------------------------------------
// getTotalSize: Compute the total "logical" size of the posting tree.
// This function traverses all leaf nodes and sums their sizes.
// Here we assume each leaf's size is:
//   overhead (e.g., sizeof(BTreeNode)) plus the number of TIDs times sizeof(TID).
// ------------------------------------------------------------------
size_t PostingTree::getTotalSize() const {
    size_t total = 0;
    
    // Define a lambda to traverse the B-tree recursively.
    std::function<void(BTreeNode*)> traverse = [&](BTreeNode* node) {
        if (node->leaf) {
            // For each leaf, we simulate the size as:
            // overhead + (number of TIDs * sizeof(TID))
            total += sizeof(BTreeNode) + node->keys.size() * sizeof(TID);
        } else {
            // For internal nodes, recursively traverse children.
            for (BTreeNode* child : node->children)
                traverse(child);
        }
    };
    
    if (root)
        traverse(root);
    return total;
}