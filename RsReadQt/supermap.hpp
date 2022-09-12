#pragma once
#include <map>

template<typename _Key,
         typename _Tp,
         typename _Compare = std::less<_Key>,
         typename _Alloc   = std::allocator<std::pair<const _Key, _Tp>>>
struct superMap : public std::map<_Key, _Tp, _Compare, _Alloc> {
    using map = std::map<_Key, _Tp, _Compare, _Alloc>;

    template<typename... Args>
    superMap(Args &&...args)
      : std::map<_Key, _Tp, _Compare, _Alloc>(std::forward<Args>(args)...)
    { }

    superMap(const std::initializer_list<typename superMap::value_type> &_Ilist)
      : std::map<_Key, _Tp, _Compare, _Alloc>(std::forward<decltype(_Ilist)>(_Ilist))
    { }

    superMap(std::initializer_list<typename superMap::value_type> &&_Ilist)
      : std::map<_Key, _Tp, _Compare, _Alloc>(std::move(_Ilist))
    { }

    superMap() = default;

    size_t GetIndexByKeyIfAny(_Key k) const
    {
        auto found = this->find(k);
        if (found == this->end())
            return -1;   //-1

        return std::distance(this->begin(), found);
    }

    bool ConvToKeyByValueIfAny(_Tp value_tofind, _Key &dest) const
    {
        for (auto &[key, value_here] : *this) {
            if (value_tofind == value_here) {
                dest = key;
                return true;
            }
        }

        return false;
    }

    bool ConvToPairByKeyIfAny(_Key k, std::pair<_Key, _Tp> &dest) const
    {
        if (this->find(k) == this->end())
            return false;
        else {
            dest = std::pair{ k, map::at(k) };
            return true;
        }
    }

    bool ConvToPairByValueIfAny(const _Tp &value_tofind, std::pair<_Key, _Tp> &dest) const
    {
        for (const auto [key, value] : *this) {
            if (value == value_tofind) {
                dest = std::pair{ key, value };

                return true;
            }
        }

        return false;
    }

    bool ConvToPairByIdxIfAny(int idx, std::pair<_Key, _Tp> &dest) const
    {
        if (idx >= this->size()) {
            return false;
        }
        else {
            int iterator = 0;

            for (const auto &val : *this) {
                if (iterator == idx) {
                    dest = std::pair{ val.first, val.second };
                    return true;
                }

                iterator++;
            }

            return false;
        }
    }

    bool ContainsKey(_Key k) const noexcept
    {
        auto found = this->find(k);
        if (found == this->end())
            return false;
        else
            return true;
    }

    bool ContainsValue(_Tp value_tofind) const noexcept
    {
        return std::any_of(this->begin(), this->end(), [&value_tofind](const auto &pair) {
            return pair.second == value_tofind;
        });
    }

    std::vector<_Tp> GetAllValues() const noexcept
    {
        std::vector<_Tp> retvect;
        for (const auto &pair : *this) {
            retvect.push_back(this->second);
        }
    }
};