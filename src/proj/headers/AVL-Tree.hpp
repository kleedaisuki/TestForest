#ifndef _AVL_TREE_HPP
#define _AVL_TREE_HPP

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <functional>
#include <iterator>
#include <memory>

namespace tf
{

    /**
     * @brief AVL树容器（类似 std::set 的有序唯一键集合）
     *        AVL tree container (set-like ordered unique-key container)
     *
     * @tparam T         键/值类型 / key & value type
     * @tparam Compare   比较器(less) / comparator (less)
     * @tparam Allocator 分配器(allocator) / allocator
     *
     * @note
     *  - 内部使用 AVL 树(AVL tree) 维护平衡，高度为 O(log N)
     *  - 使用 header 哨兵节点(sentinel) 模式，end() 指向 header
     *  - 迭代器为双向迭代器(bidirectional iterator)，中序遍历(in-order)
     */
    template <class T, class Compare = std::less<T>, class Allocator = std::allocator<T>>
    class avl_tree
    {
    private:
        // ===================== 节点定义 / Node definition =====================

        /**
         * @brief AVL 树节点结构 / AVL tree node structure
         *
         * @note
         *  - 存储值、父指针、左右子指针以及高度
         *  - 仅用于 avl_tree 内部实现 / internal-only for avl_tree
         */
        struct node
        {
            /// @brief 节点存储的值 / stored value
            T value;

            /// @brief 父节点指针 / parent pointer
            node *parent;

            /// @brief 左子节点指针 / left child pointer
            node *left;

            /// @brief 右子节点指针 / right child pointer
            node *right;

            /// @brief 节点高度 / node height
            int height;

            /**
             * @brief 以左值构造节点 / construct node from lvalue
             *
             * @param v 值 / value
             */
            explicit node(const T &v)
                : value(v), parent(nullptr), left(nullptr), right(nullptr), height(1)
            {
            }

            /**
             * @brief 以右值构造节点 / construct node from rvalue
             *
             * @param v 值 / value
             */
            explicit node(T &&v)
                : value(std::move(v)), parent(nullptr), left(nullptr), right(nullptr), height(1)
            {
            }
        };

        // 为 allocator 重新绑定节点类型 / rebind allocator for node
        using node_allocator_raw = typename std::allocator_traits<Allocator>::template rebind_alloc<node>;
        using node_traits = std::allocator_traits<node_allocator_raw>;

        // ===================== 迭代器实现 / Iterator implementation =====================

        /**
         * @brief AVL 树迭代器实现（双向，中序遍历）
         *        AVL tree iterator implementation (bidirectional, in-order)
         *
         * @note
         *  - 仅作为 avl_tree 的内部实现类型 / internal impl type
         *  - 对外通过 using iterator = iterator_impl 暴露 / aliased as iterator
         */
        class iterator_impl
        {
        public:
            using value_type = T;
            using reference = T &;
            using pointer = T *;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::bidirectional_iterator_tag;

            /// @brief 默认构造空迭代器 / default-construct null iterator
            iterator_impl() noexcept
                : node_(nullptr)
            {
            }

            /// @brief 解引用访问元素 / dereference to access value
            reference operator*() const noexcept
            {
                return node_->value;
            }

            /// @brief 成员访问 / member access operator
            pointer operator->() const noexcept
            {
                return &node_->value;
            }

            /// @brief 前置自增，移动到中序遍历的下一个元素 / pre-increment to next in-order element
            iterator_impl &operator++() noexcept
            {
                increment();
                return *this;
            }

            /// @brief 后置自增 / post-increment
            iterator_impl operator++(int) noexcept
            {
                iterator_impl tmp(*this);
                increment();
                return tmp;
            }

            /// @brief 前置自减，移动到中序遍历的上一个元素 / pre-decrement to previous in-order element
            iterator_impl &operator--() noexcept
            {
                decrement();
                return *this;
            }

            /// @brief 后置自减 / post-decrement
            iterator_impl operator--(int) noexcept
            {
                iterator_impl tmp(*this);
                decrement();
                return tmp;
            }

            /// @brief 比较相等 / equality
            friend bool operator==(const iterator_impl &a, const iterator_impl &b) noexcept
            {
                return a.node_ == b.node_;
            }

