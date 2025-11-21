#ifndef _B_TREE_HPP
#define _B_TREE_HPP

#include <array>
#include <cstddef>
#include <functional>
#include <utility>
#include <cassert>

/**
 * @brief B树容器所在的命名空间。Namespace that contains B-tree containers.
 */
namespace test_forest
{

    /**
     * @brief B树集合（set）容器，存储唯一有序键。B-tree set container storing unique ordered keys.
     *
     * @tparam Key    键类型（key type）
     * @tparam Order  B树阶数（最大子节点数，max children per node）
     * @tparam Compare 比较器（comparator used for ordering keys）
     *
     * @example
     * // 构建一个整数B树并插入/查询
     * // Build an integer B-tree and insert / query
     * test_forest::BTreeSet<int> tree;
     * tree.insert(10);
     * tree.insert(5);
     * tree.insert(20);
     * bool has10 = tree.contains(10);
     */
    template <typename Key,
              std::size_t Order = 32,
              typename Compare = std::less<Key>>
    class BTreeSet
    {
        static_assert(Order >= 3, "BTreeSet<Order>: Order must be >= 3");

    public:
        /// @brief 键类型。Key type.
        using key_type = Key;
        /// @brief 比较器类型。Comparator type.
        using key_compare = Compare;
        /// @brief 元素计数类型。Size / count type.
        using size_type = std::size_t;

    private:
        /// @brief 每个节点最多键数量。Maximum keys per node.
        static constexpr std::size_t kMaxKeys = Order - 1;
        /// @brief 每个节点最多子节点数量。Maximum children per node.
        static constexpr std::size_t kMaxChildren = Order;
        /// @brief 每个非根节点最少键数量。Minimum keys per non-root node.
        static constexpr std::size_t kMinKeys = (Order / 2) - 1;

        /**
         * @brief B树节点。B-tree node.
         *
         * @note 为了简单，节点使用固定大小数组（std::array）存储键和子指针。
         *       For simplicity, nodes use fixed-size std::array for keys and children pointers.
         */
        struct Node
        {
            /// @brief 是否为叶子节点。Whether this node is a leaf.
            bool leaf;
            /// @brief 当前键数量。Current number of keys stored.
            std::size_t count;
            /// @brief 节点中的键。Keys stored in this node.
            std::array<key_type, kMaxKeys> keys;
            /// @brief 子节点指针。Pointers to child nodes.
            std::array<Node *, kMaxChildren> children;

            /**
             * @brief 构造函数：创建叶子或内部节点。Constructor: create leaf or internal node.
             * @param is_leaf 是否为叶子节点 / whether this node is a leaf.
             */
            explicit Node(bool is_leaf) noexcept
                : leaf(is_leaf), count(0), keys{}, children{}
            {
                children.fill(nullptr);
            }
        };

    public:
        /**
         * @brief 默认构造一个空的 B树。Default-construct an empty B-tree.
         */
        BTreeSet() noexcept(std::is_nothrow_default_constructible_v<Compare>)
            : root_(nullptr), size_(0), comp_()
        {
        }

        /**
         * @brief 使用给定比较器构造 B树。Construct B-tree with a given comparator.
         * @param comp 比较器实例 / comparator instance.
         */
        explicit BTreeSet(const key_compare &comp)
            : root_(nullptr), size_(0), comp_(comp)
        {
        }

        /**
         * @brief 拷贝构造函数。Copy constructor.
         * @param other 要拷贝的另一棵树 / other tree to copy.
         */
        BTreeSet(const BTreeSet &other)
            : root_(nullptr), size_(0), comp_(other.comp_)
        {
            root_ = clone_subtree(other.root_);
            size_ = other.size_;
        }

        /**
         * @brief 移动构造函数。Move constructor.
         * @param other 要移动的另一棵树 / other tree to move from.
         */
        BTreeSet(BTreeSet &&other) noexcept
            : root_(other.root_), size_(other.size_), comp_(std::move(other.comp_))
        {
            other.root_ = nullptr;
            other.size_ = 0;
        }

