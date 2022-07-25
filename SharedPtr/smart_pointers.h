#include <memory>

struct BaseControlBlock {
    size_t shared_count;
    size_t weak_count;

    virtual void* get_ptr() = 0;
    virtual void deallocate() = 0;
    virtual void destroy() = 0;

    BaseControlBlock(size_t shared_count, size_t weak_count) : shared_count(shared_count), weak_count(weak_count) {}

    virtual ~BaseControlBlock() = default;
};

template<typename T, typename Deleter, typename Alloc>
struct ControlBlockDirect: BaseControlBlock {
    using AllocTraits = std::allocator_traits<Alloc>;
    using ControlBlockAlloc = typename AllocTraits::template rebind_alloc<ControlBlockDirect>;
    using ControlBlockAllocTraits = typename AllocTraits::template rebind_traits<ControlBlockDirect>;

    T* ptr;
    Deleter deleter;
    ControlBlockAlloc alloc;

    void* get_ptr() override {
        return ptr;
    }

    void deallocate() override {
        ControlBlockAllocTraits::deallocate(alloc, this, 1);
    }

    void destroy() override {
        if (ptr) {
            deleter(ptr);
            ptr = nullptr;
        }
    }

    ControlBlockDirect(T* ptr, Deleter deleter, Alloc alloc) : BaseControlBlock(1, 0), ptr(ptr), deleter(deleter), alloc(alloc) {}

    ~ControlBlockDirect() = default;
};

template<typename T, typename Alloc>
struct ControlBlockMakeShared: BaseControlBlock {
    using AllocTraits = std::allocator_traits<Alloc>;
    using ControlBlockAlloc = typename AllocTraits::template rebind_alloc<ControlBlockMakeShared>;
    using ControlBlockAllocTraits = typename AllocTraits::template rebind_traits<ControlBlockMakeShared>;

    ControlBlockAlloc alloc;
    T obj;

    void* get_ptr() override {
        return &obj;
    }

    void deallocate() override {
        ControlBlockAllocTraits::deallocate(alloc, this, 1);
    } 

    void destroy() override {
        ControlBlockAllocTraits::destroy(alloc, &obj);
    }

    template<typename... Args>
    ControlBlockMakeShared(Alloc alloc, Args&&... args) : BaseControlBlock(1, 0), alloc(alloc), obj(std::forward<Args>(args)...) {}

    ~ControlBlockMakeShared() = default;
};

template<typename Base, typename Derived>
using is_derived = typename std::enable_if_t<std::is_base_of_v<Base, Derived> || std::is_same_v<Base, Derived>>;

template<class T>
class SharedPtr {
    template<class U>
    friend class WeakPtr;

    template<class U>
    friend class SharedPtr;

    template<typename U, typename Alloc, typename... Args>
    friend SharedPtr<U> allocateShared(Alloc alloc, Args&&... args);

private:
    BaseControlBlock* control_block_ = nullptr;

    SharedPtr(BaseControlBlock* control_block_) : control_block_(control_block_) {
       if (control_block_) ++(control_block_->shared_count);
    }
    
    template<typename Alloc, typename... Args>
    SharedPtr(Alloc alloc, Args&&... args) {
        using AllocTraits = std::allocator_traits<Alloc>;
        using ControlBlockAlloc = typename AllocTraits::template rebind_alloc<ControlBlockMakeShared<T, Alloc>>;
        using ControlBlockAllocTraits = typename AllocTraits::template rebind_traits<ControlBlockMakeShared<T, Alloc>>;

        ControlBlockAlloc new_alloc = alloc;
        auto tmp = ControlBlockAllocTraits::allocate(new_alloc, 1);
        ControlBlockAllocTraits::construct(new_alloc, tmp, new_alloc, std::forward<Args>(args)...);
        control_block_ = tmp;
    }

public:
    SharedPtr() = default;

    template<typename Y, typename Deleter = std::default_delete<Y>, typename Alloc = std::allocator<Y>, typename Tmp = is_derived<T, Y>>
    SharedPtr(Y* ptr, Deleter deleter = Deleter(), Alloc allocator = Alloc()) {
        using AllocTraits = std::allocator_traits<Alloc>;
        using ControlBlockAlloc = typename AllocTraits::template rebind_alloc<ControlBlockDirect<Y, Deleter, Alloc>>;
        using ControlBlockAllocTraits = typename AllocTraits::template rebind_traits<ControlBlockDirect<Y, Deleter, Alloc>>;

        ControlBlockAlloc new_alloc = allocator;
        control_block_ = ControlBlockAllocTraits::allocate(new_alloc, 1);
        new (control_block_) ControlBlockDirect<Y, Deleter, Alloc>(ptr, deleter, new_alloc);
    }

    SharedPtr(const SharedPtr<T>& other) : SharedPtr(other.control_block_) {}

    template<typename Y, typename Tmp = is_derived<T, Y>>
    SharedPtr(const SharedPtr<Y>& other) : SharedPtr(other.control_block_) {}

    SharedPtr(SharedPtr<T>&& other) : control_block_(other.control_block_) {
        other.control_block_ = nullptr;
    }

