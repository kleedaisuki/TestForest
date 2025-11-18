#ifndef _RED_BLACK_TREE_HPP
#define _RED_BLACK_TREE_HPP

#include <memory>
#include <utility>
#include <cstddef>
#include <initializer_list>
#include <iterator>

namespace tf
{

    /**
     * @brief 红黑树（std::set 风格有序集合）/ Red-black tree (std::set-style ordered set).
     *
     * 这是一个以红黑树（Red-Black Tree）为底层结构的有序集合容器，
     * 提供类似 `std::set<Key>` 的接口：键唯一、有序存储、支持迭代器遍历。
     *
     * @tparam Key      关键字类型 / key type
     * @tparam Compare  比较器（缺省为 std::less<Key>）/ comparator (default std::less<Key>)
     * @tparam Allocator 分配器（缺省为 std::allocator<Key>）/ allocator (default std::allocator<Key>)
     */
    template <typename Key,
              typename Compare = std::less<Key>,
              typename Allocator = std::allocator<Key>>
    class RedBlackTree
    {
    private:
        /**
         * @brief 节点颜色 / Node color.
         */
        enum class Color : unsigned char
        {
            Red,
            Black
        };

        /**
         * @brief 红黑树节点结构 / Node structure of red-black tree.
         *
         * @note 这里所有空指针都被统一为哨兵节点 nil_，用来消除特殊情况。
         *       All "null" children are represented by the sentinel node nil_
         *       to eliminate special cases.
         */
        struct Node
        {
            /// @brief 父节点指针 / Parent pointer.
            Node *parent;
            /// @brief 左子节点指针 / Left child pointer.
            Node *left;
            /// @brief 右子节点指针 / Right child pointer.
            Node *right;
            /// @brief 节点颜色 / Node color.
            Color color;
            /// @brief 存储的键值 / Stored key value.
            Key value;

            /**
             * @brief 默认构造节点 / Default constructor.
             */
            Node()
                : parent(nullptr), left(nullptr), right(nullptr), color(Color::Black), value()
            {
            }

            /**
             * @brief 使用给定值和颜色构造节点 / Construct node with given value and color.
             *
             * @param v    [in] 键值 / key value
             * @param c    [in] 颜色 / color
             * @param nil  [in] 哨兵节点指针，用于初始化子指针 / sentinel pointer to init children
             */
            Node(const Key &v, Color c, Node *nil)
                : parent(nil), left(nil), right(nil), color(c), value(v)
            {
            }
        };

    public:
        /// @brief 键类型 / key type.
        using key_type = Key;
        /// @brief 值类型（与 key_type 相同）/ value type (same as key_type).
        using value_type = Key;
        /// @brief 比较器类型 / key comparator type.
        using key_compare = Compare;
        /// @brief 值比较器类型（与 key_compare 相同）/ value comparator type (same as key_compare).
        using value_compare = Compare;
        /// @brief 分配器类型 / allocator type.
        using allocator_type = Allocator;
        /// @brief 引用类型 / reference type.
        using reference = value_type &;
        /// @brief 常量引用类型 / const reference type.
        using const_reference = const value_type &;
        /// @brief 指针类型 / pointer type.
        using pointer = typename std::allocator_traits<Allocator>::pointer;
        /// @brief 常量指针类型 / const pointer type.
        using const_pointer = typename std::allocator_traits<Allocator>::const_pointer;
        /// @brief 差值类型 / difference type.
        using difference_type = std::ptrdiff_t;
        /// @brief 大小类型 / size type.
        using size_type = std::size_t;

    private:
        /// @brief 节点分配器类型 / node allocator type.
        using node_allocator_type =
            typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
        /// @brief 节点分配器 traits / node allocator traits.
        using node_alloc_traits = std::allocator_traits<node_allocator_type>;

        /**
         * @brief 常量双向迭代器（std::set 风格）/ Constant bidirectional iterator (std::set-like).
         *
         * @note 迭代器只允许读，不允许修改值，保持与 `std::set` 一致。
         *       Iterator is read-only to keep consistency with `std::set`.
         */
        class const_iterator_impl
        {
        public:
            /// @brief 迭代器类别 / iterator category.
            using iterator_category = std::bidirectional_iterator_tag;
            /// @brief 值类型 / value type.
            using value_type = Key;
            /// @brief 差值类型 / difference type.
            using difference_type = std::ptrdiff_t;
            /// @brief 指针类型 / pointer type.
            using pointer = const Key *;
            /// @brief 引用类型 / reference type.
            using reference = const Key &;

