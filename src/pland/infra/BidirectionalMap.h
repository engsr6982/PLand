#pragma once
#include <unordered_map>
#include <unordered_set>

namespace land {

/**
 * @brief 双向映射表
 * @tparam K 键
 * @tparam V 值
 */
template <typename K, typename V>
class BidirectionalMap {
    using LeftMap  = std::unordered_map<K, std::unordered_set<V>>;
    using RightMap = std::unordered_map<V, std::unordered_set<K>>;

    LeftMap  mLeft;
    RightMap mRight;

public:
    BidirectionalMap() = default;

    /**
     * @brief 访问正向表
     */
    std::unordered_set<V>& operator[](K const& key) { return mLeft[key]; }
    /**
     * @brief 访问反向表
     */
    std::unordered_set<K>& operator()(V const& value) { return mRight[value]; }

    std::unordered_set<V> const& at(K const& key) const { return mLeft.at(key); }
    std::unordered_set<K> const& at(V const& value) const { return mRight.at(value); }

    /**
     * @brief 插入键值对到双向表
     */
    void insert(K const& key, V const& value) {
        mLeft[key].insert(value);
        mRight[value].insert(key);
    }

    void erase_value(K const& key, V const& value) {
        if (auto it = mLeft.find(key); it != mLeft.end()) {
            it->second.erase(value); // 删除正向表容器里的值
            if (it->second.empty()) mLeft.erase(it);
        }
        if (auto it = mRight.find(value); it != mRight.end()) {
            it->second.erase(key); // 删除反向表容器里的值
            if (it->second.empty()) mRight.erase(it);
        }
    }

    void erase_key(K const& key, V const& value) {
        mLeft.erase(key);
        mRight.erase(value);
    }

    bool contains(K const& key, V const& value) const {
        auto lit = mLeft.find(key);
        if (lit == mLeft.end()) return false;
        if (!lit->second.contains(value)) return false;

        auto rit = mRight.find(value);
        if (rit == mRight.end()) return false;
        return rit->second.contains(key);
    }

    bool has_left(K const& key) const { return mLeft.contains(key); }
    bool has_right(V const& val) const { return mRight.contains(val); }

    LeftMap const&  left() const { return mLeft; }
    RightMap const& right() const { return mRight; }
};


} // namespace land