        /**
         * @brief 拷贝赋值运算符。Copy assignment operator.
         * @param other 要拷贝的另一棵树 / other tree to copy from.
         * @return 当前对象的引用 / reference to *this.
         */
        BTreeSet &operator=(const BTreeSet &other)
        {
            if (this == &other)
            {
                return *this;
            }
            clear();
            comp_ = other.comp_;
            root_ = clone_subtree(other.root_);
            size_ = other.size_;
            return *this;
        }

        /**
         * @brief 移动赋值运算符。Move assignment operator.
         * @param other 要移动的另一棵树 / other tree to move from.
         * @return 当前对象的引用 / reference to *this.
         */
        BTreeSet &operator=(BTreeSet &&other) noexcept
        {
            if (this == &other)
            {
                return *this;
            }
            clear();
            root_ = other.root_;
            size_ = other.size_;
            comp_ = std::move(other.comp_);
            other.root_ = nullptr;
            other.size_ = 0;
            return *this;
        }

        /**
         * @brief 析构函数：释放整棵 B树。Destructor: free the whole B-tree.
         */
        ~BTreeSet()
        {
            clear();
        }

        /**
         * @brief 返回当前元素个数。Return number of elements.
         * @return 元素数量 / number of stored keys.
         */
        [[nodiscard]] size_type size() const noexcept
        {
            return size_;
        }

        /**
         * @brief 判断 B树是否为空。Check whether the tree is empty.
         * @return 若为空返回 true，否则返回 false / true if empty, false otherwise.
         */
        [[nodiscard]] bool empty() const noexcept
        {
            return size_ == 0;
        }

        /**
         * @brief 清空整棵 B树并释放所有节点。Clear the whole tree and free all nodes.
         */
        void clear() noexcept
        {
            destroy_subtree(root_);
            root_ = nullptr;
            size_ = 0;
        }

        /**
         * @brief 插入一个键（若已存在则不插入）。Insert a key; if already exists, do nothing.
         *
         * @param key 要插入的键 / key to insert.
         * @return 若插入成功（之前不存在）返回 true；若键已存在返回 false。
         *         Returns true if a new key was inserted, false if key already existed.
         */
        bool insert(const key_type &key)
        {
            if (!root_)
            {
                root_ = new Node(true);
                root_->keys[0] = key;
                root_->count = 1;
                ++size_;
                return true;
            }

            if (contains(key))
            {
                return false;
            }

            if (root_->count == kMaxKeys)
            {
                Node *new_root = new Node(false);
                new_root->children[0] = root_;
                split_child(new_root, 0, root_);
                root_ = new_root;
            }

            bool inserted = insert_non_full(root_, key);
            if (inserted)
            {
                ++size_;
            }
            return inserted;
        }

        /**
         * @brief 删除一个键（若不存在则什么也不做）。Erase a key; no-op if not found.
         *
         * @param key 要删除的键 / key to erase.
         * @return 若删除成功（确实存在该键）返回 true，否则 false。
         *         Returns true if the key was present and erased, false otherwise.
         */
        bool erase(const key_type &key)
        {
            if (!root_)
            {
                return false;
            }

            bool erased = erase_internal(root_, key);
            if (!erased)
            {
                return false;
            }

            if (root_->count == 0 && !root_->leaf)
            {
                Node *old_root = root_;
                root_ = root_->children[0];
                delete old_root;
            }

            --size_;
            if (size_ == 0)
            {
                destroy_subtree(root_);
                root_ = nullptr;
            }
            return true;
        }

        /**
         * @brief 判断是否包含给定键。Check whether the tree contains a key.
         *
         * @param key 要查找的键 / key to search for.
         * @return 若找到返回 true，否则 false。Returns true if found, false otherwise.
         */
        [[nodiscard]] bool contains(const key_type &key) const
        {
            return search_node(root_, key) != nullptr;
        }

        /**
         * @brief 中序遍历所有键，对每个键调用回调函数。Traverse all keys in-order and call a callback.
         *
         * @tparam Func 可调用对象类型（callable type）
         * @param f 对每个键调用的函数 f(key) / function to be called for each key.
         *
         * @note 这是无迭代器版本的“for_each”，方便性能测试和验证。This is an iterator-free for_each
         *       style traversal, convenient for benchmarking and verification.
         */
        template <typename Func>
        void traverse_in_order(Func &&f) const
        {
            traverse_in_order_impl(root_, std::forward<Func>(f));
        }