            /**
             * @brief 默认构造空迭代器 / Default constructor (null iterator).
             */
            const_iterator_impl() noexcept
                : node_(nullptr), nil_(nullptr)
            {
            }

            /**
             * @brief 使用节点和哨兵构造迭代器 / Construct iterator from node and sentinel.
             *
             * @param node [in] 当前节点指针 / current node pointer
             * @param nil  [in] 哨兵节点指针 / sentinel node pointer
             */
            const_iterator_impl(Node *node, Node *nil) noexcept
                : node_(node), nil_(nil)
            {
            }

            /**
             * @brief 解引用迭代器 / Dereference iterator.
             *
             * @return 当前节点的键值常量引用 / const reference to current key.
             */
            reference operator*() const noexcept
            {
                return node_->value;
            }

            /**
             * @brief 成员访问操作符 / Member access operator.
             *
             * @return 当前节点键值的常量指针 / const pointer to current key.
             */
            pointer operator->() const noexcept
            {
                return &(node_->value);
            }

            /**
             * @brief 前置自增，移动到中序遍历的下一个节点
             *        / Pre-increment, move to next node in in-order traversal.
             */
            const_iterator_impl &operator++() noexcept
            {
                increment();
                return *this;
            }

            /**
             * @brief 后置自增 / Post-increment.
             */
            const_iterator_impl operator++(int) noexcept
            {
                const_iterator_impl tmp(*this);
                increment();
                return tmp;
            }

            /**
             * @brief 前置自减，移动到中序遍历的前一个节点
             *        / Pre-decrement, move to previous node in in-order traversal.
             */
            const_iterator_impl &operator--() noexcept
            {
                decrement();
                return *this;
            }

            /**
             * @brief 后置自减 / Post-decrement.
             */
            const_iterator_impl operator--(int) noexcept
            {
                const_iterator_impl tmp(*this);
                decrement();
                return tmp;
            }

            /**
             * @brief 等于比较 / Equality comparison.
             */
            friend bool operator==(const const_iterator_impl &a,
                                   const const_iterator_impl &b) noexcept
            {
                return a.node_ == b.node_;
            }

            /**
             * @brief 不等比较 / Inequality comparison.
             */
            friend bool operator!=(const const_iterator_impl &a,
                                   const const_iterator_impl &b) noexcept
            {
                return !(a == b);
            }

        private:
            /// @brief 当前节点指针 / current node pointer.
            Node *node_;
            /// @brief 哨兵节点指针 / sentinel node pointer.
            Node *nil_;

            /**
             * @brief 前进到中序遍历下一个节点 / Move to next node in in-order order.
             */
            void increment() noexcept
            {
                if (!node_ || !nil_)
                    return;

                if (node_->right != nil_)
                {
                    node_ = node_->right;
                    while (node_->left != nil_)
                    {
                        node_ = node_->left;
                    }
                    return;
                }

                Node *parent = node_->parent;
                while (parent != nil_ && node_ == parent->right)
                {
                    node_ = parent;
                    parent = parent->parent;
                }
                node_ = parent;
            }

            /**
             * @brief 后退到中序遍历前一个节点
             *        / Move to previous node in in-order order.
             *
             * @note 从 end()（指向 nil_）自减时，会跳到最大元素。
             *       Decrementing from end() (nil_) jumps to maximum element.
             */
            void decrement() noexcept
            {
                if (!node_ || !nil_)
                    return;

                if (node_ == nil_)
                {
                    // end() -- 变成最大节点 / end()-- becomes max node
                    node_ = nil_->left; // nil_->left 始终指向最大节点 / points to max
                    return;
                }

                if (node_->left != nil_)
                {
                    node_ = node_->left;
                    while (node_->right != nil_)
                    {
                        node_ = node_->right;
                    }
                    return;
                }

                Node *parent = node_->parent;
                while (parent != nil_ && node_ == parent->left)
                {
                    node_ = parent;
                    parent = parent->parent;
                }
                node_ = parent;
            }

            friend class RedBlackTree;
        };

    public:
        /// @brief 常量迭代器类型 / const iterator type.
        using const_iterator = const_iterator_impl;
        /// @brief 迭代器类型（与 const_iterator 相同）/ iterator type (same as const_iterator).
        using iterator = const_iterator;
        /// @brief 常量反向迭代器 / const reverse iterator type.
        using const_reverse_iterator = std::reverse_iterator<const_iterator>;
        /// @brief 反向迭代器类型 / reverse iterator type.
        using reverse_iterator = const_reverse_iterator;

