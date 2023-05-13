#include <algorithm>
#include <cassert>
#include <iostream>
#include <iterator>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

template <class T, class Allocator = std::allocator<T>>
class List {
 private:
  // base structures
  class Node {
   public:
    T value;
    Node* prev = nullptr;
    Node* next = nullptr;

    Node() : value(T()) {}

    Node(const T& value, Node* prev = nullptr, Node* next = nullptr)
        : prev(prev), next(next), value(value) {}

    Node(T&& value, Node* prev = nullptr, Node* next = nullptr)
        : prev(prev), next(next), value(std::move(value)) {}

    void swap(Node& other) {
      std::swap(*prev, *other.prev);
      std::swap(*next, *other.next);
      std::swap(prev->next, other.prev->next);
      std::swap(next->prev, other.next->prev);
      std::swap(value, other.value);
    }
  };

  class BaseNode {
   public:
    Node* base = nullptr;
  };

 public:
  // usings
  using value_type = T;
  using allocator_type = Allocator;
  using node_allocator_type =
      typename std::allocator_traits<Allocator>::template rebind_alloc<Node>;
  using node_allocator_traits =
      typename std::allocator_traits<node_allocator_type>;

 private:
  node_allocator_type node_alloc_;
  BaseNode root_;
  size_t size_ = 0;

 public:
  // iterator
  template <bool IsConst>
  class Iterator : public std::iterator<
                       std::bidirectional_iterator_tag,
                       typename std::conditional<IsConst, const T, T>::type> {
   private:
    Node* itptr_ = nullptr;

   public:
    typedef typename std::conditional<IsConst, const T, T>::type Ttype;
    using value_type = typename std::iterator<std::bidirectional_iterator_tag,
                                              Ttype>::value_type;
    using pointer =
        typename std::iterator<std::bidirectional_iterator_tag, Ttype>::pointer;
    using difference_type =
        typename std::iterator<std::bidirectional_iterator_tag,
                               Ttype>::difference_type;
    using reference = typename std::iterator<std::bidirectional_iterator_tag,
                                             Ttype>::reference;

    // constructors and destructor
    Iterator() = default;

    Iterator(Node* ptr) : itptr_(ptr){};

    Iterator(const Iterator<IsConst>& copy) : itptr_(copy.itptr_) {}

    ~Iterator() = default;

    // operators
    void operator=(const Iterator& copy) { itptr_ = copy.itptr_; }

    reference operator*() const { return itptr_->value; }

    pointer operator->() const { return &(itptr_->value); }

    Iterator<IsConst>& operator++() {
      itptr_ = itptr_->next;
      return *this;
    }

    Iterator<IsConst> operator++(int) {
      Iterator<IsConst> temp(*this);
      ++(*this);
      return temp;
    }

    Iterator<IsConst>& operator--() {
      itptr_ = itptr_->prev;
      return *this;
    }

    Iterator<IsConst> operator--(int) {
      Iterator<IsConst> temp(*this);
      --(*this);
      return temp;
    }

    bool operator==(const Iterator<IsConst>& other) const {
      return itptr_ == other.itptr_;
    }

    bool operator!=(const Iterator<IsConst>& other) const {
      return itptr_ != other.itptr_;
    }

    Node* get_ptr() { return itptr_; }
  };

  // methods
  void insert(Iterator<false> iter, const T& value) {
    Node* temp = construct_node(value);
    try {
      temp->next = iter.get_ptr();
      --iter;
      temp->prev = iter.get_ptr();
      temp->next->prev = temp;
      temp->prev->next = temp;
      ++size_;
    } catch (...) {
      throw;
    }
  }

  void insert(Iterator<false> iter) {
    Node* temp = node_allocator_traits::allocate(node_alloc_, 1);
    try {
      node_allocator_traits::construct(node_alloc_, temp);
    } catch (...) {
      node_allocator_traits::deallocate(node_alloc_, temp, 1);
      throw;
    }
    try {
      temp->next = iter.get_ptr();
      --iter;
      temp->prev = iter.get_ptr();
      temp->next->prev = temp;
      temp->prev->next = temp;
      ++size_;
    } catch (...) {
      throw;
    }
  }

  void erase(Iterator<false> iter) {
    Node* temp = iter.get_ptr();
    try {
      temp->next->prev = temp->prev;
      temp->prev->next = temp->next;
      node_allocator_traits::destroy(node_alloc_, temp);
      node_allocator_traits::deallocate(node_alloc_, temp, 1);
      --size_;
      if (empty()) {
        node_allocator_traits::deallocate(node_alloc_, root_.base, 1);
        root_.base = nullptr;
      }
    } catch (...) {
      throw;
    }
  }

  // usings for iterators
  using iterator = Iterator<false>;
  using const_iterator = Iterator<true>;
  using reverse_iterator = std::reverse_iterator<iterator>;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;

  iterator begin() { return iterator(root_.base->next); }

  iterator end() { return iterator(root_.base); }

  iterator begin() const { return iterator(root_.base->next); }

  iterator end() const { return iterator(root_.base); }

