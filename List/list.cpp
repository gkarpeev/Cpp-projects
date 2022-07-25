#include <iostream>
#include <memory>
#include <cstddef>

template<size_t N>
class alignas(std::max_align_t) StackStorage {
private:
    char buf_[N];
    void* current_;
    size_t sz_;
public:
    StackStorage() : current_(buf_), sz_(N) {}

    void* get_free_memory(size_t n, size_t alignment) {
        void* result = std::align(alignment, n, current_, sz_);
        if (result) {
            current_ = reinterpret_cast<char*>(current_) + n;
            sz_ -= reinterpret_cast<char*>(current_) - reinterpret_cast<char*>(result);
        } else {
            throw std::bad_alloc();
        }
        return result;
    }

    ~StackStorage() = default;
};

template<typename T, size_t N>
class StackAllocator {
private:
    StackStorage<N>* buf_;

public:
    using value_type = T;

    template<typename U>
    struct rebind {
        typedef StackAllocator<U, N> other;
    };

    StackAllocator() = default;

    StackAllocator(StackStorage<N>& other) : buf_(&other) {}

    StackStorage<N>* get_buf() const { return buf_; }

    template<typename U>
    StackAllocator(const StackAllocator<U, N>& other) : buf_(other.get_buf()) {}

    template<typename U>
    StackAllocator<T, N>& operator=(const StackAllocator<U, N>& a) {
        buf_ = a.get_buf();
        return *this;
    }

    T* allocate(size_t n) const {
        return reinterpret_cast<T*>(buf_->get_free_memory(n * sizeof(T), alignof(T)));
    }

    bool operator==(const StackAllocator& other) {
        return (buf_ == other.get_buf());
    }

    bool operator!=(const StackAllocator& other) {
        return !(*this == other);
    }

    void deallocate(T*, size_t) const {}

    ~StackAllocator() = default;
};

template<typename T, typename Allocator = std::allocator<T>>
class List {
private:
    struct BaseNode {
        BaseNode* next_ = this;
        BaseNode* prev_ = this;

        BaseNode() = default;
    };

    struct Node : BaseNode {
        T value_;

        Node() = default;
        Node(const T& other): value_(other) {}
    };

    using AllocTraits = std::allocator_traits<Allocator>;
    using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;
    using NodeAllocTraits = typename AllocTraits::template rebind_traits<Node>;

    BaseNode end_;
    size_t size_ = 0;

    NodeAlloc node_allocator_;

private:
    template<typename... Args>
    BaseNode* construct_node(Args&&... args) {
        Node* new_node = NodeAllocTraits::allocate(node_allocator_, 1);
        try {
            NodeAllocTraits::construct(node_allocator_, new_node, std::forward<Args>(args)...);
        } catch (...) {
            NodeAllocTraits::deallocate(node_allocator_, new_node, 1);
            throw;
        }
        return new_node;
    }

    static void insert_node_before(BaseNode* node, BaseNode* node_before) {
        node_before->next_ = node;
        node_before->prev_ = node->prev_;
        node->prev_->next_ = node_before;
        node->prev_ = node_before;
    }

    template<typename... Args>
    void create_node_before(BaseNode* node, Args&&... args) {
        insert_node_before(node, construct_node(std::forward<Args>(args)...));
    }

    void delete_node(BaseNode* node) {
        node->prev_->next_ = node->next_;
        node->next_->prev_ = node->prev_;
        NodeAllocTraits::destroy(node_allocator_, reinterpret_cast<Node*>(node));
        NodeAllocTraits::deallocate(node_allocator_, reinterpret_cast<Node*>(node), 1);
    }

    static void swap(BaseNode* first_node, BaseNode* second_node) {
        std::swap(first_node->next_, second_node->next_);
        std::swap(first_node->prev_, second_node->prev_);
    }

    static void rebuild_end(BaseNode* first_end, BaseNode* second_end) {
        first_end->next_->prev_ = second_end;
        first_end->prev_->next_ = second_end;
    }

    static void swap(List& first_list, List& second_list) {
        rebuild_end(&first_list.end_, &second_list.end_);
        rebuild_end(&second_list.end_, &first_list.end_);
        swap(&second_list.end_, &first_list.end_);
        std::swap(first_list.size_, second_list.size_);
    }

public:
    List(const Allocator& new_allocator = Allocator()): node_allocator_(new_allocator) {}

    List(size_t n, const T& value, const Allocator& new_allocator = Allocator()) : List(new_allocator) {
        for (size_t i = 0; i < n; i++) {
            push_back(value);
        }
    }