        /**
         * @brief 默认构造函数，创建空红黑树
         *        / Default constructor, create empty red-black tree.
         */
        RedBlackTree()
            : root_(nullptr), nil_(nullptr), size_(0), comp_(), alloc_(), node_alloc_(alloc_)
        {
            init_nil();
        }

        /**
         * @brief 使用比较器和分配器构造 / Construct with comparator and allocator.
         *
         * @param comp [in] 比较器 / comparator
         * @param alloc [in] 分配器 / allocator
         */
        explicit RedBlackTree(const Compare &comp,
                              const Allocator &alloc = Allocator())
            : root_(nullptr), nil_(nullptr), size_(0), comp_(comp), alloc_(alloc), node_alloc_(alloc_)
        {
            init_nil();
        }

        /**
         * @brief 使用迭代器区间构造 / Construct from iterator range.
         *
         * @tparam InputIt 输入迭代器类型 / input iterator type
         * @param first [in] 起始迭代器 / first iterator
         * @param last  [in] 终止迭代器（开区间）/ last iterator (one past end)
         * @param comp  [in] 比较器 / comparator
         * @param alloc [in] 分配器 / allocator
         */
        template <typename InputIt>
        RedBlackTree(InputIt first, InputIt last,
                     const Compare &comp = Compare(),
                     const Allocator &alloc = Allocator())
            : root_(nullptr), nil_(nullptr), size_(0), comp_(comp), alloc_(alloc), node_alloc_(alloc_)
        {
            init_nil();
            insert(first, last);
        }

        /**
         * @brief 使用初始化列表构造 / Construct from initializer_list.
         *
         * @param init [in] 初始化列表 / initializer list
         * @param comp [in] 比较器 / comparator
         * @param alloc [in] 分配器 / allocator
         */
        RedBlackTree(std::initializer_list<value_type> init,
                     const Compare &comp = Compare(),
                     const Allocator &alloc = Allocator())
            : root_(nullptr), nil_(nullptr), size_(0), comp_(comp), alloc_(alloc), node_alloc_(alloc_)
        {
            init_nil();
            insert(init.begin(), init.end());
        }

        /**
         * @brief 拷贝构造函数 / Copy constructor.
         *
         * @param other [in] 另一个红黑树 / another red-black tree
         */
        RedBlackTree(const RedBlackTree &other)
            : root_(nullptr), nil_(nullptr), size_(0), comp_(other.comp_), alloc_(other.alloc_), node_alloc_(alloc_)
        {
            init_nil();
            insert(other.begin(), other.end());
        }

        /**
         * @brief 移动构造函数 / Move constructor.
         *
         * @param other [in,out] 被移动的红黑树 / red-black tree to move from
         */
        RedBlackTree(RedBlackTree &&other) noexcept
            : root_(other.root_), nil_(other.nil_), size_(other.size_), comp_(std::move(other.comp_)), alloc_(std::move(other.alloc_)), node_alloc_(std::move(other.node_alloc_))
        {
            other.root_ = nullptr;
            other.nil_ = nullptr;
            other.size_ = 0;
        }

        /**
         * @brief 析构函数 / Destructor.
         */
        ~RedBlackTree()
        {
            clear();
            destroy_nil();
        }

        /**
         * @brief 拷贝赋值 / Copy assignment.
         *
         * @param other [in] 另一个红黑树 / another red-black tree
         * @return 当前对象引用 / reference to this.
         */
        RedBlackTree &operator=(const RedBlackTree &other)
        {
            if (this == &other)
                return *this;

            RedBlackTree tmp(other);
            swap(tmp);
            return *this;
        }

        /**
         * @brief 移动赋值 / Move assignment.
         *
         * @param other [in,out] 被移动的红黑树 / red-black tree to move from
         * @return 当前对象引用 / reference to this.
         */
        RedBlackTree &operator=(RedBlackTree &&other) noexcept
        {
            if (this == &other)
                return *this;

            clear();
            destroy_nil();

            root_ = other.root_;
            nil_ = other.nil_;
            size_ = other.size_;
            comp_ = std::move(other.comp_);
            alloc_ = std::move(other.alloc_);
            node_alloc_ = std::move(other.node_alloc_);

            other.root_ = nullptr;
            other.nil_ = nullptr;
            other.size_ = 0;
            return *this;
        }

