#include "list.cpp"
#include <iostream>
#include <cstring>
#include <vector>
#include <cmath>
#include <memory>
#include <cstddef>

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
class UnorderedMap;

template<typename Key, typename Value, typename Hash, typename Equal, typename Alloc>
void swap(UnorderedMap<Key, Value, Hash, Equal, Alloc>& first_map, UnorderedMap<Key, Value, Hash, Equal, Alloc>& second_map) {
    auto map_copy = std::move(second_map);
    second_map = std::move(first_map);
    first_map = std::move(map_copy);
}

template<typename Key, typename Value, typename Hash = std::hash<Key>, typename Equal = std::equal_to<Key>, typename Alloc = std::allocator<std::pair<const Key, Value>>>
class UnorderedMap {
public:
    using NodeType = std::pair<const Key, Value>;

private:
    static size_t get_hash(const Key& key) {
        return Hash{}(key);
    }

    size_t get_hash_id(size_t key_hash) const {
        return key_hash % buckets_.size();
    }

    struct Node {
        // key value
        NodeType key_value_;
        size_t key_hash_;

        Node(const NodeType& kv) : key_value_(kv) {
            key_hash_ = get_hash(key_value_.first);
        }

        Node(const Key& k, const Value& v) : key_value_(k, v) {
            key_hash_ = get_hash(key_value_.first);
        }

        Node(NodeType&& kv) = delete;

        Node(std::pair<Key, Value>&& kv) : key_value_(std::move(kv)) {
            key_hash_ = get_hash(key_value_.first);
        }

        Node(Key&& k, Value&& v) : key_value_(std::move(k), std::move(v)) {
            key_hash_ = get_hash(key_value_.first);
        }
    };

    using AllocTraits = std::allocator_traits<Alloc>;
    using NodeAlloc = typename AllocTraits::template rebind_alloc<Node>;

    using BucketList = List<Node, NodeAlloc>;
    using ListIterator = typename BucketList::iterator;

    using IterAlloc = typename AllocTraits::template rebind_alloc<typename BucketList::iterator>;
    size_t buckets_cnt_ = 0;
    double max_load_factor_ = 0.8;
    std::vector<ListIterator, IterAlloc> buckets_;
    BucketList list_;

public:
    UnorderedMap(Alloc allocator = Alloc()) : buckets_cnt_(1), buckets_(1, nullptr), list_(allocator) {}

    UnorderedMap(const UnorderedMap& other_map) : UnorderedMap(other_map.list_.get_allocator()) {
        max_load_factor_ = other_map.max_load_factor_;
        insert(other_map.begin(), other_map.end());
    }

    UnorderedMap(UnorderedMap&& other_map) : buckets_cnt_(other_map.buckets_cnt_), 
                                             max_load_factor_(other_map.max_load_factor_),
                                             buckets_(std::move(other_map.buckets_)),
                                             list_(std::move(other_map.list_)) {
        other_map.buckets_cnt_ = 0;
    }

    UnorderedMap& operator=(const UnorderedMap& other_map) {
        UnorderedMap copy_map = other_map;
        swap(this, copy_map);
        return *this;
    }

    UnorderedMap& operator=(UnorderedMap&& other_map) {
        buckets_cnt_ = other_map.buckets_cnt_;
        other_map.buckets_cnt_ = 0;
        max_load_factor_ = other_map.max_load_factor_;
        buckets_ = std::move(other_map.buckets_);
        list_ = std::move(other_map.list_);
        return *this;
    }

    template<bool IsConst>
    struct common_iterator {
    private:
        ListIterator iter_;

    public:
        using difference_type = std::ptrdiff_t;
        using value_type = std::conditional_t<IsConst, const NodeType, NodeType>;
        using pointer = std::conditional_t<IsConst, const NodeType*, NodeType*>;
        using reference = std::conditional_t<IsConst, const NodeType&, NodeType&>;
        using iterator_category = std::forward_iterator_tag;

        template<bool IsConstIterator, typename = std::enable_if_t<IsConst >= IsConstIterator>>
        common_iterator(const typename BucketList:: template common_iterator<IsConstIterator>& other_iterator) : 
                iter_(other_iterator.get_ptr()) {}

        template<bool IsConstIterator, typename = std::enable_if_t<IsConst >= IsConstIterator>>
        common_iterator(const common_iterator<IsConstIterator>& other_iterator) : iter_(other_iterator.get_iter()) {}

        template<bool IsConstIterator, typename = std::enable_if_t<IsConst >= IsConstIterator>>
        common_iterator(common_iterator<IsConstIterator>&& other_iterator): iter_(std::move(other_iterator.get_iter())) {}

        template<bool IsConstIterator, typename = std::enable_if_t<IsConst >= IsConstIterator>>
        common_iterator& operator=(const common_iterator<IsConstIterator>& other_iterator) {
            iter_ = other_iterator.get_iter();
            return *this;
        }

        ListIterator get_iter() const {
            return iter_;
        }

        reference operator*() const {
            return iter_->key_value_;
        }

        pointer operator->() const {
            return &(iter_->key_value_);
        }

        common_iterator& operator++() {
            ++iter_;
            return *this;
        }

        common_iterator operator++(int) {
            return common_iterator(iter_++);
        }

        common_iterator& operator--() {
            --iter_;
            return *this;
        }

