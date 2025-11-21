#ifndef _BINARY_TREE_HPP
#define _BINARY_TREE_HPP

/**
 * @file Binary-Tree.hpp
 * @brief 二叉搜索树容器（模板）声明与实现 / Binary search tree container (template) declaration & implementation.
 */

#include <cstddef>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <utility>
#include <type_traits>
#include <functional>

namespace test_forest
{

    /**
     * @brief
     *  二叉搜索树容器，类似 std::set，按 Compare 顺序存储唯一元素。
     *  Binary search tree container, similar to std::set, storing unique elements ordered by Compare.
     *
     * @tparam T
     *  元素类型 / element type.
     * @tparam Compare
     *  比较器类型（缺省为 std::less<T>）/ comparator type (defaults to std::less<T>).
     * @tparam Allocator
     *  分配器类型（缺省为 std::allocator<T>）/ allocator type (defaults to std::allocator<T>).
     */
    template <class T,
              class Compare = std::less<T>,
              class Allocator = std::allocator<T>>
    class BinaryTree
    {
    private:
        // ============================
        // 内部结点结构 / Internal node
        // ============================
        struct node
        {
            /// @brief 存储的值 / stored value.
            T value;
            /// @brief 左子节点指针 / left child pointer.
            node *left;
            /// @brief 右子节点指针 / right child pointer.
            node *right;
            /// @brief 父节点指针 / parent pointer.
            node *parent;

            /// @brief 构造函数 / constructor.
            explicit node(const T &v, node *p = nullptr)
                : value(v), left(nullptr), right(nullptr), parent(p) {}

            /// @brief 移动构造函数 / move constructor.
            explicit node(T &&v, node *p = nullptr)
                : value(std::move(v)), left(nullptr), right(nullptr), parent(p) {}
        };

    public:
        // ============================
        // 公共类型别名 / Public type aliases
        // ============================
        /// @brief 值类型 / value type.
        using value_type = T;
        /// @brief 比较器类型 / comparator type.
        using compare_type = Compare;
        /// @brief 分配器类型 / allocator type.
        using allocator_type = Allocator;
        /// @brief 大小类型 / size type.
        using size_type = std::size_t;
        /// @brief 差值类型 / difference type.
        using difference_type = std::ptrdiff_t;
        /// @brief 引用类型 / reference type.
        using reference = value_type &;
        /// @brief 常量引用类型 / const reference type.
        using const_reference = const value_type &;

    private:
        /// @brief 结点分配器类型 / allocator type for node.
        using node_allocator_type =
            typename std::allocator_traits<Allocator>::template rebind_alloc<node>;

        /// @brief 结点分配器 traits / allocator traits for node.
        using node_alloc_traits = std::allocator_traits<node_allocator_type>;

    public:
        // ============================
        // 迭代器定义 / Iterator definition
        // ============================

        /**
         * @brief
         *  双向迭代器，按中序遍历访问元素。
         *  Bidirectional iterator visiting elements in in-order traversal.
         */
        class iterator
        {
        public:
            /// @brief 迭代器类别 / iterator category.
            using iterator_category = std::bidirectional_iterator_tag;
            /// @brief 值类型 / value type.
            using value_type = T;
            /// @brief 差值类型 / difference type.
            using difference_type = std::ptrdiff_t;
            /// @brief 指针类型 / pointer type.
            using pointer = T *;
            /// @brief 引用类型 / reference type.
            using reference = T &;

            /// @brief 默认构造，指向空 / default constructor, points to null.
            iterator() noexcept : current_(nullptr), owner_(nullptr) {}

            /// @brief 解引用 / dereference.
            reference operator*() const noexcept
            {
                return current_->value;
            }

            /// @brief 指针访问 / pointer access.
            pointer operator->() const noexcept
            {
                return &current_->value;
            }

            /// @brief 前置++，移动到下一个元素 / pre-increment, move to next element.
            iterator &operator++() noexcept
            {
                current_ = BinaryTree::next_node(current_);
                return *this;
            }

            /// @brief 后置++ / post-increment.
            iterator operator++(int) noexcept
            {
                iterator tmp(*this);
                ++(*this);
                return tmp;
            }