        /**
         * @brief 使用初始化列表赋值 / Assign from initializer_list.
         *
         * @param init [in] 初始化列表 / initializer list
         * @return 当前对象引用 / reference to this.
         */
        RedBlackTree &operator=(std::initializer_list<value_type> init)
        {
            RedBlackTree tmp(init, comp_, alloc_);
            swap(tmp);
            return *this;
        }

        /**
         * @brief 获取分配器 / Get allocator.
         *
         * @return 容器使用的分配器 / allocator used by the container.
         */
        allocator_type get_allocator() const noexcept
        {
            return alloc_;
        }

        // ======================== 迭代器接口 / Iterator interface ========================

        /**
         * @brief 返回指向第一个（最小）元素的迭代器
         *        / Return iterator to the first (smallest) element.
         */
        iterator begin() noexcept
        {
            return iterator(minimum_node(root_), nil_);
        }

        /**
         * @brief 返回指向第一个（最小）元素的常量迭代器
         *        / Return const iterator to the first (smallest) element.
         */
        const_iterator begin() const noexcept
        {
            return cbegin();
        }

        /**
         * @brief 返回指向第一个（最小）元素的常量迭代器
         *        / Return const iterator to the first (smallest) element.
         */
        const_iterator cbegin() const noexcept
        {
            return const_iterator(minimum_node(root_), nil_);
        }

        /**
         * @brief 返回指向尾后位置的迭代器 / Return iterator to end (past-the-end).
         *
         * @note end() 始终是哨兵节点 nil_。
         *       end() is always the sentinel node nil_.
         */
        iterator end() noexcept
        {
            return iterator(nil_, nil_);
        }

        /**
         * @brief 返回指向尾后位置的常量迭代器
         *        / Return const iterator to end (past-the-end).
         */
        const_iterator end() const noexcept
        {
            return cend();
        }

        /**
         * @brief 返回指向尾后位置的常量迭代器
         *        / Return const iterator to end (past-the-end).
         */
        const_iterator cend() const noexcept
        {
            return const_iterator(nil_, nil_);
        }

        /**
         * @brief 返回指向最后一个元素的反向迭代器
         *        / Return reverse iterator to last element.
         */
        reverse_iterator rbegin() noexcept
        {
            return reverse_iterator(end());
        }

        /**
         * @brief 返回指向最后一个元素的常量反向迭代器
         *        / Return const reverse iterator to last element.
         */
        const_reverse_iterator rbegin() const noexcept
        {
            return const_reverse_iterator(end());
        }

        /**
         * @brief 返回指向反向尾后位置的反向迭代器
         *        / Return reverse iterator to reverse-end.
         */
        reverse_iterator rend() noexcept
        {
            return reverse_iterator(begin());
        }

        /**
         * @brief 返回指向反向尾后位置的常量反向迭代器
         *        / Return const reverse iterator to reverse-end.
         */
        const_reverse_iterator rend() const noexcept
        {
            return const_reverse_iterator(begin());
        }

        // ======================== 容量接口 / Capacity interface ========================

        /**
         * @brief 是否为空 / Check whether container is empty.
         *
         * @return 若无元素则为 true / true if no elements.
         */
        bool empty() const noexcept
        {
            return size_ == 0;
        }

        /**
         * @brief 返回元素个数 / Return number of elements.
         */
        size_type size() const noexcept
        {
            return size_;
        }

        /**
         * @brief 可容纳的最大元素数（近似值）
         *        / Return maximum possible number of elements (approximate).
         */
        size_type max_size() const noexcept
        {
            return node_alloc_traits::max_size(node_alloc_);
        }

        // ======================== 修改器 / Modifiers ========================

        /**
         * @brief 清空容器 / Clear the container.
         *
         * @note 所有元素被销毁，但比较器和分配器保持不变。
         *       All elements are destroyed but comparator and allocator are kept.
         */
        void clear() noexcept
        {
            if (!root_ || !nil_)
                return;
            destroy_subtree(root_);
            root_ = nil_;
            nil_->parent = nullptr;
            nil_->left = nil_;
            nil_->right = nil_;
            size_ = 0;
        }