        /**
         * @brief 获取比较器对象。Get the comparator object.
         * @return 当前使用的比较器 / current comparator.
         */
        [[nodiscard]] key_compare key_comp() const
        {
            return comp_;
        }

    private:
        /// @brief 树根指针。Pointer to root node.
        Node *root_;
        /// @brief 当前元素数。Current number of elements.
        size_type size_;
        /// @brief 键比较器。Comparator for keys.
        key_compare comp_;

        /**
         * @brief 销毁以 node 为根的子树。Destroy subtree rooted at node.
         * @param node 子树根节点 / subtree root.
         */
        static void destroy_subtree(Node *node) noexcept
        {
            if (!node)
            {
                return;
            }
            if (!node->leaf)
            {
                for (std::size_t i = 0; i <= node->count; ++i)
                {
                    destroy_subtree(node->children[i]);
                }
            }
            delete node;
        }

        /**
         * @brief 递归拷贝子树。Recursively clone a subtree.
         * @param node 要拷贝的子树根节点 / root of subtree to clone.
         * @return 新子树的根指针 / pointer to cloned root.
         */
        static Node *clone_subtree(Node *node)
        {
            if (!node)
            {
                return nullptr;
            }
            Node *new_node = new Node(node->leaf);
            new_node->count = node->count;
            for (std::size_t i = 0; i < node->count; ++i)
            {
                new_node->keys[i] = node->keys[i];
            }
            if (!node->leaf)
            {
                for (std::size_t i = 0; i <= node->count; ++i)
                {
                    new_node->children[i] = clone_subtree(node->children[i]);
                }
            }
            return new_node;
        }

        /**
         * @brief 在节点中二分查找键位置。Binary-search key position in a node.
         *
         * @param node 当前节点 / current node.
         * @param key  要定位的键 / key to locate.
         * @return 返回第一个满足 !comp_(node->keys[i], key) 的下标 i。
         *         Returns first index i such that !comp_(node->keys[i], key).
         */
        std::size_t find_key_index(const Node *node, const key_type &key) const
        {
            std::size_t left = 0;
            std::size_t right = node->count; // [left, right)
            while (left < right)
            {
                std::size_t mid = left + (right - left) / 2;
                if (comp_(node->keys[mid], key))
                {
                    left = mid + 1;
                }
                else
                {
                    right = mid;
                }
            }
            return left;
        }

        /**
         * @brief 在整棵树中搜索键所在节点。Search the key in the whole tree.
         * @param node 当前节点 / current node.
         * @param key  待搜索键 / key to search for.
         * @return 若找到返回节点指针，否则返回 nullptr。Pointer to node if found, nullptr otherwise.
         */
        Node *search_node(Node *node, const key_type &key) const
        {
            while (node)
            {
                std::size_t i = find_key_index(node, key);
                if (i < node->count && !comp_(key, node->keys[i]) && !comp_(node->keys[i], key))
                {
                    return node;
                }
                if (node->leaf)
                {
                    return nullptr;
                }
                node = node->children[i];
            }
            return nullptr;
        }

        /**
         * @brief 分裂满子节点。Split a full child node.
         *
         * @param parent 父节点 / parent node.
         * @param index  子节点在父节点中的下标 / index of child in parent.
         * @param child  已满子节点 / full child node.
         *
         * @note 分裂后：中间键上升到父节点，child 被分成 child 与 new_child。
         *       After split: middle key moves up to parent, child is split into child + new_child.
         */
        void split_child(Node *parent, std::size_t index, Node *child)
        {
            assert(child->count == kMaxKeys);
            Node *new_child = new Node(child->leaf);

            const std::size_t mid = child->count / 2;
            new_child->count = child->count - mid - 1;

            for (std::size_t j = 0; j < new_child->count; ++j)
            {
                new_child->keys[j] = child->keys[mid + 1 + j];
            }

            if (!child->leaf)
            {
                for (std::size_t j = 0; j <= new_child->count; ++j)
                {
                    new_child->children[j] = child->children[mid + 1 + j];
                }
            }

            child->count = mid;

            for (std::size_t j = parent->count + 1; j > index + 1; --j)
            {
                parent->children[j] = parent->children[j - 1];
            }
            parent->children[index + 1] = new_child;

            for (std::size_t j = parent->count; j > index; --j)
            {
                parent->keys[j] = parent->keys[j - 1];
            }
            parent->keys[index] = child->keys[mid];
            parent->count += 1;
        }

