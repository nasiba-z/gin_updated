#include "entry_tree.h"
#include <algorithm>
#include <functional>
#include <iostream>

// Fixed capacities for leaf and internal nodes. You can adjust these.
constexpr size_t LEAF_CAPACITY = 4;
constexpr size_t INTERNAL_CAPACITY = 4;

// --------------------
// EntryTreeNode Methods
// --------------------
EntryTreeNode::EntryTreeNode(bool isLeaf) : leaf(isLeaf) { }

EntryTreeNode::~EntryTreeNode() {
    // Recursively delete children.
    for (auto child : children)
        delete child;
    // IMPORTANT: Do not delete the IndexTuple pointers in values here
    // if they are managed elsewhere.
}

// --------------------
// EntryTree Methods
// --------------------
EntryTree::EntryTree() : root(nullptr) { }

EntryTree::~EntryTree() {
    if (root)
        delete root;
}

// Helper: Build leaf nodes from sorted IndexTuple pointers.
// We assume the input tuples are sorted by their key.
std::vector<EntryTreeNode*> EntryTree::buildLeafNodes(const std::vector<IndexTuple*>& tuples) {
    std::vector<EntryTreeNode*> leaves;
    size_t n = tuples.size();
    size_t i = 0;
    while (i < n) {
        size_t count = LEAF_CAPACITY;
        if (i + count > n)
            count = n - i;
        EntryTreeNode* leaf = new EntryTreeNode(true);
        // Fill leaf with tuples.
        for (size_t j = 0; j < count; j++) {
            // Each leaf stores the key and pointer from each tuple.
            leaf->keys.push_back(tuples[i+j]->key);
            leaf->values.push_back(tuples[i+j]);
        }
        leaves.push_back(leaf);
        i += count;
    }
    return leaves;
}

// Helper: Build internal nodes from a vector of child nodes.
EntryTreeNode* EntryTree::buildInternalLevel(const std::vector<EntryTreeNode*>& children) {
    if (children.size() == 1)
        return children[0];

    std::vector<EntryTreeNode*> parents;
    size_t n = children.size();
    size_t i = 0;
    while (i < n) {
        size_t count = INTERNAL_CAPACITY;
        if (i + count > n)
            count = n - i;
        EntryTreeNode* parent = new EntryTreeNode(false);
        // Group count children under this parent.
        for (size_t j = 0; j < count; j++) {
            parent->children.push_back(children[i+j]);
            // For internal nodes, keys are taken as the first key from each child (except the first).
            if (j > 0)
                parent->keys.push_back(children[i+j]->keys.front());
        }
        parents.push_back(parent);
        i += count;
    }
    return buildInternalLevel(parents);
}

// Bulk-load the tree from a sorted vector of IndexTuple pointers.
void EntryTree::bulkLoad(const std::vector<IndexTuple*>& tuples) {
    // First, build leaf nodes from the sorted tuples.
    std::vector<EntryTreeNode*> leaves = buildLeafNodes(tuples);
    // Then, build internal levels until only one node remains.
    root = buildInternalLevel(leaves);
}

// Insert: A simple insertion algorithm for a B‑tree.
// This implementation does not handle node splitting.
void EntryTree::insert(int32_t key, IndexTuple* tuple) {
    // If the tree is empty, create a new leaf node.
    if (!root) {
        root = new EntryTreeNode(true);
        root->keys.push_back(key);
        root->values.push_back(tuple);
        return;
    }
    // Otherwise, traverse to the appropriate leaf node.
    EntryTreeNode* current = root;
    while (!current->leaf) {
        size_t i = 0;
        while (i < current->keys.size() && key >= current->keys[i])
            i++;
        current = current->children[i];
    }
    // Now current is a leaf; insert in sorted order.
    auto it = std::upper_bound(current->keys.begin(), current->keys.end(), key);
    size_t pos = it - current->keys.begin();
    current->keys.insert(it, key);
    current->values.insert(current->values.begin() + pos, tuple);
    // Note: Splitting of full nodes is not handled in this simple version.
}

// Search: Simple search through the tree.
bool EntryTree::search(int32_t key) const {
    EntryTreeNode* current = root;
    if (!current) return false;
    while (!current->leaf) {
        size_t i = 0;
        while (i < current->keys.size() && key >= current->keys[i])
            i++;
        current = current->children[i];
    }
    return std::find(current->keys.begin(), current->keys.end(), key) != current->keys.end();
}

// getTotalSize: Compute the total "logical" size of the tree by traversing all nodes.
size_t EntryTree::getTotalSize() const {
    size_t total = 0;
    // Recursive lambda to traverse the tree.
    std::function<void(EntryTreeNode*)> traverse = [&](EntryTreeNode* node) {
        if (node->leaf) {
            total += sizeof(EntryTreeNode) + node->keys.size() * sizeof(int32_t)
                     + node->values.size() * sizeof(IndexTuple*);
        } else {
            for (EntryTreeNode* child : node->children)
                traverse(child);
            total += sizeof(EntryTreeNode) + node->keys.size() * sizeof(int32_t)
                     + node->children.size() * sizeof(EntryTreeNode*);
        }
    };
    if (root) traverse(root);
    return total;
}