        /**
         * @brief 插入一个元素（若已存在等价键则不插入）
         *        / Insert one element (do not insert if an equivalent key exists).
         *
         * @param value [in] 要插入的值 / value to insert
         * @return pair<迭代器, 是否插入成功>
         *         pair<iterator, bool>: iterator to element, bool = whether inserted.
         */
        std::pair<iterator, bool> insert(const value_type &value)
        {
            Node *y = nil_;
            Node *x = root_;

            while (x != nil_)
            {
                y = x;
                if (comp_(value, x->value))
                {
                    x = x->left;
                }
                else if (comp_(x->value, value))
                {
                    x = x->right;
                }
                else
                {
                    return {iterator(x, nil_), false};
                }
            }

            Node *z = create_node(value);
            z->parent = y;

            if (y == nil_)
            {
                root_ = z;
            }
            else if (comp_(z->value, y->value))
            {
                y->left = z;
            }
            else
            {
                y->right = z;
            }

            insert_fixup(z);
            ++size_;
            update_nil_extremes();
            return {iterator(z, nil_), true};
        }

        /**
         * @brief 带 hint 的插入（当前实现忽略 hint，仅作为接口兼容）
         *        / Insert with hint (hint is ignored for now, kept for API compatibility).
         *
         * @param hint  [in] 插入位置提示迭代器 / position hint (ignored)
         * @param value [in] 值 / value to insert
         * @return 指向插入或已存在元素的迭代器 / iterator to inserted or existing element.
         */
        iterator insert(iterator /*hint*/, const value_type &value)
        {
            auto res = insert(value);
            return res.first;
        }

        /**
         * @brief 迭代器区间插入 / Insert range [first, last).
         *
         * @tparam InputIt 输入迭代器类型 / input iterator type
         * @param first [in] 起始迭代器 / first
         * @param last  [in] 终止迭代器（开区间）/ last (one past end)
         */
        template <typename InputIt>
        void insert(InputIt first, InputIt last)
        {
            for (; first != last; ++first)
            {
                insert(*first);
            }
        }

        /**
         * @brief 初始化列表插入 / Insert from initializer_list.
         *
         * @param init [in] 初始化列表 / initializer list
         */
        void insert(std::initializer_list<value_type> init)
        {
            insert(init.begin(), init.end());
        }

        /**
         * @brief 按键值删除元素（若存在）/ Erase element with given key (if exists).
         *
         * @param key [in] 要删除的键 / key to erase
         * @return 删除的元素个数（0 或 1）/ number of erased elements (0 or 1).
         */
        size_type erase(const key_type &key)
        {
            Node *z = find_node(key);
            if (z == nil_)
                return 0;
            erase_node(z);
            return 1;
        }

        /**
         * @brief 通过迭代器删除单个元素
         *        / Erase element pointed to by iterator.
         *
         * @param pos [in] 指向要删除元素的迭代器 / iterator to element
         * @return 删除元素后紧随其后的迭代器 / iterator following the erased element.
         */
        iterator erase(iterator pos)
        {
            if (pos == end())
                return end();
            Node *node = pos.node_;
            iterator next(pos);
            ++next;
            erase_node(node);
            return next;
        }

        /**
         * @brief 删除区间 [first, last)
         *        / Erase range [first, last).
         *
         * @param first [in] 起始迭代器 / first
         * @param last  [in] 终止迭代器（开区间）/ last
         */
        iterator erase(iterator first, iterator last)
        {
            while (first != last)
            {
                first = erase(first);
            }
            return last;
        }

        /**
         * @brief 交换两个红黑树的内容
         *        / Swap the contents of two red-black trees.
         *
         * @param other [in,out] 另一个红黑树 / another red-black tree
         */
        void swap(RedBlackTree &other) noexcept
        {
            using std::swap;
            swap(root_, other.root_);
            swap(nil_, other.nil_);
            swap(size_, other.size_);
            swap(comp_, other.comp_);
            swap(alloc_, other.alloc_);
            swap(node_alloc_, other.node_alloc_);
        }

        // ======================== 查找操作 / Lookup ========================

        /**
         * @brief 查找等于给定键的元素
         *        / Find element with key equivalent to given key.
         *
         * @param key [in] 要查找的键 / key to find
         * @return 指向该元素的迭代器，若不存在返回 end()
         *         / iterator to element or end() if not found.
         */
        iterator find(const key_type &key)
        {
            return iterator(find_node(key), nil_);
        }

