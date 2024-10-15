#include <iostream>
#include <memory>
#include <stack>
#include <optional>
#include <vector>

template<typename K, typename V>
struct Pair {
    K key;
    V value;
    std::unique_ptr<Pair<K, V>> left;
    std::unique_ptr<Pair<K, V>> right;
    Pair(const K& k, const V& v) : key(k), value(v), left(nullptr), right(nullptr) {}
};

template<typename K, typename V>
class Iterator {
private:
    std::stack<Pair<K, V>*> stack;

    void pushLeft(Pair<K, V>* node) {
        while (node) {
            stack.push(node);
            node = node->left.get();
        }
    }

public:
    Iterator(Pair<K, V>* root) {
        pushLeft(root);
    }

    std::pair<const K&, V&> operator*() const {
        return {stack.top()->key, stack.top()->value};
    }

    Iterator& operator++() {
        if (stack.empty()) {
            return *this;
        }

        auto current = stack.top();
        stack.pop();
        if (current->right) {
            pushLeft(current->right.get());
        }
        return *this;
    }

    bool operator!=(const Iterator& other) const {
        return stack != other.stack;
    }

    bool operator==(const Iterator& other) const {
        return stack.empty() && other.stack.empty();
    }
};

template<typename K, typename V>
class SearchingTree {
private:
    std::unique_ptr<Pair<K, V>> root;

    void collectInRange(Pair<K, V>* node, K a, K b, std::vector<std::pair<K, V>>& result) {
        if (!node) {
            return;
        }
        if (node->key >= a) {
            collectInRange(node->left.get(), a, b, result);
        }
        if (node->key >= a && node->key < b) {
            result.emplace_back(node->key, node->value);
        }
        if (node->key < b) {
            collectInRange(node->right.get(), a, b, result);
        }
    }

    std::unique_ptr<Pair<K, V>> eraseNode(std::unique_ptr<Pair<K, V>> node, const K& key) {
        if (!node) {
            return nullptr;
        } 

        if (key < node->key) {
            node->left = eraseNode(std::move(node->left), key);
        } else if (key > node->key) {
            node->right = eraseNode(std::move(node->right), key);
        } else {
            if (!node->left) {
                return std::move(node->right);
            }

            if (!node->right) {
                return std::move(node->left);
            }

            Pair<K, V>* minNode = findMinNode(node->right.get());
            node->key = minNode->key;
            node->value = minNode->value;
            node->right = eraseNode(std::move(node->right), minNode->key);
        }
        return node;
    }

    Pair<K, V>* findMinNode(Pair<K, V>* node) const {
        while (node->left) {
            node = node->left.get();
        }
        return node;
    }

    Pair<K, V>* findNode(const K& key) const {
        Pair<K, V>* node = root.get();
        while (node) {
            if (key < node->key) {
                node = node->left.get();
            } else if (key > node->key) {
                node = node->right.get();
            } else {
                return node;
            }
        }
        return nullptr;
    }

public:
    void insert(const K& key, const V& value) {
        std::unique_ptr<Pair<K, V>>* node = &root;
        while (*node) {
            if (key < (*node)->key) {
                node = &(*node)->left;
            } else if (key > (*node)->key) {
                node = &(*node)->right;
            } else {
                return;
            }
        }
        *node = std::make_unique<Pair<K, V>>(key, value);
    } 
    void erase(const K &key) {
        root = eraseNode(std::move(root), key);
    }

    std::optional<std::pair<K, V>> find(const K& key) const {
        Pair<K, V>* node = findNode(key);
        if (node) {
            return std::make_pair(node->key, node->value);
        }
        return std::nullopt;
    }

    Iterator<K, V> begin() const {
        return Iterator<K, V>(root.get());
    }

    Iterator<K, V> end() const {
        return Iterator<K, V>(nullptr);
    }

    std::vector<std::pair<K, V>> range(K a, K b) {
        std::vector<std::pair<K, V>> result;
        if (a < b) {
            collectInRange(root.get(), a, b, result);    
        } else {
            collectInRange(root.get(), b, a, result); 
            std::vector<std::pair<K, V>> rev_res(result.rbegin(), result.rend());
            return rev_res;
        }
        
        return result;
    }

};

int main()
{
    SearchingTree<int, std::string> tree;
    tree.insert(5, "root");
    tree.insert(3, "left");
    tree.insert(7, "right");
    tree.insert(2, "left-left");
    tree.insert(4, "left-right");
    tree.insert(6, "right-left");
    tree.insert(8, "right-right");

    tree.erase(3);

    auto found = tree.find(4);
    if (found) {
        std::cout << "Found: " << found->first << ", " << found->second << std::endl;
    } else {
        std::cout << "Not found" << std::endl;

    }

    for (const auto &[k, v] : tree) {
        std::cout << k << ": " << v << std::endl;
    }

    int a = 3, b = 7;
    for (const auto &[k, v] : tree.range(a, b)) {
        std::cout << k << ": " << v << std::endl;
    }

    return 0;
}
