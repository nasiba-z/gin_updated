// ART.cpp
#include "ART.h"
#include <iostream>

// ---------------------
// ARTNode Implementation
// ---------------------
ARTNode::ARTNode(NodeType t) : type(t) {}
ARTNode::~ARTNode() {}

// ---------------------
// ARTLeaf Implementation
// ---------------------
ARTLeaf::ARTLeaf(const std::vector<unsigned char>& key, int v)
    : ARTNode(NodeType::LEAF), fullKey(key), value(v) {}

ARTNode* ARTLeaf::insert(const unsigned char* key, int keyLen, int depth, int newValue) {
    if (fullKey.size() == static_cast<size_t>(keyLen) &&
        memcmp(fullKey.data(), key, keyLen) == 0) {
        value = newValue;
    }
    // Splitting on key mismatch is not implemented in this simplified example.
    return this;
}

int* ARTLeaf::search(const unsigned char* key, int keyLen, int depth) const {
    if (fullKey.size() == static_cast<size_t>(keyLen) &&
        memcmp(fullKey.data(), key, keyLen) == 0)
        return const_cast<int*>(&value);
    return nullptr;
}

// ---------------------
// ARTNode4 Implementation
// ---------------------
ARTNode4::ARTNode4() : ARTNode(NodeType::NODE4), count(0) {
    memset(keys, 0, sizeof(keys));
    memset(children, 0, sizeof(children));
}

ARTNode4::~ARTNode4() {
    for (int i = 0; i < count; i++) {
        delete children[i];
    }
}

ARTNode* ARTNode4::insert(const unsigned char* key, int keyLen, int depth, int value) {
    assert(depth < keyLen);
    unsigned char currentKey = key[depth];

    for (int i = 0; i < count; i++) {
        if (keys[i] == currentKey) {
            ARTNode* child = children[i];
            ARTNode* newChild = child->insert(key, keyLen, depth + 1, value);
            children[i] = newChild;
            return this;
        }
    }

    if (count < 4) {
        std::vector<unsigned char> fullKey(key, key + keyLen);
        ARTLeaf* newLeaf = new ARTLeaf(fullKey, value);

        int pos = 0;
        while (pos < count && keys[pos] < currentKey)
            pos++;
        for (int i = count; i > pos; i--) {
            keys[i] = keys[i - 1];
            children[i] = children[i - 1];
        }
        keys[pos] = currentKey;
        children[pos] = newLeaf;
        count++;
        return this;
    }
    assert(false && "Node4 is full; node growth is not implemented.");
    return this;
}

int* ARTNode4::search(const unsigned char* key, int keyLen, int depth) const {
    if (depth >= keyLen) return nullptr;
    unsigned char currentKey = key[depth];
    for (int i = 0; i < count; i++) {
        if (keys[i] == currentKey) {
            return children[i]->search(key, keyLen, depth + 1);
        }
    }
    return nullptr;
}

// ---------------------
// ARTNode16 Implementation
// ---------------------
ARTNode16::ARTNode16() : ARTNode(NodeType::NODE16), count(0) {
    memset(keys, 0, sizeof(keys));
    memset(children, 0, sizeof(children));
}

ARTNode16::~ARTNode16() {
    for (int i = 0; i < count; i++) {
        delete children[i];
    }
}

ARTNode* ARTNode16::insert(const unsigned char* key, int keyLen, int depth, int value) {
    assert(depth < keyLen);
    unsigned char currentKey = key[depth];

    // Check if the key already exists.
    for (int i = 0; i < count; i++) {
        if (keys[i] == currentKey) {
            ARTNode* child = children[i];
            ARTNode* newChild = child->insert(key, keyLen, depth + 1, value);
            children[i] = newChild;
            return this;
        }
    }

    // If there is room, insert the new key in sorted order.
    if (count < 16) {
        std::vector<unsigned char> fullKey(key, key + keyLen);
        ARTLeaf* newLeaf = new ARTLeaf(fullKey, value);
        int pos = 0;
        while (pos < count && keys[pos] < currentKey)
            pos++;
        for (int i = count; i > pos; i--) {
            keys[i] = keys[i - 1];
            children[i] = children[i - 1];
        }
        keys[pos] = currentKey;
        children[pos] = newLeaf;
        count++;
        return this;
    }
    assert(false && "Node16 is full; node growth is not implemented.");
    return this;
}

int* ARTNode16::search(const unsigned char* key, int keyLen, int depth) const {
    if (depth >= keyLen) return nullptr;
    unsigned char currentKey = key[depth];
    for (int i = 0; i < count; i++) {
        if (keys[i] == currentKey) {
            return children[i]->search(key, keyLen, depth + 1);
        }
    }
    return nullptr;
}

// ---------------------
// ARTNode48 Implementation
// ---------------------
ARTNode48::ARTNode48() : ARTNode(NodeType::NODE48), count(0) {
    // Initialize the childIndex array with an invalid index (0xFF).
    for (int i = 0; i < 256; i++) {
        childIndex[i] = 0xFF;
    }
    memset(children, 0, sizeof(children));
}

ARTNode48::~ARTNode48() {
    for (int i = 0; i < count; i++) {
        delete children[i];
    }
}

ARTNode* ARTNode48::insert(const unsigned char* key, int keyLen, int depth, int value) {
    assert(depth < keyLen);
    unsigned char currentKey = key[depth];

    // If a child already exists for this key byte, delegate the insertion.
    if (childIndex[currentKey] != 0xFF) {
        int idx = childIndex[currentKey];
        ARTNode* child = children[idx];
        ARTNode* newChild = child->insert(key, keyLen, depth + 1, value);
        children[idx] = newChild;
        return this;
    }

    // If there is room in this node, add a new child.
    if (count < 48) {
        std::vector<unsigned char> fullKey(key, key + keyLen);
        ARTLeaf* newLeaf = new ARTLeaf(fullKey, value);
        childIndex[currentKey] = count;
        children[count] = newLeaf;
        count++;
        return this;
    }
    assert(false && "Node48 is full; node growth is not implemented.");
    return this;
}