            /// @brief 比较不相等 / inequality
            friend bool operator!=(const iterator_impl &a, const iterator_impl &b) noexcept
            {
                return a.node_ != b.node_;
            }

        private:
            /// @brief 当前节点指针 / underlying node pointer
            node *node_;

            /// @brief 允许容器访问内部节点指针 / allow avl_tree to access node_
            friend class avl_tree;

            /// @brief 通过节点构造迭代器 / construct from node pointer
            explicit iterator_impl(node *n) noexcept
                : node_(n)
            {
            }

            /**
             * @brief 中序 ++ / in-order ++
             *
             * @note
             *  - 不对 end()++ 做防御 / no guard for ++ on end()
             */
            void increment() noexcept
            {
                if (!node_)
                    return;

                if (node_->right)
                {
                    node_ = node_->right;
                    while (node_->left)
                        node_ = node_->left;
                    return;
                }

                auto *p = node_->parent;
                while (p && node_ == p->right)
                {
                    node_ = p;
                    p = p->parent;
                }
                node_ = p;
            }

            /**
             * @brief 中序 -- / in-order --
             *
             * @note
             *  - 对 --end() 未做 header 特殊处理，行为未定义
             *  - STL 中 --end() 通常是定义良好的，这里实现略为简化
             */
            void decrement() noexcept
            {
                if (!node_)
                    return;

                if (node_->left)
                {
                    node_ = node_->left;
                    while (node_->right)
                        node_ = node_->right;
                    return;
                }

                auto *p = node_->parent;
                while (p && node_ == p->left)
                {
                    node_ = p;
                    p = p->parent;
                }
                node_ = p;
            }
        };

        /**
         * @brief const 版本的 AVL 树迭代器实现 / const version of AVL tree iterator
         *
         * @note
         *  - 可由非 const iterator 隐式构造 / constructible from non-const iterator
         */
        class const_iterator_impl
        {
        public:
            using value_type = T;
            using reference = const T &;
            using pointer = const T *;
            using difference_type = std::ptrdiff_t;
            using iterator_category = std::bidirectional_iterator_tag;

            /// @brief 默认构造空迭代器 / default-construct null iterator
            const_iterator_impl() noexcept
                : node_(nullptr)
            {
            }

            /// @brief 从节点指针构造 / construct from node pointer
            explicit const_iterator_impl(const node *n) noexcept
                : node_(n)
            {
            }

            /// @brief 从非 const 迭代器构造 / construct from non-const iterator
            const_iterator_impl(const iterator_impl &it) noexcept
                : node_(it.node_)
            {
            }

            /// @brief 解引用访问元素 / dereference
            reference operator*() const noexcept
            {
                return node_->value;
            }

            /// @brief 成员访问 / member access
            pointer operator->() const noexcept
            {
                return &node_->value;
            }

            /// @brief 前置自增 / pre-increment
            const_iterator_impl &operator++() noexcept
            {
                increment();
                return *this;
            }

            /// @brief 后置自增 / post-increment
            const_iterator_impl operator++(int) noexcept
            {
                const_iterator_impl tmp(*this);
                increment();
                return tmp;
            }

            /// @brief 前置自减 / pre-decrement
            const_iterator_impl &operator--() noexcept
            {
                decrement();
                return *this;
            }

            /// @brief 后置自减 / post-decrement
            const_iterator_impl operator--(int) noexcept
            {
                const_iterator_impl tmp(*this);
                decrement();
                return tmp;
            }

            /// @brief 比较相等 / equality
            friend bool operator==(const const_iterator_impl &a, const const_iterator_impl &b) noexcept
            {
                return a.node_ == b.node_;
            }

            /// @brief 比较不相等 / inequality
            friend bool operator!=(const const_iterator_impl &a, const const_iterator_impl &b) noexcept
            {
                return a.node_ != b.node_;
            }

        private:
            /// @brief 当前节点指针 / underlying node pointer
            const node *node_;

            /// @brief 允许容器访问内部节点指针 / allow avl_tree to access node_
            friend class avl_tree;

            void increment() noexcept
            {
                if (!node_)
                    return;

                if (node_->right)
                {
                    node_ = node_->right;
                    while (node_->left)
                        node_ = node_->left;
                    return;
                }

                auto *p = node_->parent;
                while (p && node_ == p->right)
                {
                    node_ = p;
                    p = p->parent;
                }
                node_ = p;
            }