            /// @brief 前置--，移动到前一个元素；若当前为 end()，则跳到最大元素。
            ///        pre-decrement, move to previous element; if at end(), jumps to maximum element.
            iterator &operator--() noexcept
            {
                if (!owner_)
                {
                    return *this;
                }
                if (!current_)
                { // end() -> 最大元素 / end() -> max element
                    current_ = BinaryTree::maximum(owner_->root_);
                }
                else
                {
                    current_ = BinaryTree::prev_node(current_);
                }
                return *this;
            }

            /// @brief 后置-- / post-decrement.
            iterator operator--(int) noexcept
            {
                iterator tmp(*this);
                --(*this);
                return tmp;
            }

            /// @brief 相等比较 / equality comparison.
            friend bool operator==(const iterator &a, const iterator &b) noexcept
            {
                return a.current_ == b.current_;
            }

            /// @brief 不等比较 / inequality comparison.
            friend bool operator!=(const iterator &a, const iterator &b) noexcept
            {
                return !(a == b);
            }

        private:
            node *current_;
            const BinaryTree *owner_;

            explicit iterator(node *n, const BinaryTree *owner) noexcept
                : current_(n), owner_(owner) {}

            friend class BinaryTree;
            friend class const_iterator;
        };

        /**
         * @brief
         *  常量双向迭代器 / const bidirectional iterator.
         */
        class const_iterator
        {
        public:
            using iterator_category = std::bidirectional_iterator_tag;
            using value_type = T;
            using difference_type = std::ptrdiff_t;
            using pointer = const T *;
            using reference = const T &;

            const_iterator() noexcept : current_(nullptr), owner_(nullptr) {}

            /// @brief 从非常量迭代器构造 / construct from non-const iterator.
            const_iterator(const iterator &it) noexcept
                : current_(it.current_), owner_(it.owner_) {}

            reference operator*() const noexcept
            {
                return current_->value;
            }

            pointer operator->() const noexcept
            {
                return &current_->value;
            }

            const_iterator &operator++() noexcept
            {
                current_ = BinaryTree::next_node(current_);
                return *this;
            }

            const_iterator operator++(int) noexcept
            {
                const_iterator tmp(*this);
                ++(*this);
                return tmp;
            }

            const_iterator &operator--() noexcept
            {
                if (!owner_)
                {
                    return *this;
                }
                if (!current_)
                {
                    current_ = BinaryTree::maximum(owner_->root_);
                }
                else
                {
                    current_ = BinaryTree::prev_node(current_);
                }
                return *this;
            }

            const_iterator operator--(int) noexcept
            {
                const_iterator tmp(*this);
                --(*this);
                return tmp;
            }

            friend bool operator==(const const_iterator &a,
                                   const const_iterator &b) noexcept
            {
                return a.current_ == b.current_;
            }

            friend bool operator!=(const const_iterator &a,
                                   const const_iterator &b) noexcept
            {
                return !(a == b);
            }

        private:
            const node *current_;
            const BinaryTree *owner_;

            explicit const_iterator(const node *n, const BinaryTree *owner) noexcept
                : current_(n), owner_(owner) {}

            friend class BinaryTree;
        };

        // ============================
        // 构造 / 析构 / 赋值
        // Constructors / destructor / assignment
        // ============================

        /**
         * @brief
         *  默认构造，使用缺省比较器和分配器。
         *  Default constructor using default comparator and allocator.
         */
        BinaryTree()
            : root_(nullptr),
              size_(0),
              comp_(),
              alloc_(),
              node_alloc_(alloc_) {}

        /**
         * @brief
         *  使用指定比较器和分配器构造。
         *  Construct with specific comparator and allocator.
         */
        explicit BinaryTree(const Compare &comp,
                            const Allocator &alloc = Allocator())
            : root_(nullptr),
              size_(0),
              comp_(comp),
              alloc_(alloc),
              node_alloc_(alloc_) {}

        /**
         * @brief
         *  仅指定分配器构造，比较器使用缺省。
         *  Construct with allocator only, comparator is default constructed.
         */
        explicit BinaryTree(const Allocator &alloc)
            : root_(nullptr),
              size_(0),
              comp_(),
              alloc_(alloc),
              node_alloc_(alloc_) {}

