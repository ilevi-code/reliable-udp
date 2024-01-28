#include <iostream>
#include <vector>
#include <iterator>
#include <algorithm>

template<typename T>
std::ostream& operator<< (std::ostream& out, const std::vector<T>& v) {
    std::cout << '{';
    std::copy(v.begin(), v.end(), std::ostream_iterator<int>(std::cout, ","));
    std::cout << '}';
    return out;
}