            void decrement() noexcept
            {
                if (!node_)
                    return;

                if (node_->left)
                {
                    node_ = node_->left;
                    while (node_->right)
                        node_ = node_->right;
                    return;
                }

                auto *p = node_->parent;
                while (p && node_ == p->left)
                {
                    node_ = p;
                    p = p->parent;
                }
                node_ = p;
            }
        };

    public:
        // ===================== 公共类型别名 / Public type aliases =====================

        using key_type = T;
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using key_compare = Compare;
        using value_compare = Compare;
        using allocator_type = Allocator;
        using reference = value_type &;
        using const_reference = const value_type &;
        using pointer = typename node_traits::pointer;
        using const_pointer = typename node_traits::const_pointer;
        using iterator = iterator_impl;
        using const_iterator = const_iterator_impl;
        using reverse_iterator = std::reverse_iterator<iterator>;
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;

        // ===================== 构造/析构 / Constructors & destructor =====================

        /**
         * @brief 默认构造空 AVL 树 / construct empty AVL tree
         */
        avl_tree()
            : avl_tree(key_compare(), allocator_type())
        {
        }

        /**
         * @brief 使用比较器和分配器构造 / construct with comparator and allocator
         *
         * @param comp  比较器 / comparator
         * @param alloc 分配器 / allocator
         */
        explicit avl_tree(const key_compare &comp, const allocator_type &alloc = allocator_type())
            : header_(nullptr), size_(0), comp_(comp), alloc_(alloc)
        {
            init_header();
        }

        /**
         * @brief 仅指定分配器构造 / construct with allocator only
         *
         * @param alloc 分配器 / allocator
         */
        explicit avl_tree(const allocator_type &alloc)
            : header_(nullptr), size_(0), comp_(key_compare()), alloc_(alloc)
        {
            init_header();
        }

        /**
         * @brief 拷贝构造 / copy constructor
         *
         * @param other 另一个 AVL 树 / another AVL tree
         */
        avl_tree(const avl_tree &other)
            : avl_tree(other.comp_, other.get_allocator())
        {
            for (const auto &v : other)
            {
                insert(v);
            }
        }

        /**
         * @brief 移动构造 / move constructor
         *
         * @param other 被移动的 AVL 树 / AVL tree to move from
         */
        avl_tree(avl_tree &&other) noexcept
            : header_(other.header_), size_(other.size_), comp_(std::move(other.comp_)), alloc_(std::move(other.alloc_))
        {
            other.header_ = nullptr;
            other.size_ = 0;
        }

        /**
         * @brief 拷贝赋值 / copy assignment
         *
         * @param other 另一个 AVL 树 / another AVL tree
         * @return *this
         */
        avl_tree &operator=(const avl_tree &other)
        {
            if (this == &other)
                return *this;

            avl_tree tmp(other);
            swap(tmp);
            return *this;
        }

        /**
         * @brief 移动赋值 / move assignment
         *
         * @param other 被移动的 AVL 树 / AVL tree to move from
         * @return *this
         */
        avl_tree &operator=(avl_tree &&other) noexcept
        {
            if (this == &other)
                return *this;

            clear();
            destroy_header();

            header_ = other.header_;
            size_ = other.size_;
            comp_ = std::move(other.comp_);
            alloc_ = std::move(other.alloc_);

            other.header_ = nullptr;
            other.size_ = 0;

            return *this;
        }

        /**
         * @brief 析构函数 / destructor
         */
        ~avl_tree()
        {
            clear();
            destroy_header();
        }

        // ===================== 基本属性 / Basic properties =====================

        /**
         * @brief 获取分配器 / get allocator
         *
         * @return 分配器对象 / allocator object
         */
        allocator_type get_allocator() const noexcept
        {
            return allocator_type(alloc_);
        }

        /**
         * @brief 判断是否为空 / check if container is empty
         *
         * @return 若无元素则为 true / true if no elements
         */
        bool empty() const noexcept
        {
            return size_ == 0;
        }

        /**
         * @brief 返回元素个数 / get number of elements
         *
         * @return 元素数量 / size
         */
        size_type size() const noexcept
        {
            return size_;
        }