        /**
         * @brief
         *  区间构造，从 [first, last) 插入所有元素。
         *  Range constructor, inserts all elements from [first, last).
         *
         * @tparam InputIt
         *  输入迭代器类型 / input iterator type.
         * @param first
         *  起始迭代器 / begin iterator.
         * @param last
         *  终止迭代器 / end iterator.
         * @param comp
         *  比较器 / comparator.
         * @param alloc
         *  分配器 / allocator.
         */
        template <class InputIt>
        BinaryTree(InputIt first, InputIt last,
                   const Compare &comp = Compare(),
                   const Allocator &alloc = Allocator())
            : BinaryTree(comp, alloc)
        {
            insert(first, last);
        }

        /**
         * @brief
         *  使用初始化列表构造。
         *  Construct from initializer list.
         */
        BinaryTree(std::initializer_list<T> init,
                   const Compare &comp = Compare(),
                   const Allocator &alloc = Allocator())
            : BinaryTree(comp, alloc)
        {
            insert(init);
        }

        /**
         * @brief
         *  拷贝构造函数，深拷贝整个树。
         *  Copy constructor, deep-copy the whole tree.
         */
        BinaryTree(const BinaryTree &other)
            : BinaryTree(other.comp_,
                         std::allocator_traits<Allocator>::
                             select_on_container_copy_construction(other.alloc_))
        {
            for (const auto &v : other)
            {
                insert(v);
            }
        }

        /**
         * @brief
         *  移动构造函数，接管另一棵树的资源。
         *  Move constructor, steal resources from another tree.
         */
        BinaryTree(BinaryTree &&other) noexcept
            : root_(other.root_),
              size_(other.size_),
              comp_(std::move(other.comp_)),
              alloc_(std::move(other.alloc_)),
              node_alloc_(std::move(other.node_alloc_))
        {
            other.root_ = nullptr;
            other.size_ = 0;
        }

        /**
         * @brief
         *  析构函数，释放所有结点。
         *  Destructor, releases all nodes.
         */
        ~BinaryTree()
        {
            clear();
        }

        /**
         * @brief
         *  拷贝赋值运算符。
         *  Copy assignment operator.
         */
        BinaryTree &operator=(const BinaryTree &other)
        {
            if (this == &other)
            {
                return *this;
            }

            if constexpr (std::allocator_traits<Allocator>::
                              propagate_on_container_copy_assignment::value)
            {
                if (alloc_ != other.alloc_)
                {
                    clear();
                }
                alloc_ = other.alloc_;
                node_alloc_ = node_allocator_type(alloc_);
            }
            comp_ = other.comp_;

            BinaryTree tmp(other, comp_, alloc_);
            swap(tmp);
            return *this;
        }

        /**
         * @brief
         *  移动赋值运算符。
         *  Move assignment operator.
         */
        BinaryTree &operator=(BinaryTree &&other) noexcept(
            std::allocator_traits<Allocator>::
                propagate_on_container_move_assignment::value ||
            std::allocator_traits<Allocator>::is_always_equal::value)
        {
            if (this == &other)
            {
                return *this;
            }

            clear();

            if constexpr (std::allocator_traits<Allocator>::
                              propagate_on_container_move_assignment::value)
            {
                alloc_ = std::move(other.alloc_);
                node_alloc_ = std::move(other.node_alloc_);
            }
            comp_ = std::move(other.comp_);
            root_ = other.root_;
            size_ = other.size_;

            other.root_ = nullptr;
            other.size_ = 0;
            return *this;
        }

        /**
         * @brief
         *  使用初始化列表赋值。
         *  Assign from initializer list.
         */
        BinaryTree &operator=(std::initializer_list<T> init)
        {
            BinaryTree tmp(init, comp_, alloc_);
            swap(tmp);
            return *this;
        }

        // ============================
        // 容量 / Capacity
        // ============================

        /**
         * @brief
         *  是否为空。
         *  Check if container is empty.
         */
        bool empty() const noexcept
        {
            return size_ == 0;
        }