  const_iterator cbegin() { return const_iterator(root_.base->next); }

  const_iterator cend() { return const_iterator(root_.base); }

  const_iterator cbegin() const { return const_iterator(root_.base->next); }

  const_iterator cend() const { return const_iterator(root_.base); }

  reverse_iterator rbegin() { return reverse_iterator(end()); }

  reverse_iterator rend() { return reverse_iterator(begin()); }

  const_reverse_iterator crbegin() { return const_reverse_iterator(cend()); }

  const_reverse_iterator crend() { return const_reverse_iterator(cbegin()); }

 private:
  // base structure constructor/destructor
  Node* allocate_base_node() {
    Node* node = node_allocator_traits::allocate(node_alloc_, 1);
    node->prev = node;
    node->next = node;
    return node;
  }

  Node* construct_node(const T& value) {
    Node* node = node_allocator_traits::allocate(node_alloc_, 1);
    try {
      node_allocator_traits::construct(node_alloc_, node, value);
    } catch (...) {
      node_allocator_traits::deallocate(node_alloc_, node, 1);
      throw;
    }
    return node;
  }

  Node* construct_node(T&& value) {
    Node* node = node_allocator_traits::allocate(node_alloc_, 1);
    try {
      node_allocator_traits::construct(node_alloc_, node, std::move(value));
    } catch (...) {
      node_allocator_traits::deallocate(node_alloc_, node, 1);
    }
    return node;
  }

 public:
  void push_back(const T& value) {
    if (empty()) {
      root_.base = allocate_base_node();
      Node* temp = construct_node(value);
      temp->prev = root_.base;
      temp->prev->next = temp;
      temp->next = root_.base;
      temp->next->prev = temp;
      ++size_;
      return;
    }
    insert(end(), value);
  }

  void push_back(T&& value) {
    if (empty()) {
      root_.base = allocate_base_node();
      Node* temp = construct_node(std::move(value));
      temp->prev = root_.base;
      temp->prev->next = temp;
      temp->next = root_.base;
      temp->next->prev = temp;
      ++size_;
      return;
    }
    insert(end(), value);
  }

  void emplace_back() {
    if (empty()) {
      root_.base = allocate_base_node();
    }
    insert(end());
  }

  void push_front(const T& value) { insert(begin(), value); }

  void pop_back() {
    auto iter = end();
    --iter;
    erase(iter);
  }

  void pop_front() { erase(begin()); }

  void clear() {
    while (size_ > 0) {
      pop_front();
    }
  }

  // constructors
  explicit List(Allocator alloc = Allocator()) : node_alloc_(alloc) {}

  explicit List(size_t count, Allocator alloc = Allocator()) try
      : node_alloc_(alloc) {
    for (size_t i = 0; i < count; ++i) {
      try {
        emplace_back();
      } catch (...) {
        clear();
        throw;
      }
    }
  } catch (...) {
    throw;
  }

  List(std::initializer_list<T> init, const Allocator& alloc = Allocator()) try
      : node_alloc_(alloc) {
    auto iter = init.begin();
    while (iter != init.end()) {
      push_back(*(iter++));
    }
  } catch (...) {
    throw;
  }

  // ok
  List(size_t count, const T& value, const Allocator& alloc = Allocator()) try
      : node_alloc_(alloc) {
    for (size_t i = 0; i < count; ++i) {
      try {
        push_back(value);
      } catch (...) {
        clear();
        throw;
      }
    }
  } catch (...) {
    throw;
  }

  List(const List& copy) try
      : node_alloc_(
            std::allocator_traits<Allocator>::
                select_on_container_copy_construction(copy.node_alloc_)) {
    for (auto iter = copy.cbegin(); iter != copy.cend(); ++iter) {
      try {
        push_back(*iter);
      } catch (...) {
        clear();
        throw;
      }
    }
  } catch (...) {
    throw;
  }

  // destructor
  ~List() {
    while (size_ > 0) {
      pop_front();
    }
  }

  // operators
  List& operator=(const List& copy) {
    if (std::allocator_traits<
            Allocator>::propagate_on_container_copy_assignment::value) {
      List<T, Allocator> temp(copy.node_alloc_);
      try {
        for (auto iter = copy.begin(); iter != copy.end(); ++iter) {
          temp.push_back(*iter);
        }
        std::swap(node_alloc_, temp.node_alloc_);
        std::swap(size_, temp.size_);
        std::swap(root_, temp.root_);

      } catch (...) {
        while (temp.size() > 0) {
          temp.pop_back();
        }
        throw;
      }
      return *this;
    }
    size_t prev_size = size_;
    try {
      for (auto iter = copy.begin(); iter != copy.end(); ++iter) {
        push_back(*iter);
      }
      while (prev_size > 0) {
        pop_front();
        --prev_size;
      }
    } catch (...) {
      while (size_ > prev_size) {
        pop_back();
      }
      throw;
    };
    return *this;
  }

  // getters
  node_allocator_type get_allocator() const { return node_alloc_; }

  size_t size() const { return size_; }

  bool empty() const { return size_ == 0; }
};
