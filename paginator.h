#pragma once

template <typename Iterator>
class IteratorRange {
public:
    IteratorRange(Iterator begin, Iterator end)
        : first_(begin)
        , last_(end)
        , size_(distance(first_, last_)) {
    }
    Iterator begin() const {
        return first_;
    }
    Iterator end() const {
        return last_;
    }
    size_t size() const {
        return size_;
    }
private:
    Iterator first_; //Насчет объявления нескольких переменной в одной строке - столкнулся с таким написанием в авторском
    Iterator last_;  //решении, подумал, что стоит также привыкнуть и к такому формату, чтобы не путаться в будущем.
    size_t size_;    //Такая же тема есть в Lua, там даже присвоение можно делать между группами переменных в одной строке,
};                   //что выглядит довольно интересно.

template <typename Iterator>
ostream& operator<<(ostream& out, const IteratorRange<Iterator>& range) {
    for (Iterator it = range.begin(); it != range.end(); ++it) {
        out << *it;
    }
    return out;
}

template <typename Iterator>
class Paginator {
public:
    Paginator(Iterator begin, Iterator end, size_t page_size) {
        for (size_t left = distance(begin, end); left > 0;) {
            const size_t current_page_size = min(page_size, left);
            const Iterator current_page_end = next(begin, current_page_size);
            pages_.push_back({begin, current_page_end});
            left -= current_page_size;
            begin = current_page_end;
        }
    }
    auto begin() const {
        return pages_.begin();
    }
    auto end() const {
        return pages_.end();
    }
    size_t size() const {
        return pages_.size();
    }
private:
    vector<IteratorRange<Iterator>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}