        /**
         * @brief
         *  返回元素个数。
         *  Get number of elements.
         */
        size_type size() const noexcept
        {
            return size_;
        }

        /**
         * @brief
         *  清空整棵树。
         *  Clear the whole tree.
         */
        void clear() noexcept
        {
            destroy_subtree(root_);
            root_ = nullptr;
            size_ = 0;
        }

        // ============================
        // 迭代器 / Iterators
        // ============================

        /// @brief 返回指向第一个元素的迭代器 / return iterator to first element.
        iterator begin() noexcept
        {
            return iterator(minimum(root_), this);
        }

        /// @brief 返回指向第一个元素的常量迭代器 / return const iterator to first element.
        const_iterator begin() const noexcept
        {
            return const_iterator(minimum(root_), this);
        }

        /// @brief 返回指向第一个元素的常量迭代器 / return const iterator to first element.
        const_iterator cbegin() const noexcept
        {
            return begin();
        }

        /// @brief 返回尾后迭代器 / return iterator past the last element.
        iterator end() noexcept
        {
            return iterator(nullptr, this);
        }

        /// @brief 返回尾后常量迭代器 / return const iterator past the last element.
        const_iterator end() const noexcept
        {
            return const_iterator(nullptr, this);
        }

        /// @brief 返回尾后常量迭代器 / return const iterator past the last element.
        const_iterator cend() const noexcept
        {
            return end();
        }

        // ============================
        // 修改器 / Modifiers
        // ============================

        /**
         * @brief
         *  插入一个值（拷贝），若已存在则不插入。
         *  Insert a value (copy). If already exists, no insertion.
         *
         * @param value
         *  要插入的值 / value to insert.
         * @return
         *  pair(迭代器, 是否插入成功) / pair(iterator, bool inserted).
         */
        std::pair<iterator, bool> insert(const T &value)
        {
            return emplace_internal(value);
        }

        /**
         * @brief
         *  插入一个值（移动），若已存在则不插入。
         *  Insert a value (move). If already exists, no insertion.
         */
        std::pair<iterator, bool> insert(T &&value)
        {
            return emplace_internal(std::move(value));
        }

        /**
         * @brief
         *  插入区间 [first, last) 的所有元素。
         *  Insert all elements from range [first, last).
         */
        template <class InputIt>
        void insert(InputIt first, InputIt last)
        {
            for (; first != last; ++first)
            {
                insert(*first);
            }
        }

        /**
         * @brief
         *  插入初始化列表中所有元素。
         *  Insert all elements from initializer list.
         */
        void insert(std::initializer_list<T> init)
        {
            insert(init.begin(), init.end());
        }

        /**
         * @brief
         *  删除指定位置的元素，返回指向后一个元素的迭代器。
         *  Erase element at given position, returns iterator to next element.
         */
        iterator erase(iterator pos)
        {
            if (pos == end())
            {
                return end();
            }
            node *n = pos.current_;
            iterator next_it = ++pos;
            erase_node(n);
            return next_it;
        }

        /**
         * @brief
         *  删除指定位置的元素（const 版本）。
         *  Erase element at given position (const version).
         */
        iterator erase(const_iterator pos)
        {
            if (pos == cend())
            {
                return end();
            }
            node *n = const_cast<node *>(pos.current_);
            iterator it(n, this);
            return erase(it);
        }

        /**
         * @brief
         *  删除所有等于 key 的元素（此容器中最多 1 个）。
         *  Erase element equal to key (at most 1 in this container).
         *
         * @param key
         *  要删除的键 / key to erase.
         * @return
         *  删除的元素个数（0 或 1）/ number of erased elements (0 or 1).
         */
        size_type erase(const T &key)
        {
            node *n = find_node(key);
            if (!n)
            {
                return 0;
            }
            erase_node(n);
            return 1;
        }

