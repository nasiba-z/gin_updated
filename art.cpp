// ART.cpp
#include "art.h"
#include <iostream>
#include <emmintrin.h>   // for _mm_cmpeq_epi8
#include <tmmintrin.h>   // for _mm_movemask_epi8 (SSE3)
#include <array>         // for std::array



// ---------------------
// ARTLeaf Implementation
// ---------------------
ARTLeaf::ARTLeaf(const std::vector<unsigned char>& key, IndexTuple* v)
    : ARTNode(NodeType::LEAF), fullKey(key), value(v) {}

// ARTLeaf::~ARTLeaf() {
//     delete value;
// }


ARTNode* ARTLeaf::insert(const unsigned char* key, int keyLen, int depth, IndexTuple* newValue) {
    if (fullKey.size() == static_cast<size_t>(keyLen) &&
        memcmp(fullKey.data(), key, keyLen) == 0) {
        value = newValue;
        return this;
    }
    
    // Otherwise, we have a key mismatch and need to split this leaf.
    // Create a new inner node (using Node4 for simplicity).
    ARTNode4* newNode = new ARTNode4();
    
    // Compute the common prefix length between the existing leaf key and the new key,
    // starting at the given depth.
    int i = depth;
    int maxLen = std::min(static_cast<int>(fullKey.size()), keyLen);
    while (i < maxLen && fullKey[i] == key[i])
        i++;
    int commonPrefixLen = i - depth;
    
    // Store the common prefix in the new inner node.
    newNode->prefix.resize(commonPrefixLen);
    memcpy(newNode->prefix.data(), key + depth, commonPrefixLen);
    newNode->prefixLen = commonPrefixLen;
    
    // Advance depth past the common prefix.
    depth += commonPrefixLen;
    
    // Determine the branching bytes for both keys at the current depth.
    unsigned char existingByte = (depth < (int)fullKey.size()) ? fullKey[depth] : 0;
    unsigned char newByte = (depth < keyLen) ? key[depth] : 0;
    
    // Create a new leaf for the new key.
    std::vector<unsigned char> newKeyVec(key, key + keyLen);
    ARTLeaf* newLeaf = new ARTLeaf(newKeyVec, newValue);
    
    // Insert both children into newNode in sorted order by the branching byte.
    if (existingByte < newByte) {
        newNode->keys[0] = existingByte;
        newNode->children[0] = this;   // existing leaf
        newNode->keys[1] = newByte;
        newNode->children[1] = newLeaf;
    } else {
        newNode->keys[0] = newByte;
        newNode->children[0] = newLeaf;
        newNode->keys[1] = existingByte;
        newNode->children[1] = this;   // existing leaf
    }
    newNode->count = 2;
    
    return newNode;
    }

