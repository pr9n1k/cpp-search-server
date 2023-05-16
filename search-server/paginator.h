#pragma once
#include <vector>

template<typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end) {
        it_begin = begin;
        it_end = end;
        size = std::distance(begin, end);
    }
    Iterator begin() {
        return it_begin;
    }
    Iterator end() {
        return it_end;
    }

    template<typename T>
    friend std::ostream& operator<<(std::ostream& out, IteratorRange<T> iterator);

private:
    Iterator it_begin;
    Iterator it_end;
    size_t size;
};


template<typename Iterator>
class Paginator {
public:
    Paginator(Iterator range_begin, Iterator range_end, const size_t& page_size) {
        size_t size_container = distance(range_begin, range_end);
        for (size_t i = 0; i < size_container; i += page_size) {
            Iterator current_begin = range_begin + i;
            Iterator current_end = current_begin > range_end - page_size ? range_end : current_begin + page_size;
            iterators_.push_back(IteratorRange(current_begin, current_end));
        }
    }
    typename std::vector<IteratorRange<Iterator>>::const_iterator begin() const {
        return iterators_.begin();
    }
    typename std::vector<IteratorRange<Iterator>>::const_iterator end() const {
        return iterators_.end();
    }
private:
    std::vector<IteratorRange<Iterator>> iterators_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template<typename T>
std::ostream& operator<<(std::ostream& out, IteratorRange<T> iterator) {
    for (T it = iterator.it_begin; it != iterator.it_end; ++it) {
        out << *it;
    }
    return out;
}