        /**
         * @brief
         *  与另一棵树交换内容。
         *  Swap contents with another tree.
         */
        void swap(BinaryTree &other) noexcept(
            node_alloc_traits::is_always_equal::value &&
            std::is_nothrow_swappable<Compare>::value)
        {
            using std::swap;
            swap(root_, other.root_);
            swap(size_, other.size_);
            swap(comp_, other.comp_);
            if constexpr (node_alloc_traits::propagate_on_container_swap::value)
            {
                swap(node_alloc_, other.node_alloc_);
                swap(alloc_, other.alloc_);
            }
        }

        // ============================
        // 查找 / Lookup
        // ============================

        /**
         * @brief
         *  查找等于 key 的元素。
         *  Find element equal to key.
         */
        iterator find(const T &key) noexcept
        {
            return iterator(find_node(key), this);
        }

        /**
         * @brief
         *  查找等于 key 的元素（常量版本）。
         *  Find element equal to key (const version).
         */
        const_iterator find(const T &key) const noexcept
        {
            return const_iterator(find_node(key), this);
        }

        /**
         * @brief
         *  是否包含等于 key 的元素。
         *  Check if container contains an element equal to key.
         */
        bool contains(const T &key) const noexcept
        {
            return find_node(key) != nullptr;
        }

        // ============================
        // 访问器 / Observers
        // ============================

        /**
         * @brief
         *  返回比较器对象。
         *  Get comparator object.
         */
        compare_type value_comp() const
        {
            return comp_;
        }

        /**
         * @brief
         *  返回分配器。
         *  Get allocator.
         */
        allocator_type get_allocator() const noexcept
        {
            return alloc_;
        }

    private:
        // ============================
        // 成员变量 / Data members
        // ============================
        /// @brief 根结点指针 / root node pointer.
        node *root_;
        /// @brief 元素个数 / number of elements.
        size_type size_;
        /// @brief 比较器 / comparator.
        compare_type comp_;
        /// @brief 元素分配器 / allocator for values.
        allocator_type alloc_;
        /// @brief 结点分配器 / allocator for nodes.
        node_allocator_type node_alloc_;

        // ============================
        // 结点工具函数 / Node helpers
        // ============================

        /**
         * @brief
         *  在子树中找到最小结点。
         *  Find minimum node in subtree.
         */
        static node *minimum(node *n) noexcept
        {
            if (!n)
                return nullptr;
            while (n->left)
            {
                n = n->left;
            }
            return n;
        }

        /**
         * @brief
         *  在子树中找到最大结点。
         *  Find maximum node in subtree.
         */
        static node *maximum(node *n) noexcept
        {
            if (!n)
                return nullptr;
            while (n->right)
            {
                n = n->right;
            }
            return n;
        }

        /**
         * @brief
         *  找到中序遍历意义下当前结点的后继。
         *  Find in-order successor of current node.
         */
        static node *next_node(node *n) noexcept
        {
            if (!n)
                return nullptr;
            if (n->right)
            {
                return minimum(n->right);
            }
            node *p = n->parent;
            while (p && n == p->right)
            {
                n = p;
                p = p->parent;
            }
            return p;
        }

        /**
         * @brief
         *  找到中序遍历意义下当前结点的前驱。
         *  Find in-order predecessor of current node.
         */
        static node *prev_node(node *n) noexcept
        {
            if (!n)
                return nullptr;
            if (n->left)
            {
                return maximum(n->left);
            }
            node *p = n->parent;
            while (p && n == p->left)
            {
                n = p;
                p = p->parent;
            }
            return p;
        }

        /**
         * @brief
         *  销毁子树 [递归]。
         *  Destroy subtree (recursive).
         */
        void destroy_subtree(node *n) noexcept
        {
            if (!n)
                return;
            destroy_subtree(n->left);
            destroy_subtree(n->right);
            destroy_node(n);
        }

        /**
         * @brief
         *  分配并构造一个结点（拷贝）。
         *  Allocate and construct a node (copy).
         */
        node *create_node(const T &value, node *parent)
        {
            node *n = node_alloc_traits::allocate(node_alloc_, 1);
            try
            {
                node_alloc_traits::construct(node_alloc_, n, value, parent);
            }
            catch (...)
            {
                node_alloc_traits::deallocate(node_alloc_, n, 1);
                throw;
            }
            return n;
        }

