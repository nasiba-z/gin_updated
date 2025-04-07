#include "entry_tree.h"
#include <algorithm>
#include <functional>
#include <iostream>

// Utility: For simplicity, EntryLeafMaxCount is maximum keys for all nodes.
static const size_t MAX_KEYS = EntryLeafMaxCount;
// Define minimum number of keys per node (roughly half the maximum).
static const size_t MIN_KEYS = (EntryLeafMaxCount + 1) / 2;

// -------------------------
// Destructor
// -------------------------
EntryTree::~EntryTree() {
    if (root)
        delete root;
}

// -------------------------
// Bulk-Loading: Build leaf nodes from a sorted vector of IndexTuple pointers.
// Assumes the input tuples are sorted by their key.
// -------------------------
std::vector<EntryTreeNode*> EntryTree::buildLeafNodes(const std::vector<IndexTuple*>& tuples) {
    std::vector<EntryTreeNode*> leaves;
    size_t n = tuples.size();
    size_t i = 0;
    while (i < n) {
        // Create a new leaf.
        EntryTreeNode* leaf = new EntryTreeNode(true);
        // Insert up to MAX_KEYS items.
        size_t count = std::min(n - i, MAX_KEYS);
        for (size_t j = 0; j < count; j++) {
            leaf->keys.push_back(tuples[i + j]->key);
            leaf->values.push_back(tuples[i + j]);
        }
        leaves.push_back(leaf);
        i += count;
    }
    return leaves;
}

// -------------------------
// Bulk-Loading: Build internal nodes bottom-up from a vector of child nodes.
// In internal nodes, keys are used for routing: each key is the first key of the child (except the first child).
// -------------------------
EntryTreeNode* EntryTree::buildInternalLevel(const std::vector<EntryTreeNode*>& children) {
    if (children.size() == 1)
        return children[0];

    std::vector<EntryTreeNode*> parents;
    size_t n = children.size();
    size_t i = 0;
    while (i < n) {
        EntryTreeNode* parent = new EntryTreeNode(false);
        // Group up to MAX_KEYS+1 children in one internal node.
        size_t count = std::min(n - i, MAX_KEYS + 1);
        // Assign children and seperator keys
        // The first child pointer is inserted without a key.
        for (size_t j = 0; j < count; j++) {
            parent->children.push_back(children[i + j]);
            // For each child after the first, use its first key as a separator.
            if (j > 0 && !children[i + j]->keys.empty()) {
                parent->keys.push_back(children[i + j]->keys.front());
            }
        }
        parents.push_back(parent);
        i += count;
    }
    return buildInternalLevel(parents);
}

// -------------------------
// Bulk-Loading: Build the EntryTree from a sorted vector of IndexTuple pointers.
// -------------------------
void EntryTree::bulkLoad(const std::vector<IndexTuple*>& tuples) {
    // First, build leaf nodes.
    std::vector<EntryTreeNode*> leaves = buildLeafNodes(tuples);
    // Then, build the internal levels until one root remains.
    root = buildInternalLevel(leaves);
}

// -------------------------
// Incremental Insertion: Insert a key and its associated tuple into the tree.
// -------------------------
void EntryTree::insert(int32_t key, IndexTuple* tuple) {
    // If tree is empty, create a new leaf.
    if (!root) {
        root = new EntryTreeNode(true);
        root->keys.push_back(key);
        root->values.push_back(tuple);
        return;
    }

    // If root is full, split it.
    if ( (root->leaf && root->keys.size() == MAX_KEYS) ||
         (!root->leaf && root->keys.size() == MAX_KEYS) ) {
        EntryTreeNode* newRoot = new EntryTreeNode(false);
        newRoot->children.push_back(root);
        splitChild(newRoot, 0);
        root = newRoot;
    }
    insertNonFull(root, key, tuple);
}