        common_iterator operator--(int) {
            return common_iterator(iter_--);
        }

        template<bool IsConstIterator>
        bool operator==(const common_iterator<IsConstIterator>& other_iterator) const {
            return iter_ == other_iterator.get_iter();
        }

        template<bool IsConstIterator>
        bool operator!=(const common_iterator<IsConstIterator>& other_iterator) const {
            return !(*this == other_iterator);
        }
    };

    using iterator = common_iterator<false>;
    using const_iterator = common_iterator<true>;

    iterator begin() {
        return iterator(list_.begin());
    }

    const_iterator begin() const {
        return const_iterator(list_.begin());
    }

    const_iterator cbegin() const {
        return begin();
    }

    iterator end() {
        return iterator(list_.end());
    }

    const_iterator end() const {
        return const_iterator(list_.end());
    }

    const_iterator cend() const {
        return end();
    }

    void clear() {
        list_.clear();
    }

    size_t size() const {
        return list_.size();
    }

    size_t max_size() const {
        return 1 << 30;
    }

    size_t bucket_count() const {
        return buckets_cnt_;
    }

    double max_load_factor() const {
        return max_load_factor_;
    }

    void max_load_factor(double new_factor) {
        max_load_factor_ = new_factor;
    }

    double load_factor() const {
        return bucket_count() * 1.0 / buckets_.size();
    }

    ListIterator get_bucket_start(size_t key_id) {
        return (buckets_[key_id] ? buckets_[key_id] : list_.end());
    }

    void rehash(size_t new_sz) {
        buckets_.clear();
        buckets_.resize(new_sz, nullptr);
        buckets_cnt_ = 0;
        for (auto it = list_.begin(); it != list_.end(); ) {
            size_t key_id = get_hash_id(it->key_hash_);
            auto it2 = it++;
            list_.link_iterators(get_bucket_start(key_id), it2);
            if (get_bucket_start(key_id) == list_.end()) {
                buckets_cnt_++;
                buckets_[key_id] = it2;
            }
        }
    }

    void reserve(size_t reserved_sz) {
        rehash(std::ceil(static_cast<double>(reserved_sz) / max_load_factor()));
    }

    iterator find(const Key& key) {
        size_t key_hash = get_hash(key);
        size_t key_id = get_hash_id(key_hash);
        for (auto it = get_bucket_start(key_id); it != list_.end(); ++it) {
            if (get_hash_id(it->key_hash_) != key_id) break;
            if (Equal{}((it->key_value_).first, key)) return iterator(it);
        }
        return end();
    }

    const_iterator find(const Key& key) const {
        size_t key_hash = get_hash(key);
        size_t key_id = get_hash_id(key_hash);
        for (auto it = get_bucket_start(key_id); it != list_.cend(); ++it) {
            if (get_hash_id(it->key_hash_) != key_id) break;
            if (Equal{}((it->key_value_).first, key)) return const_iterator(it);
        }
        return cend();
    }

    std::pair<iterator, bool> insert(const NodeType& value) {
        std::pair<Key, Value> copy(value);
        return emplace(std::move(copy));
    }

    template<typename... Args>
    std::pair<iterator, bool> insert(Args&&... args) {
        return emplace(std::forward<Args>(args)...);
    }

    template<class InputIt>
    void insert(InputIt first, InputIt last) {
        for (auto it = first; it != last; ++it) {
            insert(*it);
        }
    }

    Value& at(const Key& key) {
        auto it = find(key);
        if (it == end()) {
            throw std::out_of_range("Wrong Key!");
        }
        return it->second;
    }

    const Value& at(const Key& key) const {
        auto it = find(key);
        if (it == cend()) {
            throw std::out_of_range("Wrong Key!");
        }
        return it->second;
    }

    Value& operator[](const Key& key) {
        auto it = find(key);
        if (it == end()) {
            it = emplace(key, Value()).first;
        }
        return it->second;
    }

    template<typename... Args>
    std::pair<iterator, bool> emplace(Args&&... args) {
        if (load_factor() > max_load_factor()) {
            reserve(2 * buckets_.size());
        }
        list_.emplace(list_.end(), std::forward<Args>(args)...);
        auto iter = --list_.end();
        auto it = find((iter->key_value_).first);
        if (it != end() && it != --end()) {
            list_.pop_back();
            return {it, false};
        }
        size_t key_id = get_hash_id(iter->key_hash_);
        if (get_bucket_start(key_id) == list_.end()) buckets_cnt_++;
        list_.link_iterators(--get_bucket_start(key_id), iter);
        buckets_[key_id] = iter;
        return {iterator(buckets_[key_id]), true};
    }

    void erase(iterator it) {
        size_t key_id = get_hash_id(it.get_iter()->key_hash_);
        if (it == iterator(get_bucket_start(key_id))) {
            list_.erase((it++).get_iter());
            if (it != end() && key_id == get_hash_id(it.get_iter()->key_hash_)) {
                buckets_[key_id] = it.get_iter();
            } else {
                buckets_[key_id] = nullptr;
                buckets_cnt_--;
            }
        } else {
            list_.erase(it.get_iter());
        }
    }

    void erase(iterator it_left, iterator it_right) {
        for (auto it = it_left; it != it_right; ) {
            erase(it++);
        }
    }
};