        /**
         * @brief 向非满节点中插入键。Insert a key into a non-full node.
         *
         * @param node 当前节点（保证非满）/ current non-full node.
         * @param key  要插入的键 / key to insert.
         * @return 总是返回 true（调用前应保证键不存在）Always returns true (caller must ensure key is unique).
         */
        bool insert_non_full(Node *node, const key_type &key)
        {
            std::size_t i = node->count;

            if (node->leaf)
            {
                while (i > 0 && comp_(key, node->keys[i - 1]))
                {
                    node->keys[i] = node->keys[i - 1];
                    --i;
                }
                node->keys[i] = key;
                node->count += 1;
                return true;
            }

            i = find_key_index(node, key);
            Node *child = node->children[i];
            if (child->count == kMaxKeys)
            {
                split_child(node, i, child);
                if (comp_(node->keys[i], key))
                {
                    ++i;
                }
            }
            return insert_non_full(node->children[i], key);
        }

        /**
         * @brief 从节点中删除键的内部实现。Internal erase implementation starting from node.
         *
         * @param node 当前节点 / current node.
         * @param key  要删除的键 / key to erase.
         * @return 若删除成功返回 true，否则 false。Returns true if erased, false otherwise.
         */
        bool erase_internal(Node *node, const key_type &key)
        {
            std::size_t idx = find_key_index(node, key);

            if (idx < node->count &&
                !comp_(key, node->keys[idx]) &&
                !comp_(node->keys[idx], key))
            {

                if (node->leaf)
                {
                    for (std::size_t i = idx + 1; i < node->count; ++i)
                    {
                        node->keys[i - 1] = node->keys[i];
                    }
                    node->count -= 1;
                    return true;
                }

                if (node->children[idx]->count > kMinKeys)
                {
                    key_type pred = get_predecessor(node, idx);
                    node->keys[idx] = pred;
                    return erase_internal(node->children[idx], pred);
                }

                if (node->children[idx + 1]->count > kMinKeys)
                {
                    key_type succ = get_successor(node, idx);
                    node->keys[idx] = succ;
                    return erase_internal(node->children[idx + 1], succ);
                }

                merge_children(node, idx);
                return erase_internal(node->children[idx], key);
            }

            if (node->leaf)
            {
                return false;
            }

            bool at_last_child = (idx == node->count);
            Node *child = node->children[idx];

            if (child->count == kMinKeys)
            {
                if (idx > 0 && node->children[idx - 1]->count > kMinKeys)
                {
                    borrow_from_prev(node, idx);
                }
                else if (idx < node->count && node->children[idx + 1]->count > kMinKeys)
                {
                    borrow_from_next(node, idx);
                }
                else
                {
                    if (idx < node->count)
                    {
                        merge_children(node, idx);
                    }
                    else
                    {
                        merge_children(node, idx - 1);
                        child = node->children[idx - 1];
                    }
                }
            }

            if (at_last_child && idx > node->count)
            {
                child = node->children[node->count];
            }

            return erase_internal(child, key);
        }

        /**
         * @brief 获取前驱键：左子树中最大键。Get predecessor key: maximum key in left subtree.
         * @param node 当前节点 / current node.
         * @param idx  键下标 / index of key.
         * @return 前驱键 / predecessor key.
         */
        static key_type get_predecessor(Node *node, std::size_t idx)
        {
            Node *cur = node->children[idx];
            while (!cur->leaf)
            {
                cur = cur->children[cur->count];
            }
            return cur->keys[cur->count - 1];
        }

