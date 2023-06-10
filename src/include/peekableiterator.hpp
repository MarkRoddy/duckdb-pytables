#include <deque>
#include <vector>

template<typename Iterator>
class PeekableIterator {
public:
  // Constructors
  PeekableIterator(const Iterator& begin, const Iterator& end)
    : current_(begin), end_(end) {}

  // Access current element
  typename Iterator::value_type operator*() {
    if (!buffer_.empty()) {
      return buffer_.front();
    } else {
      return *current_;
    }
  }

  // Move to the next element
  PeekableIterator& operator++() {
    if (!buffer_.empty()) {
      buffer_.pop_front();
    } else if (current_ != end_) {
      ++current_;
    }
    return *this;
  }

  // Check if two iterators are different
  friend bool operator!=(const PeekableIterator& a, const PeekableIterator& b) {
    return a.current_ != b.current_ || a.buffer_.size() != b.buffer_.size();
  }

  // Peek at the next n values
  std::vector<typename Iterator::value_type> peek(size_t n) {
    std::vector<typename Iterator::value_type> peeked_values;
    while (n > buffer_.size() && current_ != end_) {
      typename Iterator::value_type value = *current_++;
      buffer_.push_back(value);
      peeked_values.push_back(value);
    }
    return peeked_values;
  }

  PeekableIterator end() {
    return PeekableIterator(nullptr, end_);
  }
  
private:
  Iterator current_;
  Iterator end_;
  std::deque<typename Iterator::value_type> buffer_;
};