    List(size_t n, const Allocator& new_allocator = Allocator()) : List(new_allocator) {
        for (size_t i = 0; i < n; i++) {
            emplace_back();
        }
    }

    List(const List& other_list) : List(AllocTraits::select_on_container_copy_construction(other_list.get_allocator())) {
        for (auto it = other_list.begin(); it != other_list.end(); ++it) {
            push_back(*it);
        }
    }

    void clear() {
        for (size_t i = size_; i > 0; i--) {
            pop_back();
        }
    }

    List& operator=(const List& other_list) {
        List new_list(other_list);
        new_list.node_allocator_ = node_allocator_;
        if (AllocTraits::propagate_on_container_copy_assignment::value) node_allocator_ = other_list.node_allocator_;
        swap(*this, new_list);
        return *this;
    }

    Allocator get_allocator() const {
        return static_cast<Allocator>(node_allocator_);
    }

    size_t size() const {
        return size_;
    }

    template<bool IsConst>
    struct common_iterator {
    private:
        BaseNode* ptr_;

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = std::conditional_t<IsConst, const T, T>;
        using pointer = std::conditional_t<IsConst, const T*, T*>;
        using reference = std::conditional_t<IsConst, const T&, T&>;
        using iterator_category = std::bidirectional_iterator_tag;

        common_iterator(const common_iterator<false>& other_iterator) : ptr_(other_iterator.get_ptr()) {}
        common_iterator(const BaseNode* ptr) : ptr_(const_cast<BaseNode*>(ptr)) {}

        common_iterator& operator=(const common_iterator& other_iterator) {
            ptr_ = other_iterator.get_ptr();
            return *this;
        }

        BaseNode* get_ptr() const {
            return ptr_;
        }

        reference operator*() const {
            return static_cast<std::conditional_t<IsConst, const Node*, Node*>>(ptr_)->value_;
        }

        pointer operator->() const {
            return &(static_cast<std::conditional_t<IsConst, const Node*, Node*>>(ptr_)->value_);
        }

        common_iterator& operator++() {
            ptr_ = ptr_->next_;
            return *this;
        }

        common_iterator operator++(int) {
            common_iterator old_ptr = *this;
            ptr_= ptr_->next_;
            return old_ptr;
        }

        common_iterator& operator--() {
            ptr_ = ptr_->prev_;
            return *this;
        }

        common_iterator operator--(int) {
            common_iterator old_ptr = *this;
            ptr_ = ptr_->prev_;
            return old_ptr;
        }

        template<bool IsConstIterator>
        bool operator==(const common_iterator<IsConstIterator>& other_iterator) {
            return ptr_ == other_iterator.get_ptr();
        }

        template<bool IsConstIterator>
        bool operator!=(const common_iterator<IsConstIterator>& other_iterator) {
            return !(*this == other_iterator);
        }
    };

    using iterator = common_iterator<false>;
    using const_iterator = common_iterator<true>;
    using reverse_iterator = std::reverse_iterator<iterator>;
    using const_reverse_iterator = std::reverse_iterator<const_iterator>;

    template<bool IsConstIterator, typename... Args>
    void insert(const common_iterator<IsConstIterator>& it, Args&&... args) {
        create_node_before(it.get_ptr(), std::forward<Args>(args)...);
        size_++;
    }

    template<bool IsConstIterator>
    void erase(const common_iterator<IsConstIterator>& it) {
        delete_node(it.get_ptr());
        size_--;
    }

    void push_back(const T& value) {
        insert(end(), value);
    }

    template<typename... Args>
    void emplace_back(Args&&... args) {
        insert(end(), std::forward<Args>(args)...);
    }

    void push_front(const T& value) {
        insert(begin(), value);
    }

    void pop_back() {
        erase(--end());
    }

    void pop_front() {
        erase(begin());
    }

    iterator begin() {
        return iterator(end_.next_);
    }

    const_iterator begin() const {
        return const_iterator(end_.next_);
    }

    const_iterator cbegin() const {
        return const_iterator(end_.next_);
    }

    reverse_iterator rbegin() {
        return reverse_iterator(&end_);
    }

    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(&end_);
    }

    const_reverse_iterator crbegin() const {
        return const_reverse_iterator(&end_);
    }

    iterator end() {
        return iterator(&end_);
    }

    const_iterator end() const {
        return const_iterator(&end_);
    }

    const_iterator cend() const {
        return const_iterator(&end_);
    }

    reverse_iterator rend() {
        return reverse_iterator(&end_);
    }

    const_reverse_iterator rend() const {
        return const_reverse_iterator(&end_);
    }

    const_reverse_iterator crend() const {
        return const_reverse_iterator(&end_);
    }

    ~List() {
        clear();
    }
};