        /**
         * @brief 清空所有元素 / clear all elements
         *
         * @note
         *  - 保留比较器和分配器、header 结构 / comparator & allocator & header stay
         */
        void clear() noexcept
        {
            if (!header_)
                return;

            destroy_subtree(root());
            set_root(nullptr);
            header_->left = header_;
            header_->right = header_;
            size_ = 0;
        }

        // ===================== 插入 / Insertion =====================

        /**
         * @brief 插入一个元素（左值）/ insert a value (lvalue)
         *
         * @param value 要插入的值 / value to insert
         * @return pair(迭代器, 是否插入成功)
         *         pair(iterator, inserted ?)
         */
        std::pair<iterator, bool> insert(const value_type &value)
        {
            return insert_impl(value);
        }

        /**
         * @brief 插入一个元素（右值）/ insert a value (rvalue)
         *
         * @param value 要插入的值 / value to insert
         * @return pair(迭代器, 是否插入成功)
         *         pair(iterator, inserted ?)
         */
        std::pair<iterator, bool> insert(value_type &&value)
        {
            return insert_impl(std::move(value));
        }

        // ===================== 删除 / Erase =====================

        /**
         * @brief 根据键删除元素 / erase element by key
         *
         * @param key 要删除的键 / key to erase
         * @return 删除的数量(0 或 1) / count of erased elements (0 or 1)
         */
        size_type erase(const key_type &key)
        {
            auto it = find(key);
            if (it == end())
                return 0;
            erase(it);
            return 1;
        }

        /**
         * @brief 删除指定位置的元素 / erase element at iterator
         *
         * @param pos 迭代器位置 / iterator to element
         * @return 删除元素后的位置 / iterator following the erased element
         */
        iterator erase(iterator pos)
        {
            node *n = pos.node_;
            assert(n && "erase on end() is undefined");

            iterator next = pos;
            ++next;

            erase_node(n);

            return next;
        }

        // ===================== 迭代器接口 / Iterator interface =====================

        /**
         * @brief 返回指向首元素的迭代器 / iterator to first element
         *
         * @return 若空则返回 end() / end() if empty
         */
        iterator begin() noexcept
        {
            return iterator(size_ == 0 ? header_ : header_->left);
        }

        /**
         * @brief 返回 const 版本的首元素迭代器 / const iterator to first element
         */
        const_iterator begin() const noexcept
        {
            return const_iterator(size_ == 0 ? header_ : header_->left);
        }

        /**
         * @brief 返回 const 版本的首元素迭代器 / const iterator to first element
         */
        const_iterator cbegin() const noexcept
        {
            return begin();
        }

        /**
         * @brief 返回尾后迭代器 / iterator to one past the last element
         *
         * @note
         *  - end() 指向 header 哨兵节点(sentinel) / end() points to header sentinel
         */
        iterator end() noexcept
        {
            return iterator(header_);
        }

        /**
         * @brief 返回 const 版本的尾后迭代器 / const iterator to end
         */
        const_iterator end() const noexcept
        {
            return const_iterator(header_);
        }

        /**
         * @brief 返回 const 版本的尾后迭代器 / const iterator to end
         */
        const_iterator cend() const noexcept
        {
            return end();
        }

        /**
         * @brief 反向迭代器起点 / reverse begin
         */
        reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }

        /**
         * @brief const 反向迭代器起点 / const reverse begin
         */
        const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(end());
        }

        const_reverse_iterator crbegin() const noexcept
        {
            return rbegin();
        }

        /**
         * @brief 反向迭代器终点 / reverse end
         */
        reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }

        /**
         * @brief const 反向迭代器终点 / const reverse end
         */
        const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(begin());
        }

        const_reverse_iterator crend() const noexcept
        {
            return rend();
        }

        // ===================== 查找与范围 / Lookup & range =====================

        /**
         * @brief 按键查找元素 / find element by key
         *
         * @param key 要查找的键 / key to search
         * @return 指向找到元素的迭代器，若无则为 end()
         *         iterator to found element or end() if not found
         */
        iterator find(const key_type &key) noexcept
        {
            node *cur = root();
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
                    return iterator(cur);
                }
            }
            return end();
        }

        /**
         * @brief const 版本按键查找 / const find by key
         */
        const_iterator find(const key_type &key) const noexcept
        {
            const node *cur = root();
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
                    return const_iterator(cur);
                }
            }
            return end();
        }

        /**
         * @brief 统计某键出现次数（0或1）/ count occurrences of key (0 or 1)
         *
         * @param key 键值 / key
         * @return 0 或 1 / 0 or 1
         */
        size_type count(const key_type &key) const noexcept
        {
            return find(key) == end() ? 0 : 1;
        }

        /**
         * @brief lower_bound：第一个不小于 key 的元素 / first element not less than key
         *
         * @param key 键 / key
         * @return 迭代器 / iterator
         */
        iterator lower_bound(const key_type &key) noexcept
        {
            node *cur = root();
            node *result = nullptr;

            while (cur)
            {
                if (!comp_(cur->value, key))
                {
                    result = cur;
                    cur = cur->left;
                }
                else
                {
                    cur = cur->right;
                }
            }
            return iterator(result ? result : header_);
        }

        /**
         * @brief const 版本 lower_bound / const lower_bound
         */
        const_iterator lower_bound(const key_type &key) const noexcept
        {
            const node *cur = root();
            const node *result = nullptr;

            while (cur)
            {
                if (!comp_(cur->value, key))
                {
                    result = cur;
                    cur = cur->left;
                }
                else
                {
                    cur = cur->right;
                }
            }
            return const_iterator(result ? result : header_);
        }

        /**
         * @brief upper_bound：第一个大于 key 的元素 / first element greater than key
         *
         * @param key 键 / key
         * @return 迭代器 / iterator
         */
        iterator upper_bound(const key_type &key) noexcept
        {
            node *cur = root();
            node *result = nullptr;

            while (cur)
            {
                if (comp_(key, cur->value))
                {
                    result = cur;
                    cur = cur->left;
                }
                else
                {
                    cur = cur->right;
                }
            }
            return iterator(result ? result : header_);
        }

        /**
         * @brief const 版本 upper_bound / const upper_bound
         */
        const_iterator upper_bound(const key_type &key) const noexcept
        {
            const node *cur = root();
            const node *result = nullptr;

            while (cur)
            {
                if (comp_(key, cur->value))
                {
                    result = cur;
                    cur = cur->left;
                }
                else
                {
                    cur = cur->right;
                }
            }
            return const_iterator(result ? result : header_);
        }

        /**
         * @brief equal_range：返回 [lower_bound, upper_bound) 区间
         *        equal_range: [lower_bound, upper_bound)
         */
        std::pair<iterator, iterator> equal_range(const key_type &key) noexcept
        {
            return {lower_bound(key), upper_bound(key)};
        }

        /**
         * @brief const 版本 equal_range / const equal_range
         */
        std::pair<const_iterator, const_iterator> equal_range(const key_type &key) const noexcept
        {
            return {lower_bound(key), upper_bound(key)};
        }

        /**
         * @brief 获取 key 比较器 / get key comparator
         */
        key_compare key_comp() const
        {
            return comp_;
        }

        /**
         * @brief 获取 value 比较器（同 key_compare）/ get value comparator
         */
        value_compare value_comp() const
        {
            return comp_;
        }

        // ===================== 其它辅助接口 / Other helpers =====================

        /**
         * @brief 交换两棵树的内容 / swap contents of two trees
         *
         * @param other 另一棵树 / other tree
         */
        void swap(avl_tree &other) noexcept
        {
            using std::swap;
            swap(header_, other.header_);
            swap(size_, other.size_);
            swap(comp_, other.comp_);
            swap(alloc_, other.alloc_);
        }

    private:
        // ===================== 内部状态 / Internal state =====================

        /// @brief header 哨兵节点指针 / header sentinel node pointer
        node *header_;

        /// @brief 元素数量 / number of elements
        size_type size_;

        /// @brief 键比较器 / key comparator
        key_compare comp_;

        /// @brief 节点分配器 / node allocator
        node_allocator_raw alloc_;

        // ===================== 内部工具函数 / Internal helpers =====================

        /**
         * @brief 获取根节点指针 / get root node pointer
         */
        node *root() const noexcept
        {
            return header_ ? header_->parent : nullptr;
        }

        /**
         * @brief 设置根节点 / set root node
         *
         * @param r 新的根节点指针 / new root pointer
         */
        void set_root(node *r) noexcept
        {
            header_->parent = r;
            if (r)
                r->parent = header_;
        }

        /**
         * @brief 获取节点高度 / get node height
         *
         * @param n 节点指针 / node pointer
         * @return 节点高度，空节点为 0 / height or 0 if null
         */
        int height(node *n) const noexcept
        {
            return n ? n->height : 0;
        }

        /**
         * @brief 获取节点的平衡因子 / get node balance factor
         *
         * @param n 节点指针 / node pointer
         * @return 左高为正，右高为负 / positive if left-heavy, negative if right-heavy
         */
        int balance_factor(node *n) const noexcept
        {
            return n ? (height(n->left) - height(n->right)) : 0;
        }

        /**
         * @brief 更新节点高度 / update node height
         *
         * @param n 节点指针 / node pointer
         */
        void update_height(node *n) noexcept
        {
            if (!n)
                return;
            int hl = height(n->left);
            int hr = height(n->right);
            n->height = (hl > hr ? hl : hr) + 1;
        }

        /**
         * @brief 返回以 n 为根的最小节点 / minimum node in subtree
         *
         * @param n 子树根节点 / subtree root
         * @return 最小节点 / minimum node
         */
        node *min_node(node *n) const noexcept
        {
            if (!n)
                return nullptr;
            while (n->left)
                n = n->left;
            return n;
        }

        /**
         * @brief 返回以 n 为根的最大节点 / maximum node in subtree
         */
        node *max_node(node *n) const noexcept
        {
            if (!n)
                return nullptr;
            while (n->right)
                n = n->right;
            return n;
        }

        /**
         * @brief 初始化 header 哨兵节点 / initialize header sentinel node
         */
        void init_header()
        {
            header_ = node_traits::allocate(alloc_, 1);
            header_->parent = nullptr;
            header_->left = header_;
            header_->right = header_;
            header_->height = 0;
        }

        /**
         * @brief 销毁 header 哨兵节点 / destroy header sentinel node
         */
        void destroy_header()
        {
            if (header_)
            {
                node_traits::deallocate(alloc_, header_, 1);
                header_ = nullptr;
            }
        }

        /**
         * @brief 创建新节点 / create a new node
         *
         * @tparam U 值类型 / value type
         * @param v 值 / value
         * @return 新节点指针 / new node pointer
         */
        template <class U>
        node *create_node(U &&v)
        {
            node *n = node_traits::allocate(alloc_, 1);
            try
            {
                ::new (static_cast<void *>(n)) node(std::forward<U>(v));
            }
            catch (...)
            {
                node_traits::deallocate(alloc_, n, 1);
                throw;
            }
            return n;
        }

        /**
         * @brief 销毁节点 / destroy a node
         *
         * @param n 节点指针 / node pointer
         */
        void destroy_node(node *n) noexcept
        {
            if (!n)
                return;
            n->~node();
            node_traits::deallocate(alloc_, n, 1);
        }

        /**
         * @brief 递归销毁子树 / destroy subtree recursively
         *
         * @param n 子树根节点 / subtree root
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
         * @brief 左旋操作 / left rotation
         *
         * @param x 旋转轴节点 / pivot node
         * @return 新子树根节点 / new subtree root
         */
        node *rotate_left(node *x) noexcept
        {
            node *y = x->right;
            node *B = y->left;

            y->left = x;
            x->right = B;

            if (B)
                B->parent = x;

            node *p = x->parent;
            y->parent = p;
            x->parent = y;

            if (p == header_)
            {
                set_root(y);
            }
            else if (p->left == x)
            {
                p->left = y;
            }
            else
            {
                p->right = y;
            }

            update_height(x);
            update_height(y);

            return y;
        }

        /**
         * @brief 右旋操作 / right rotation
         *
         * @param y 旋转轴节点 / pivot node
         * @return 新子树根节点 / new subtree root
         */
        node *rotate_right(node *y) noexcept
        {
            node *x = y->left;
            node *B = x->right;

            x->right = y;
            y->left = B;

            if (B)
                B->parent = y;

            node *p = y->parent;
            x->parent = p;
            y->parent = x;

            if (p == header_)
            {
                set_root(x);
            }
            else if (p->left == y)
            {
                p->left = x;
            }
            else
            {
                p->right = x;
            }

            update_height(y);
            update_height(x);

            return x;
        }

        /**
         * @brief 从某节点开始向上重平衡 / rebalance from a node upwards
         *
         * @param n 起始节点 / starting node
         */
        void rebalance_from(node *n) noexcept
        {
            while (n && n != header_)
            {
                update_height(n);
                int bf = balance_factor(n);

                if (bf > 1)
                {
                    if (balance_factor(n->left) < 0)
                        rotate_left(n->left);
                    n = rotate_right(n);
                }
                else if (bf < -1)
                {
                    if (balance_factor(n->right) > 0)
                        rotate_right(n->right);
                    n = rotate_left(n);
                }

                if (n->parent == header_)
                    break;
                n = n->parent;
            }
            update_extreme();
        }

        /**
         * @brief 更新 header 的最左和最右指针 / update leftmost & rightmost in header
         */
        void update_extreme() noexcept
        {
            node *r = root();
            if (!r)
            {
                header_->left = header_;
                header_->right = header_;
                return;
            }
            header_->left = min_node(r);
            header_->right = max_node(r);
        }

        /**
         * @brief 插入实现函数 / internal insert implementation
         *
         * @tparam U 值类型 / value type
         * @param value 要插入的值 / value to insert
         * @return pair(迭代器, 是否插入成功)
         *         pair(iterator, inserted ?)
         */
        template <class U>
        std::pair<iterator, bool> insert_impl(U &&value)
        {
            node *r = root();

            if (!r)
            {
                node *n = create_node(std::forward<U>(value));
                set_root(n);
                header_->left = n;
                header_->right = n;
                size_ = 1;
                return {iterator(n), true};
            }

            node *cur = r;
            node *parent = header_;
            bool left_child = false;

            while (cur)
            {
                parent = cur;
                if (comp_(value, cur->value))
                {
                    cur = cur->left;
                    left_child = true;
                }
                else if (comp_(cur->value, value))
                {
                    cur = cur->right;
                    left_child = false;
                }
                else
                {
                    return {iterator(cur), false};
                }
            }

            node *n = create_node(std::forward<U>(value));
            n->parent = parent;

            if (parent == header_)
            {
                set_root(n);
            }
            else if (left_child)
            {
                parent->left = n;
            }
            else
            {
                parent->right = n;
            }

            ++size_;
            if (header_->left == header_ || comp_(n->value, header_->left->value))
                header_->left = n;
            if (header_->right == header_ || comp_(header_->right->value, n->value))
                header_->right = n;

            rebalance_from(parent);

            return {iterator(n), true};
        }

        /**
         * @brief 用 v 子树替换 u 子树 / transplant subtree v in place of u
         *
         * @param u 原子树根节点 / original subtree root
         * @param v 新子树根节点 / new subtree root
         */
        void transplant(node *u, node *v) noexcept
        {
            node *p = u->parent;
            if (p == header_)
            {
                set_root(v);
            }
            else if (p->left == u)
            {
                p->left = v;
            }
            else
            {
                p->right = v;
            }
            if (v)
                v->parent = p;
        }

        /**
         * @brief 删除指定节点并保持 AVL 平衡 / erase node and keep AVL balance
         *
         * @param z 待删除节点 / node to erase
         */
        void erase_node(node *z)
        {
            node *rebalance_start = nullptr;

            if (!z->left)
            {
                rebalance_start = z->parent;
                transplant(z, z->right);
            }
            else if (!z->right)
            {
                rebalance_start = z->parent;
                transplant(z, z->left);
            }
            else
            {
                node *y = min_node(z->right);
                if (y->parent != z)
                {
                    rebalance_start = y->parent;
                    transplant(y, y->right);
                    y->right = z->right;
                    if (y->right)
                        y->right->parent = y;
                }
                else
                {
                    rebalance_start = y;
                }

                transplant(z, y);
                y->left = z->left;
                if (y->left)
                    y->left->parent = y;
                update_height(y);
            }

            if (header_->left == z)
                header_->left = (size_ > 1 ? min_node(root()) : header_);
            if (header_->right == z)
                header_->right = (size_ > 1 ? max_node(root()) : header_);

            destroy_node(z);
            --size_;

            if (rebalance_start && rebalance_start != header_)
                rebalance_from(rebalance_start);
            else
                update_extreme();
        }
    };

} // namespace tf

#endif