        /**
         * @brief 获取后继键：右子树中最小键。Get successor key: minimum key in right subtree.
         * @param node 当前节点 / current node.
         * @param idx  键下标 / index of key.
         * @return 后继键 / successor key.
         */
        static key_type get_successor(Node *node, std::size_t idx)
        {
            Node *cur = node->children[idx + 1];
            while (!cur->leaf)
            {
                cur = cur->children[0];
            }
            return cur->keys[0];
        }

        /**
         * @brief 从左兄弟节点借一个键。Borrow one key from left sibling.
         * @param parent 父节点 / parent node.
         * @param idx    子节点下标 / index of child in parent.
         */
        static void borrow_from_prev(Node *parent, std::size_t idx)
        {
            Node *child = parent->children[idx];
            Node *sibling = parent->children[idx - 1];

            for (std::size_t i = child->count; i > 0; --i)
            {
                child->keys[i] = child->keys[i - 1];
            }

            if (!child->leaf)
            {
                for (std::size_t i = child->count + 1; i > 0; --i)
                {
                    child->children[i] = child->children[i - 1];
                }
            }

            child->keys[0] = parent->keys[idx - 1];

            if (!child->leaf)
            {
                child->children[0] = sibling->children[sibling->count];
            }

            parent->keys[idx - 1] = sibling->keys[sibling->count - 1];

            child->count += 1;
            sibling->count -= 1;
        }

        /**
         * @brief 从右兄弟节点借一个键。Borrow one key from right sibling.
         * @param parent 父节点 / parent node.
         * @param idx    子节点下标 / index of child in parent.
         */
        static void borrow_from_next(Node *parent, std::size_t idx)
        {
            Node *child = parent->children[idx];
            Node *sibling = parent->children[idx + 1];

            child->keys[child->count] = parent->keys[idx];

            if (!child->leaf)
            {
                child->children[child->count + 1] = sibling->children[0];
            }

            parent->keys[idx] = sibling->keys[0];

            for (std::size_t i = 1; i < sibling->count; ++i)
            {
                sibling->keys[i - 1] = sibling->keys[i];
            }

            if (!sibling->leaf)
            {
                for (std::size_t i = 1; i <= sibling->count; ++i)
                {
                    sibling->children[i - 1] = sibling->children[i];
                }
            }

            child->count += 1;
            sibling->count -= 1;
        }

        /**
         * @brief 合并两个相邻子节点。Merge two adjacent children.
         * @param parent 父节点 / parent node.
         * @param idx    左子节点下标，合并 idx 与 idx+1 / index of left child, merge child[idx] & child[idx+1].
         */
        static void merge_children(Node *parent, std::size_t idx)
        {
            Node *child = parent->children[idx];
            Node *sibling = parent->children[idx + 1];

            child->keys[kMinKeys] = parent->keys[idx];

            for (std::size_t i = 0; i < sibling->count; ++i)
            {
                child->keys[i + kMinKeys + 1] = sibling->keys[i];
            }

            if (!child->leaf)
            {
                for (std::size_t i = 0; i <= sibling->count; ++i)
                {
                    child->children[i + kMinKeys + 1] = sibling->children[i];
                }
            }

            child->count += sibling->count + 1;

            for (std::size_t i = idx + 1; i < parent->count; ++i)
            {
                parent->keys[i - 1] = parent->keys[i];
            }
            for (std::size_t i = idx + 2; i <= parent->count; ++i)
            {
                parent->children[i - 1] = parent->children[i];
            }

            parent->count -= 1;
            delete sibling;
        }

        /**
         * @brief 递归中序遍历实现。Recursive in-order traversal implementation.
         * @tparam Func 可调用对象类型 / callable type.
         * @param node 当前节点 / current node.
         * @param f    回调函数 / callback function.
         */
        template <typename Func>
        static void traverse_in_order_impl(Node *node, Func &&f)
        {
            if (!node)
            {
                return;
            }
            for (std::size_t i = 0; i < node->count; ++i)
            {
                if (!node->leaf)
                {
                    traverse_in_order_impl(node->children[i], f);
                }
                f(node->keys[i]);
            }
            if (!node->leaf)
            {
                traverse_in_order_impl(node->children[node->count], f);
            }
        }
    };

} // namespace test_forest

#endif