        /**
         * @brief 查找等于给定键的元素（常量版本）
         *        / Find element with key equivalent to given key (const).
         */
        const_iterator find(const key_type &key) const
        {
            return const_iterator(find_node(key), nil_);
        }

        /**
         * @brief 返回等于给定键的元素个数（set 中为 0 或 1）
         *        / Count elements with a given key (0 or 1 in a set).
         */
        size_type count(const key_type &key) const
        {
            return find_node(key) == nil_ ? 0u : 1u;
        }

        /**
         * @brief 返回指向首个不小于 key 的元素迭代器
         *        / Return iterator to first element not less than key (lower_bound).
         */
        iterator lower_bound(const key_type &key)
        {
            return iterator(lower_bound_node(key), nil_);
        }

        /**
         * @brief 返回指向首个不小于 key 的元素常量迭代器
         *        / Return const iterator to first element not less than key.
         */
        const_iterator lower_bound(const key_type &key) const
        {
            return const_iterator(lower_bound_node(key), nil_);
        }

        /**
         * @brief 返回指向首个大于 key 的元素迭代器
         *        / Return iterator to first element greater than key (upper_bound).
         */
        iterator upper_bound(const key_type &key)
        {
            return iterator(upper_bound_node(key), nil_);
        }

        /**
         * @brief 返回指向首个大于 key 的元素常量迭代器
         *        / Return const iterator to first element greater than key.
         */
        const_iterator upper_bound(const key_type &key) const
        {
            return const_iterator(upper_bound_node(key), nil_);
        }

        /**
         * @brief 返回等价键的范围 [lower_bound, upper_bound)
         *        / Return range of elements equivalent to key.
         */
        std::pair<iterator, iterator> equal_range(const key_type &key)
        {
            return {lower_bound(key), upper_bound(key)};
        }

        /**
         * @brief 返回等价键范围（常量版本）
         *        / Return range of elements equivalent to key (const).
         */
        std::pair<const_iterator, const_iterator> equal_range(const key_type &key) const
        {
            return {lower_bound(key), upper_bound(key)};
        }

        // ======================== 观察器 / Observers ========================

        /**
         * @brief 获取键比较器 / Get key comparator.
         */
        key_compare key_comp() const
        {
            return comp_;
        }

        /**
         * @brief 获取值比较器（与键比较器相同）
         *        / Get value comparator (same as key comparator).
         */
        value_compare value_comp() const
        {
            return comp_;
        }

    private:
        /// @brief 根节点指针 / root node pointer.
        Node *root_;
        /// @brief 哨兵 nil 节点指针 / sentinel nil node pointer.
        Node *nil_;
        /// @brief 元素个数 / number of elements.
        size_type size_;
        /// @brief 键比较器 / key comparator.
        key_compare comp_;
        /// @brief 元素分配器 / allocator for values.
        allocator_type alloc_;
        /// @brief 节点分配器 / allocator for nodes.
        node_allocator_type node_alloc_;

        // ======================== 内部工具函数 / Internal helpers ========================

        /**
         * @brief 初始化哨兵 nil 节点 / Initialize sentinel nil node.
         */
        void init_nil()
        {
            nil_ = node_alloc_traits::allocate(node_alloc_, 1);
            node_alloc_traits::construct(node_alloc_, nil_);
            nil_->color = Color::Black;
            nil_->parent = nullptr;
            nil_->left = nil_;
            nil_->right = nil_;
            root_ = nil_;
        }

        /**
         * @brief 销毁哨兵 nil 节点 / Destroy sentinel nil node.
         */
        void destroy_nil()
        {
            if (!nil_)
                return;
            node_alloc_traits::destroy(node_alloc_, nil_);
            node_alloc_traits::deallocate(node_alloc_, nil_, 1);
            nil_ = nullptr;
            root_ = nullptr;
        }

        /**
         * @brief 根据值创建新节点（红色）
         *        / Create a new red node with given value.
         */
        Node *create_node(const value_type &value)
        {
            Node *n = node_alloc_traits::allocate(node_alloc_, 1);
            try
            {
                node_alloc_traits::construct(node_alloc_, n, value, Color::Red, nil_);
            }
            catch (...)
            {
                node_alloc_traits::deallocate(node_alloc_, n, 1);
                throw;
            }
            return n;
        }

        /**
         * @brief 销毁节点 / Destroy a node.
         */
        void destroy_node(Node *node)
        {
            node_alloc_traits::destroy(node_alloc_, node);
            node_alloc_traits::deallocate(node_alloc_, node, 1);
        }