    template<typename Y, typename Tmp = is_derived<T, Y>>
    SharedPtr(SharedPtr<Y>&& other) : control_block_(other.control_block_) {
        other.control_block_ = nullptr;
    }

    template<typename Y, typename Tmp = is_derived<T, Y>>
    void swap(SharedPtr<Y>& other) {
        std::swap(control_block_, other.control_block_);
    }

    SharedPtr& operator=(const SharedPtr<T>& other) {
        SharedPtr tmp = other;
        swap(tmp);
        return *this;
    }

    template<typename Y, typename Tmp = is_derived<T, Y>>
    SharedPtr& operator=(const SharedPtr<Y>& other) {
        SharedPtr tmp = other;
        swap(tmp);
        return *this;
    }

    SharedPtr& operator=(SharedPtr<T>&& other) {
        auto tmp = std::move(other);
        swap(tmp);
        return *this;
    }

    template<typename Y, typename Tmp = is_derived<T, Y>>
    SharedPtr& operator=(SharedPtr<Y>&& other) {
        auto tmp(std::forward<SharedPtr<Y>>(other));
        swap(tmp);
        return *this;
    }

    T& operator*() const {
        return *(reinterpret_cast<T*>(control_block_->get_ptr()));
    }

    T* operator->() const {
        return reinterpret_cast<T*>(control_block_->get_ptr());
    }

    size_t use_count() const {
        return control_block_->shared_count;
    }

    void reset() {
        *this = SharedPtr<T>();
    }

    template<typename Y, typename Tmp = is_derived<T, Y>>
    void reset(Y* ptr) {
        *this = ptr;
    }

    T* get() const {
        if (control_block_) return reinterpret_cast<T*>(control_block_->get_ptr());
        return nullptr;
    }

    ~SharedPtr() {
        if (!control_block_) return;
        --(control_block_->shared_count);
        if (control_block_->shared_count == 0 && control_block_->weak_count == 0) {
            control_block_->destroy();
            control_block_->deallocate();
        } else if (control_block_->shared_count == 0) {
            control_block_->destroy();
        }
    }
};

template<typename T, typename Alloc, typename... Args>
SharedPtr<T> allocateShared(Alloc alloc, Args&&... args) {
    return SharedPtr<T>(alloc, std::forward<Args>(args)...);
}

template<typename T, typename... Args>
SharedPtr<T> makeShared(Args&&... args) {
    return allocateShared<T>(std::allocator<T>(), std::forward<Args>(args)...);
}

template<class T>
class WeakPtr {
private:
    template<class U>
    friend class WeakPtr;

private:
    BaseControlBlock* control_block_ = nullptr;

    WeakPtr(BaseControlBlock* other_cb) : control_block_(other_cb) {
        if (control_block_) ++(control_block_->weak_count);
    }

public:
    WeakPtr() = default;

    WeakPtr(const SharedPtr<T>& other) : WeakPtr(other.control_block_) {}

    template<typename U, typename Tmp = is_derived<T, U>>
    WeakPtr(const SharedPtr<U>& other) : WeakPtr(other.control_block_) {}

    WeakPtr(const WeakPtr<T>& other) : WeakPtr(other.control_block_) {}

    template<typename U, typename Tmp = is_derived<T, U>>
    WeakPtr(const WeakPtr<U>& other) : WeakPtr(other.control_block_) {}

    WeakPtr(WeakPtr<T>&& other) : control_block_(other.control_block_) {
        other.control_block_ = nullptr;
    }

    template<typename U, typename Tmp = is_derived<T, U>>
    WeakPtr(WeakPtr<U>&& other) : control_block_(other.control_block_) {
        other.control_block_ = nullptr;
    }

    template<typename U, typename Tmp = is_derived<T, U>>
    void swap(WeakPtr<U>& other) {
        std::swap(control_block_, other.control_block_);
    }

    WeakPtr& operator=(const WeakPtr<T>& other) {
        WeakPtr tmp = other;
        swap(tmp);
        return *this;
    }

    template<typename U, typename Tmp = is_derived<T, U>>
    WeakPtr& operator=(const WeakPtr<U>& other) {
        WeakPtr tmp = other;
        swap(tmp);
        return *this;
    }

    WeakPtr& operator=(WeakPtr<T>&& other) {
        auto tmp = std::move(other);
        swap(tmp);
        return *this;
    }

    template<typename U, typename Tmp = is_derived<T, U>>
    WeakPtr& operator=(WeakPtr<U>&& other) {
        auto tmp = std::move(other);
        swap(tmp);
        return *this;
    }

    bool expired() const {
        return control_block_->shared_count == 0;
    }

    size_t use_count() const {
        return control_block_->shared_count;
    }

    SharedPtr<T> lock() const {
        if (expired()) return SharedPtr<T>();
        return SharedPtr<T>(control_block_);
    }

    ~WeakPtr() {
        if (control_block_) {
            --(control_block_->weak_count);
            if (control_block_->shared_count == 0 && control_block_->weak_count == 0) {
                control_block_->deallocate();
            }
        }
    }
};
