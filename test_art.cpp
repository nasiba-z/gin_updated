#include <iostream>
#include <vector>
#include <cstring>
#include <cassert>

// Enumeration for node types.
enum class NodeType { NODE4, LEAF };

// Base class for all ART nodes.
class ARTNode {
public:
    NodeType type;
    ARTNode(NodeType t) : type(t) {}
    virtual ~ARTNode() {}

    // Insert a key/value pair starting at a given depth.
    virtual ARTNode* insert(const unsigned char* key, int keyLen, int depth, int value) = 0;
    // Search for a key starting at a given depth.
    virtual int* search(const unsigned char* key, int keyLen, int depth) const = 0;
};

// Leaf node storing the full key and its associated value.
class ARTLeaf : public ARTNode {
public:
    std::vector<unsigned char> fullKey;
    int value;

    ARTLeaf(const std::vector<unsigned char>& key, int v)
        : ARTNode(NodeType::LEAF), fullKey(key), value(v) {}

    ARTNode* insert(const unsigned char* key, int keyLen, int depth, int newValue) override {
        // If key already exists, update value.
        if (fullKey.size() == static_cast<size_t>(keyLen) &&
            memcmp(fullKey.data(), key, keyLen) == 0) {
            value = newValue;
        }
        // In a full implementation, a key mismatch would trigger a split.
        return this;
    }

    int* search(const unsigned char* key, int keyLen, int depth) const override {
        if (fullKey.size() == static_cast<size_t>(keyLen) &&
            memcmp(fullKey.data(), key, keyLen) == 0)
            return const_cast<int*>(&value);
        return nullptr;
    }
};

// Node4 implementation based exactly on the given text.
// - It can store up to 4 children.
// - It uses two arrays: one for keys and one for pointers.
// - The keys and pointers are stored at corresponding positions, and the keys are kept sorted.
class ARTNode4 : public ARTNode {
public:
    unsigned char keys[4];    // Array of length 4 for keys.
    ARTNode* children[4];     // Array of length 4 for child pointers.
    int count;                // Current number of children.

    ARTNode4() : ARTNode(NodeType::NODE4), count(0) {
        memset(keys, 0, sizeof(keys));
        memset(children, 0, sizeof(children));
    }

    ~ARTNode4() override {
        for (int i = 0; i < count; i++) {
            delete children[i];
        }
    }

    // Insert a key/value pair.
    // If a child corresponding to the current key byte exists, delegate insertion.
    // Otherwise, create a new leaf and insert it in sorted order.
    ARTNode* insert(const unsigned char* key, int keyLen, int depth, int value) override {
        assert(depth < keyLen);
        unsigned char currentKey = key[depth];

        // Check if the current key byte is already present.
        for (int i = 0; i < count; i++) {
            if (keys[i] == currentKey) {
                ARTNode* child = children[i];
                ARTNode* newChild = child->insert(key, keyLen, depth + 1, value);
                children[i] = newChild;
                return this;
            }
        }

        // If there is room, insert the new key.
        if (count < 4) {
            // Create a new leaf node for the key.
            std::vector<unsigned char> fullKey(key, key + keyLen);
            ARTLeaf* newLeaf = new ARTLeaf(fullKey, value);

            // Find the position to insert to keep keys sorted.
            int pos = 0;
            while (pos < count && keys[pos] < currentKey)
                pos++;

            // Shift keys and children to the right to make space.
            for (int i = count; i > pos; i--) {
                keys[i] = keys[i - 1];
                children[i] = children[i - 1];
            }

            keys[pos] = currentKey;
            children[pos] = newLeaf;
            count++;
            return this;
        }

        // If node is full, this simplified implementation cannot insert further.
        assert(false && "Node4 is full; node growth is not implemented.");
        return this;
    }

    // Search for a key by comparing the key byte at the current depth.
    // Delegates to the appropriate child if found.
    int* search(const unsigned char* key, int keyLen, int depth) const override {
        if (depth >= keyLen) return nullptr;
        unsigned char currentKey = key[depth];
        for (int i = 0; i < count; i++) {
            if (keys[i] == currentKey) {
                return children[i]->search(key, keyLen, depth + 1);
            }
        }
        return nullptr;
    }
};

// Example usage: constructing a small ART with Node4 only.
int main() {
    ARTNode* root = new ARTNode4();

    // Insert three keys. For simplicity, we use 3-character strings.
    const char* key1 = "abc";
    const char* key2 = "abd";
    const char* key3 = "xyz";

    root = root->insert(reinterpret_cast<const unsigned char*>(key1), 3, 0, 100);
    root = root->insert(reinterpret_cast<const unsigned char*>(key2), 3, 0, 200);
    root = root->insert(reinterpret_cast<const unsigned char*>(key3), 3, 0, 300);

    int* result = root->search(reinterpret_cast<const unsigned char*>("abd"), 3, 0);
    if (result)
        std::cout << "Found key 'abd' with value: " << *result << std::endl;
    else
        std::cout << "Key 'abd' not found." << std::endl;

    delete root;
    return 0;
}