IndexTuple* ARTLeaf::search(const unsigned char* key, int keyLen, int depth) const {
    if (fullKey.size() == static_cast<size_t>(keyLen) &&
        memcmp(fullKey.data(), key, keyLen) == 0)
        return value;
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
ARTNode* ARTNode4::insert(const unsigned char* key, int keyLen, int depth, IndexTuple* value) {
    int d = depth;

    // If the node has a stored prefix, check if it matches the key.
    if (prefixLen > 0) {
        int i = 0;
        int maxCmp = std::min(prefixLen, keyLen - d);
        while (i < maxCmp && prefix[i] == key[d + i])
            i++;

        if (i != prefixLen) {
            // Mismatch in the stored prefix; need to split this node.
            ARTNode4* newNode = new ARTNode4();

            // Store the common prefix into the new node.
            newNode->prefix.resize(i);
            memcpy(newNode->prefix.data(), prefix.data(), i);
            newNode->prefixLen = i;
            
            // Adjust the current node's prefix: remove the common part.
            prefix.erase(prefix.begin(), prefix.begin() + i);
            prefixLen -= i;
            
            // Insert the current node as a child of newNode.
            unsigned char existingByte = prefix[0]; // first byte of the adjusted prefix
            newNode->keys[newNode->count] = existingByte;
            newNode->children[newNode->count] = this;
            newNode->count++;
            
            // Create a new leaf for the inserted key.
            std::vector<unsigned char> fullKey(key, key + keyLen);
            ARTLeaf* newLeaf = new ARTLeaf(fullKey, value);
            
            // Determine the branching byte for the new key.
            unsigned char newByte = (d + i < keyLen) ? key[d + i] : 0;
            
            // Insert the new leaf in sorted order into newNode.
            int pos = 0;
            while (pos < newNode->count && newNode->keys[pos] < newByte)
                pos++;
            for (int j = newNode->count; j > pos; j--) {
                newNode->keys[j] = newNode->keys[j - 1];
                newNode->children[j] = newNode->children[j - 1];
            }
            newNode->keys[pos] = newByte;
            newNode->children[pos] = newLeaf;
            newNode->count++;
            
            return newNode;
        }
        // If the prefix completely matches, advance the depth.
        d += prefixLen;
    }
    
    // If the key is completely consumed, handle accordingly (here, we just return this node).
    if (d >= keyLen) {
        return this;
    }
    
    // Find an existing child that matches the key byte at the current depth.
    unsigned char currentKey = key[d];
    for (int i = 0; i < count; i++) {
        if (keys[i] == currentKey) {
            ARTNode* child = children[i];
            ARTNode* newChild = child->insert(key, keyLen, d + 1, value);
            children[i] = newChild;
            return this;
        }
    }
    
    // There's no existing child for currentKey; insert a new leaf.
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
    // Node4 is full; grow to a Node16.
    else {
        ARTNode16* newNode = new ARTNode16();
        newNode->prefix = this->prefix;
        newNode->prefixLen = this->prefixLen;
        for (int i = 0; i < count; i++) {
            newNode->keys[i] = keys[i];
            newNode->children[i] = children[i];
        }
        newNode->count = count;
        delete this;
        return newNode->insert(key, keyLen, d, value);
    }
    
    assert(false && "Node4 insertion reached an unreachable state.");
    return this;
}
IndexTuple* ARTNode4::search(const unsigned char* key, int keyLen, int depth) const {
    int d = depth;
    // prefix check
    if (prefixLen > 0) {
        if (keyLen - d < prefixLen || memcmp(prefix.data(), key + d, prefixLen) != 0)
            return nullptr;
        d += prefixLen;
    }
    if (d >= keyLen)
        return nullptr;
    unsigned char b = key[d];
    // simple linear search among up to 4 keys
    for (int i = 0; i < count; ++i) {
        if (keys[i] == b)
            return children[i]->search(key, keyLen, d + 1);
    }
    return nullptr;
}
// IndexTuple* ARTNode4::search(const unsigned char* key, int keyLen, int depth) const {
//     int d = depth;
//     // If there is a stored prefix in this node, check that the search key matches.
//     if (prefixLen > 0) {
//         if (keyLen - depth < prefixLen) return nullptr;
//         if (memcmp(prefix.data(), key + depth, prefixLen) != 0)
//             return nullptr;
//         d += prefixLen;
//     }
//     if (d >= keyLen) return nullptr;
//     unsigned char currentKey = key[d];
//     for (int i = 0; i < count; i++) {
//         if (keys[i] == currentKey) {
//             return children[i]->search(key, keyLen, d + 1);
//         }
//     }
//     return nullptr;
// }

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

ARTNode* ARTNode16::insert(const unsigned char* key, int keyLen, int depth, IndexTuple* value) {
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
    // Node16 is full; grow to a Node48.
    else {
        ARTNode48* newNode = new ARTNode48();
        newNode->prefix = this->prefix;
        newNode->prefixLen = this->prefixLen;
        // Copy each child from Node16 into Node48.
        for (int i = 0; i < count; i++) {
            unsigned char k = keys[i];
            newNode->childIndex[k] = newNode->count;
            newNode->children[newNode->count] = children[i];
            newNode->count++;
        }
        delete this;
        return newNode->insert(key, keyLen, depth, value);
    }
    assert(false && "Node16 is full; node growth is not implemented.");
    return this;
}

IndexTuple* ARTNode16::search(const unsigned char* key, int keyLen, int depth) const {
    int d = depth;
    // prefix check
    if (prefixLen > 0) {
        if (keyLen - d < prefixLen || memcmp(prefix.data(), key + d, prefixLen) != 0)
            return nullptr;
        d += prefixLen;
    }
    if (d >= keyLen)
        return nullptr;
    unsigned char b = key[d];

    // Broadcast the search byte
    __m128i cmp_key = _mm_set1_epi8((char)b);
    // Load stored keys (aligned or unaligned)
    __m128i key_vec = _mm_loadu_si128(reinterpret_cast<const __m128i*>(keys));
    // Compare
    __m128i cmp = _mm_cmpeq_epi8(cmp_key, key_vec);
    // Mask to valid entries
    int mask = (1 << count) - 1;
    int bits = _mm_movemask_epi8(cmp) & mask;
    if (bits == 0)
        return nullptr;
    int idx = __builtin_ctz(bits);
    return children[idx]->search(key, keyLen, d + 1);
}

// IndexTuple* ARTNode16::search(const unsigned char* key, int keyLen, int depth) const {
//     if (depth >= keyLen) return nullptr;
//     unsigned char currentKey = key[depth];
//     for (int i = 0; i < count; i++) {
//         if (keys[i] == currentKey) {
//             return children[i]->search(key, keyLen, depth + 1);
//         }
//     }
//     return nullptr;
// }

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

ARTNode* ARTNode48::insert(const unsigned char* key, int keyLen, int depth, IndexTuple* value) {
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
    // Node48 is full; grow to a Node256.
    else {
        ARTNode256* newNode = new ARTNode256();
        newNode->prefix = this->prefix;
        newNode->prefixLen = this->prefixLen;
        // For every possible key byte, if childIndex is valid, copy the child.
        for (int i = 0; i < 256; i++) {
            if (childIndex[i] != 0xFF) {
                int pos = childIndex[i];
                newNode->children[i] = children[pos];
                newNode->count++;
            }
        }
        delete this;
        return newNode->insert(key, keyLen, depth, value);
    }
    assert(false && "Node48 is full; node growth is not implemented.");
    return this;
}
IndexTuple* ARTNode48::search(const unsigned char* key, int keyLen, int depth) const {
    int d = depth;
    // prefix check
    if (prefixLen > 0) {
        if (keyLen - d < prefixLen || memcmp(prefix.data(), key + d, prefixLen) != 0)
            return nullptr;
        d += prefixLen;
    }
    if (d >= keyLen)
        return nullptr;
    unsigned char b = key[d];
    int idx = childIndex[b];
    if (idx == 0xFF)
        return nullptr;
    return children[idx]->search(key, keyLen, d + 1);
}

// IndexTuple* ARTNode48::search(const unsigned char* key, int keyLen, int depth) const {
//     if (depth >= keyLen) return nullptr;
//     unsigned char currentKey = key[depth];
//     if (childIndex[currentKey] == 0xFF)
//         return nullptr;
//     int idx = childIndex[currentKey];
//     return children[idx]->search(key, keyLen, depth + 1);
// }

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

ARTNode* ARTNode256::insert(const unsigned char* key, int keyLen, int depth, IndexTuple* value) {
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

IndexTuple* ARTNode256::search(const unsigned char* key, int keyLen, int depth) const {
    if (depth >= keyLen) return nullptr;
    unsigned char currentKey = key[depth];
    if (children[currentKey] == nullptr)
        return nullptr;
    return children[currentKey]->search(key, keyLen, depth + 1);
}

// Helper function: Recursively bulk load ART from sorted items.
ARTNode* ART_bulkLoad(const std::vector<std::pair<std::vector<unsigned char>, IndexTuple*>>& items, int depth) {
    // Base case: If there are no items, return nullptr.
    if (items.empty())
        return nullptr;

    // If the number of items is small or we've reached the end of the key,
    // create a leaf-level container.
    if (items.size() <= LEAF_THRESHOLD || depth >= (int)items[0].first.size()) {
        // If there's only one item, create and return a single ARTLeaf node.
        if (items.size() == 1) {
            return new ARTLeaf(items[0].first, items[0].second);
        }
        // Otherwise, create a Node4 to store the items as children.
        ARTNode4* node = new ARTNode4();
        // Insert each item as a child leaf.
        for (size_t i = 0; i < items.size(); i++) {
            ARTLeaf* leaf = new ARTLeaf(items[i].first, items[i].second);
            // Use the byte at the current depth as the separator key.
            unsigned char separator = (depth < items[i].first.size()) ? items[i].first[depth] : 0;
            node->keys[node->count] = separator;
            node->children[node->count] = leaf;
            node->count++;
        }
        return node;
    }

    // 1. Partition into 256 buckets (unchanged)
    std::array<std::vector<std::pair<std::vector<unsigned char>,IndexTuple*>>,256> partitions;
    for (auto &item : items) {
        unsigned char b = (depth < (int)item.first.size())
                        ? item.first[depth]
                        : 0;
        partitions[b].push_back(item);
    }

    // 2. Build a small vector of only the non-empty byte values
    int nonEmptyCount = 0;
    for (int b = 0; b < 256; ++b)
        if (!partitions[b].empty())
            ++nonEmptyCount;

    std::vector<unsigned char> keysPresent;
    keysPresent.reserve(nonEmptyCount);
    for (int b = 0; b < 256; ++b) {
        if (!partitions[b].empty())
            keysPresent.push_back((unsigned char)b);
    }

    // 3. Choose node type using nonEmptyCount
    ARTNode* node = nullptr;
    if      (nonEmptyCount <= 4)  node = new ARTNode4();
    else if (nonEmptyCount <= 16) node = new ARTNode16();
    else if (nonEmptyCount <= 48) node = new ARTNode48();
    else                          node = new ARTNode256();

    // 4. Recurse only over those non-empty buckets
    switch (node->type) {
    case NodeType::NODE4: {
        auto n4 = static_cast<ARTNode4*>(node);
        for (auto b: keysPresent) {
        auto &bucket = partitions[b];
        auto child  = ART_bulkLoad(bucket, depth+1);
        n4->keys   [n4->count] = b;
        n4->children[n4->count] = child;
        ++n4->count;
        }
        break;
    }
    case NodeType::NODE16: {
        auto n16 = static_cast<ARTNode16*>(node);
        for (auto b: keysPresent) {
        auto &bucket = partitions[b];
        auto child  = ART_bulkLoad(bucket, depth+1);
        n16->keys   [n16->count] = b;
        n16->children[n16->count] = child;
        ++n16->count;
        }
        break;
    }
    case NodeType::NODE48: {
        auto n48 = static_cast<ARTNode48*>(node);
        for (auto b: keysPresent) {
        auto &bucket = partitions[b];
        auto child  = ART_bulkLoad(bucket, depth+1);
        n48->childIndex[b]      = n48->count;
        n48->children [n48->count] = child;
        ++n48->count;
        }
        break;
    }
    case NodeType::NODE256: {
        auto n256 = static_cast<ARTNode256*>(node);
        for (auto b: keysPresent) {
        auto &bucket = partitions[b];
        auto child  = ART_bulkLoad(bucket, depth+1);
        n256->children[b] = child;
        ++n256->count;
        }
        break;
    }
    }

    return node;
}