        /**
         * @brief 递归销毁子树 / Recursively destroy a subtree.
         *
         * @param node [in] 子树根节点 / root of subtree
         */
        void destroy_subtree(Node *node)
        {
            if (!node || node == nil_)
                return;
            destroy_subtree(node->left);
            destroy_subtree(node->right);
            destroy_node(node);
        }

        /**
         * @brief 左旋转操作 / Left rotation.
         *
         * @param x [in] 旋转支点节点 / pivot node.
         */
        void rotate_left(Node *x)
        {
            Node *y = x->right;
            x->right = y->left;

            if (y->left != nil_)
            {
                y->left->parent = x;
            }

            y->parent = x->parent;

            if (x->parent == nil_)
            {
                root_ = y;
            }
            else if (x == x->parent->left)
            {
                x->parent->left = y;
            }
            else
            {
                x->parent->right = y;
            }

            y->left = x;
            x->parent = y;
        }

        /**
         * @brief 右旋转操作 / Right rotation.
         *
         * @param x [in] 旋转支点节点 / pivot node.
         */
        void rotate_right(Node *x)
        {
            Node *y = x->left;
            x->left = y->right;

            if (y->right != nil_)
            {
                y->right->parent = x;
            }

            y->parent = x->parent;

            if (x->parent == nil_)
            {
                root_ = y;
            }
            else if (x == x->parent->right)
            {
                x->parent->right = y;
            }
            else
            {
                x->parent->left = y;
            }

            y->right = x;
            x->parent = y;
        }

        /**
         * @brief 插入修复：保持红黑树性质
         *        / Insert fixup: restore red-black properties after insertion.
         */
        void insert_fixup(Node *z)
        {
            while (z->parent->color == Color::Red)
            {
                if (z->parent == z->parent->parent->left)
                {
                    Node *y = z->parent->parent->right; // 叔叔 / uncle
                    if (y->color == Color::Red)
                    {
                        z->parent->color = Color::Black;
                        y->color = Color::Black;
                        z->parent->parent->color = Color::Red;
                        z = z->parent->parent;
                    }
                    else
                    {
                        if (z == z->parent->right)
                        {
                            z = z->parent;
                            rotate_left(z);
                        }
                        z->parent->color = Color::Black;
                        z->parent->parent->color = Color::Red;
                        rotate_right(z->parent->parent);
                    }
                }
                else
                {
                    Node *y = z->parent->parent->left;
                    if (y->color == Color::Red)
                    {
                        z->parent->color = Color::Black;
                        y->color = Color::Black;
                        z->parent->parent->color = Color::Red;
                        z = z->parent->parent;
                    }
                    else
                    {
                        if (z == z->parent->left)
                        {
                            z = z->parent;
                            rotate_right(z);
                        }
                        z->parent->color = Color::Black;
                        z->parent->parent->color = Color::Red;
                        rotate_left(z->parent->parent);
                    }
                }
            }
            root_->color = Color::Black;
        }

        /**
         * @brief 用 v 子树替换 u 子树的根位置（不改变颜色）
         *        / Transplant subtree rooted at u with subtree rooted at v.
         */
        void transplant(Node *u, Node *v)
        {
            if (u->parent == nil_)
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
            v->parent = u->parent;
        }

        /**
         * @brief 删除节点并修复红黑树性质
         *        / Erase node and restore red-black properties.
         *
         * @param z [in] 要删除的节点 / node to erase
         */
        void erase_node(Node *z)
        {
            Node *y = z;
            Node *x = nullptr;
            Color y_original_color = y->color;

            if (z->left == nil_)
            {
                x = z->right;
                transplant(z, z->right);
            }
            else if (z->right == nil_)
            {
                x = z->left;
                transplant(z, z->left);
            }
            else
            {
                y = minimum_node(z->right);
                y_original_color = y->color;
                x = y->right;

                if (y->parent == z)
                {
                    x->parent = y;
                }
                else
                {
                    transplant(y, y->right);
                    y->right = z->right;
                    y->right->parent = y;
                }

                transplant(z, y);
                y->left = z->left;
                y->left->parent = y;
                y->color = z->color;
            }

            destroy_node(z);
            --size_;

            if (y_original_color == Color::Black)
            {
                erase_fixup(x);
            }

            update_nil_extremes();
        }