        /**
         * @brief
         *  分配并构造一个结点（移动）。
         *  Allocate and construct a node (move).
         */
        node *create_node(T &&value, node *parent)
        {
            node *n = node_alloc_traits::allocate(node_alloc_, 1);
            try
            {
                node_alloc_traits::construct(node_alloc_, n, std::move(value), parent);
            }
            catch (...)
            {
                node_alloc_traits::deallocate(node_alloc_, n, 1);
                throw;
            }
            return n;
        }

        /**
         * @brief
         *  销毁并释放结点。
         *  Destroy and deallocate node.
         */
        void destroy_node(node *n) noexcept
        {
            if (!n)
                return;
            node_alloc_traits::destroy(node_alloc_, n);
            node_alloc_traits::deallocate(node_alloc_, n, 1);
        }

        /**
         * @brief
         *  在树中查找等于 key 的结点。
         *  Find node equal to key in the tree.
         */
        node *find_node(const T &key) const noexcept
        {
            node *cur = root_;
            while (cur)
            {
                if (comp_(key, cur->value))
                {
                    cur = cur->left;
                }
                else if (comp_(cur->value, key))
                {
                    cur = cur->right;
                }
                else
                {
                    return cur;
                }
            }
            return nullptr;
        }

        /**
         * @brief
         *  将以 v 为根的子树替换到 u 的位置（不修改 v 的左右子树）。
         *  Transplant subtree rooted at v to replace u (without touching v's children).
         *
         * @param u
         *  被替换的结点 / node being replaced.
         * @param v
         *  替换上的结点 / node to replace with (can be nullptr).
         */
        void transplant(node *u, node *v) noexcept
        {
            if (!u->parent)
            {
                root_ = v;
            }
            else if (u == u->parent->left)
            {
                u->parent->left = v;
            }
            else
            {
                u->parent->right = v;
            }
            if (v)
            {
                v->parent = u->parent;
            }
        }

        /**
         * @brief
         *  删除指定结点（标准 BST 删除算法）。
         *  Erase given node (standard BST deletion algorithm).
         */
        void erase_node(node *z) noexcept
        {
            if (!z)
                return;

            if (!z->left)
            {
                transplant(z, z->right);
            }
            else if (!z->right)
            {
                transplant(z, z->left);
            }
            else
            {
                node *y = minimum(z->right);
                if (y->parent != z)
                {
                    transplant(y, y->right);
                    y->right = z->right;
                    if (y->right)
                    {
                        y->right->parent = y;
                    }
                }
                transplant(z, y);
                y->left = z->left;
                if (y->left)
                {
                    y->left->parent = y;
                }
            }
            destroy_node(z);
            --size_;
        }

        /**
         * @brief
         *  内部 emplace 实现，统一处理左/右插入和重复元素检查。
         *  Internal emplace implementation, handling left/right insertion and duplicate check.
         */
        template <class U>
        std::pair<iterator, bool> emplace_internal(U &&value)
        {
            if (!root_)
            {
                node *n = create_node(std::forward<U>(value), nullptr);
                root_ = n;
                ++size_;
                return {iterator(n, this), true};
            }

            node *parent = nullptr;
            node *cur = root_;
            while (cur)
            {
                parent = cur;
                if (comp_(value, cur->value))
                {
                    cur = cur->left;
                }
                else if (comp_(cur->value, value))
                {
                    cur = cur->right;
                }
                else
                {
                    // 已存在 / already exists
                    return {iterator(cur, this), false};
                }
            }

            node *n = create_node(std::forward<U>(value), parent);
            if (comp_(n->value, parent->value))
            {
                parent->left = n;
            }
            else
            {
                parent->right = n;
            }
            ++size_;
            return {iterator(n, this), true};
        }

        /**
         * @brief
         *  供拷贝赋值内部使用的辅助构造函数。
         *  Helper constructor for copy-assignment.
         */
        BinaryTree(const BinaryTree &other,
                   const Compare &comp,
                   const Allocator &alloc)
            : BinaryTree(comp, alloc)
        {
            for (const auto &v : other)
            {
                insert(v);
            }
        }
    };

} // namespace test_forest

#endif