int* ARTNode48::search(const unsigned char* key, int keyLen, int depth) const {
    if (depth >= keyLen) return nullptr;
    unsigned char currentKey = key[depth];
    if (childIndex[currentKey] == 0xFF)
        return nullptr;
    int idx = childIndex[currentKey];
    return children[idx]->search(key, keyLen, depth + 1);
}

// ---------------------
// ARTNode256 Implementation
// ---------------------
ARTNode256::ARTNode256() : ARTNode(NodeType::NODE256), count(0) {
    memset(children, 0, sizeof(children));
}

ARTNode256::~ARTNode256() {
    for (int i = 0; i < 256; i++) {
        if (children[i])
            delete children[i];
    }
}

ARTNode* ARTNode256::insert(const unsigned char* key, int keyLen, int depth, int value) {
    assert(depth < keyLen);
    unsigned char currentKey = key[depth];
    // If a child already exists at the index, delegate insertion.
    if (children[currentKey] != nullptr) {
        ARTNode* child = children[currentKey];
        ARTNode* newChild = child->insert(key, keyLen, depth + 1, value);
        children[currentKey] = newChild;
        return this;
    }
    // Otherwise, create a new leaf and assign it directly.
    std::vector<unsigned char> fullKey(key, key + keyLen);
    ARTLeaf* newLeaf = new ARTLeaf(fullKey, value);
    children[currentKey] = newLeaf;
    count++;
    return this;
}

int* ARTNode256::search(const unsigned char* key, int keyLen, int depth) const {
    if (depth >= keyLen) return nullptr;
    unsigned char currentKey = key[depth];
    if (children[currentKey] == nullptr)
        return nullptr;
    return children[currentKey]->search(key, keyLen, depth + 1);
}

// Helper function: Recursively bulk load ART from sorted items.
ARTNode* ART_bulkLoad(const std::vector<std::pair<std::vector<unsigned char>, IndexTuple*>>& items, int depth) {
    if (items.empty())
        return nullptr;

    // If we have few items, or we've reached the end of the key,
    // then build a leaf-level container.
    // (In a classic ART, each leaf is a single key/value pair;
    // here, for bulk loading, we group adjacent items into an inner node.)
    if (items.size() <= LEAF_THRESHOLD || depth >= (int)items[0].first.size()) {
        // If there's only one item, return an ARTLeaf.
        if (items.size() == 1) {
            return new ARTLeaf(items[0].first, items[0].second->postingSize);
        }
        // Otherwise, create a simple inner node (using Node4 for simplicity)
        ARTNode4* node = new ARTNode4();
        // Insert each item as a child leaf.
        for (size_t i = 0; i < items.size(); i++) {
            ARTLeaf* leaf = new ARTLeaf(items[i].first, items[i].second->postingSize);
            // For all but the first child, the separator key is the first byte of the key.
            if (i > 0) {
                node->keys[node->count] = items[i].first[0];
            }
            node->children[node->count] = leaf;
            node->count++;
        }
        return node;
    }

    // Partition the items by the byte at the current depth.
    std::vector<std::vector<std::pair<std::vector<unsigned char>, IndexTuple*>>> partitions(256);
    for (const auto& item : items) {
        // Get the byte at position 'depth'. If the key is shorter, treat it as 0.
        unsigned char b = (depth < (int)item.first.size()) ? item.first[depth] : 0;
        partitions[b].push_back(item);
    }

    // Count non-empty partitions.
    int nonEmptyCount = 0;
    for (int i = 0; i < 256; i++) {
        if (!partitions[i].empty())
            nonEmptyCount++;
    }

    // Choose an internal node type based on nonEmptyCount.
    ARTNode* node = nullptr;
    if (nonEmptyCount <= 4) {
        node = new ARTNode4();
    } else if (nonEmptyCount <= 16) {
        node = new ARTNode16();
    } else {
        node = new ARTNode256();
    }

    // For each possible byte value, if the partition is non-empty, recursively build a child.
    if (node->type == NodeType::NODE4) {
        ARTNode4* n4 = static_cast<ARTNode4*>(node);
        for (int b = 0; b < 256; b++) {
            if (!partitions[b].empty()) {
                ARTNode* child = ART_bulkLoad(partitions[b], depth + 1);
                // For the first child, no separator key is stored.
                if (n4->count > 0)
                    n4->keys[n4->count] = (unsigned char)b;
                n4->children[n4->count] = child;
                n4->count++;
            }
        }
    } else if (node->type == NodeType::NODE16) {
        ARTNode16* n16 = static_cast<ARTNode16*>(node);
        for (int b = 0; b < 256; b++) {
            if (!partitions[b].empty()) {
                ARTNode* child = ART_bulkLoad(partitions[b], depth + 1);
                if (n16->count > 0)
                    n16->keys[n16->count] = (unsigned char)b;
                n16->children[n16->count] = child;
                n16->count++;
            }
        }
    } else if (node->type == NodeType::NODE256) {
        ARTNode256* n256 = static_cast<ARTNode256*>(node);
        // In Node256, we use the direct index.
        for (int b = 0; b < 256; b++) {
            if (!partitions[b].empty()) {
                ARTNode* child = ART_bulkLoad(partitions[b], depth + 1);
                n256->children[b] = child;
                n256->count++;
            }
        }
    }
    return node;
}