        /**
         * @brief 删除修复：保持红黑树性质
         *        / Erase fixup: restore red-black properties after deletion.
         */
        void erase_fixup(Node *x)
        {
            while (x != root_ && x->color == Color::Black)
            {
                if (x == x->parent->left)
                {
                    Node *w = x->parent->right;
                    if (w->color == Color::Red)
                    {
                        w->color = Color::Black;
                        x->parent->color = Color::Red;
                        rotate_left(x->parent);
                        w = x->parent->right;
                    }
                    if (w->left->color == Color::Black &&
                        w->right->color == Color::Black)
                    {
                        w->color = Color::Red;
                        x = x->parent;
                    }
                    else
                    {
                        if (w->right->color == Color::Black)
                        {
                            w->left->color = Color::Black;
                            w->color = Color::Red;
                            rotate_right(w);
                            w = x->parent->right;
                        }
                        w->color = x->parent->color;
                        x->parent->color = Color::Black;
                        w->right->color = Color::Black;
                        rotate_left(x->parent);
                        x = root_;
                    }
                }
                else
                {
                    Node *w = x->parent->left;
                    if (w->color == Color::Red)
                    {
                        w->color = Color::Black;
                        x->parent->color = Color::Red;
                        rotate_right(x->parent);
                        w = x->parent->left;
                    }
                    if (w->right->color == Color::Black &&
                        w->left->color == Color::Black)
                    {
                        w->color = Color::Red;
                        x = x->parent;
                    }
                    else
                    {
                        if (w->left->color == Color::Black)
                        {
                            w->right->color = Color::Black;
                            w->color = Color::Red;
                            rotate_left(w);
                            w = x->parent->left;
                        }
                        w->color = x->parent->color;
                        x->parent->color = Color::Black;
                        w->left->color = Color::Black;
                        rotate_right(x->parent);
                        x = root_;
                    }
                }
            }
            x->color = Color::Black;
        }

        /**
         * @brief 查找最小节点 / Find minimum node in subtree.
         *
         * @param node [in] 子树根 / root of subtree
         * @return 最小节点指针 / pointer to minimum node
         */
        Node *minimum_node(Node *node) const noexcept
        {
            if (!node || node == nil_)
                return nil_;
            while (node->left != nil_)
            {
                node = node->left;
            }
            return node;
        }

        /**
         * @brief 查找最大节点 / Find maximum node in subtree.
         */
        Node *maximum_node(Node *node) const noexcept
        {
            if (!node || node == nil_)
                return nil_;
            while (node->right != nil_)
            {
                node = node->right;
            }
            return node;
        }

        /**
         * @brief 内部查找等于给定键的节点
         *        / Internal find node with given key.
         */
        Node *find_node(const key_type &key) const
        {
            Node *cur = root_;
            while (cur != nil_)
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
            return nil_;
        }

        /**
         * @brief 内部 lower_bound 查找
         *        / Internal lower_bound: first node not less than key.
         */
        Node *lower_bound_node(const key_type &key) const
        {
            Node *cur = root_;
            Node *res = nil_;

            while (cur != nil_)
            {
                if (!comp_(cur->value, key))
                {
                    res = cur;
                    cur = cur->left;
                }
                else
                {
                    cur = cur->right;
                }
            }
            return res;
        }

        /**
         * @brief 内部 upper_bound 查找
         *        / Internal upper_bound: first node greater than key.
         */
        Node *upper_bound_node(const key_type &key) const
        {
            Node *cur = root_;
            Node *res = nil_;

            while (cur != nil_)
            {
                if (comp_(key, cur->value))
                {
                    res = cur;
                    cur = cur->left;
                }
                else
                {
                    cur = cur->right;
                }
            }
            return res;
        }

        /**
         * @brief 更新 nil_ 的辅助信息，使得：
         *        nil_->left 指向最大节点，nil_->right 指向最小节点。
         *
         *        / Update auxiliary info of nil_ so that:
         *          nil_->left is maximum node, nil_->right is minimum node.
         *
         * @note 这样可以支持从 end()-- 跳到最大节点等行为。
         *       This supports behaviors like end()-- moving to max element.
         */
        void update_nil_extremes()
        {
            if (size_ == 0 || root_ == nil_)
            {
                nil_->left = nil_;
                nil_->right = nil_;
                return;
            }
            nil_->left = maximum_node(root_);
            nil_->right = minimum_node(root_);
        }

    }; // class RedBlackTree

} // namespace tf

#endif