// -------------------------
// Helper for Incremental Insertion: Insert key/tuple into a node assumed not full.
// -------------------------
void EntryTree::insertNonFull(EntryTreeNode* node, int32_t key, IndexTuple* tuple) {
    if (node->leaf) {
        // Insert into leaf in sorted order.
        auto pos = std::upper_bound(node->keys.begin(), node->keys.end(), key);
        size_t index = pos - node->keys.begin();
        node->keys.insert(pos, key);
        node->values.insert(node->values.begin() + index, tuple);
    } else {
        // Find the child to descend.
        size_t i = 0;
        while (i < node->keys.size() && key >= node->keys[i])
            i++;
        // Check if the child is full.
        if (node->children[i]->leaf) {
            if (node->children[i]->keys.size() == MAX_KEYS) {
                splitChild(node, i);
                // After split, decide which child to descend.
                if (key >= node->keys[i])
                    i++;
            }
        } else {
            if (node->children[i]->keys.size() == MAX_KEYS) {
                splitChild(node, i);
                if (key >= node->keys[i])
                    i++;
            }
        }
        insertNonFull(node->children[i], key, tuple);
    }
}

// -------------------------
// Helper: Split the full child node at index i of the given parent.
// For a leaf node, we split and promote the first key of the new sibling.
// For an internal node, we remove the median key and promote it.
// -------------------------
void EntryTree::splitChild(EntryTreeNode* parent, size_t i) {
    EntryTreeNode* child = parent->children[i];
    EntryTreeNode* sibling = new EntryTreeNode(child->leaf);
    size_t t; // splitting index
    if (child->leaf) {
        t = (child->keys.size() + 1) / 2; // split roughly in half
        // Sibling gets keys and values from t to end.
        sibling->keys.assign(child->keys.begin() + t, child->keys.end());
        sibling->values.assign(child->values.begin() + t, child->values.end());
        // Resize child.
        child->keys.resize(t);
        child->values.resize(t);
        // In a B+ tree, the promoted key is the first key of the new sibling.
        int32_t promoteKey = sibling->keys.front();
        parent->keys.insert(parent->keys.begin() + i, promoteKey);
    } else {
        t = child->keys.size() / 2; // for internal nodes, use lower median
        // The promoted key is child->keys[t]
        int32_t promoteKey = child->keys[t];
        // Sibling gets keys from t+1 to end.
        sibling->keys.assign(child->keys.begin() + t + 1, child->keys.end());
        child->keys.resize(t);
        // Sibling gets child pointers from t+1 to end.
        sibling->children.assign(child->children.begin() + t + 1, child->children.end());
        child->children.resize(t + 1);
        // Insert the promoted key into the parent.
        parent->keys.insert(parent->keys.begin() + i, promoteKey);
    }
    // Insert sibling into parent's children.
    parent->children.insert(parent->children.begin() + i + 1, sibling);
}

// -------------------------
// Search Function: Look for a key in the tree.
// -------------------------
IndexTuple* EntryTree::search(int32_t key) const {
    EntryTreeNode* current = root;
    if (!current)
        return nullptr;
    // Traverse down the tree until we reach a leaf.
    while (!current->leaf) {
        size_t i = 0;
        while (i < current->keys.size() && key >= current->keys[i])
            i++;
        current = current->children[i];
    }
    // In the leaf, search for the key.
    for (size_t i = 0; i < current->keys.size(); ++i) {
        if (current->keys[i] == key)
            return current->values[i];  // Return the IndexTuple pointer.
    }
    return nullptr;  // Key not found.
}

// -------------------------
// getTotalSize: Compute the total "logical" size (number of keys) in the tree.
// -------------------------
size_t EntryTree::getTotalSize() const {
    size_t total = 0;
    std::function<void(EntryTreeNode*)> traverse = [&](EntryTreeNode* node) {
        if (node->leaf) {
            total += node->keys.size();
        } else {
            for (auto child : node->children)
                traverse(child);
            total += node->keys.size();
        }
    };
    if (root)
        traverse(root);
    return total;
}


