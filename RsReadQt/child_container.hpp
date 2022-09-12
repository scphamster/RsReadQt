#pragma once

// todo: add sorting
template<typename ChildT, size_t DimensionsInUse = 1>
class ChildContainer {
  public:
    ~ChildContainer()
    {
        for (auto child : children)
            delete child;
    }

    [[nodiscard]] auto GetChild(size_t position)
    {
        if (children.size() - 1 < position)
            return static_cast<ChildT *>(nullptr);

        return children.at(position);
    }

    template<typename T, typename FinderFunctor>
    [[nodiscard]] auto GetChildWithParam(const T &param, FinderFunctor finder)
    {
        auto child = std::find_if(children.begin(), children.end(), finder);

        if (child == children.end())
            return static_cast<ChildT *>(nullptr);
        else
            return *child;
    }

    // todo: make children be packed with no gaps, contiguously
    [[nodiscard]] auto InsertChild(size_t position, ChildT *newchild)
    {
        if (newchild == nullptr)
            return false;

        if (dimensionMaxPosition + 1 < position)
            position = dimensionMaxPosition + 1;

        dimensionMaxPosition++;
        children.resize(dimensionMaxPosition + 1);

        // push all children one row down after "position"
        if (children.size() > 1 and (position < children.size() - 1)) {
            auto insert_item_at     = children.rbegin() + (children.size() - 1 - position);
            auto new_child_position = children.size() - 1;
            auto one_child_ahead    = children.rbegin() + 1;

            std::for_each(children.rbegin(), insert_item_at, [this, &new_child_position, &one_child_ahead](auto &child) {
                child = *one_child_ahead;
                child->SetRow(new_child_position);
                new_child_position--;
                one_child_ahead++;
            });
        }

        children.at(position) = newchild;
        newchild->SetRow(position);

        return true;
    }

    [[nodiscard]] auto ChildAtPosExists(size_t at_pos) const noexcept { return children.contains(at_pos); }

    [[nodiscard]] auto GetChildrenCount() const noexcept { return children.size(); }

    [[nodiscard]] auto GetDoNotExceedDimensionsSizes(int dimension) const noexcept { return dimensionMaxPosition; }

    // template<std::predicate SortingFunctor>
    // void Sort(auto column, auto order, SortingFunctor sorter)
    //{
    //     std::sort(children.begin(), children.end(), sorter);
    // }

    template<typename SortingFunctor>
    decltype(auto) Sort(auto column, auto order, SortingFunctor sorter)
    {
        std::pair<QList<std::pair<int, ChildT *>>, QList<std::pair<int, ChildT *>>> changedModelIndexesFromTo{};

        for (const auto child : children) {
            changedModelIndexesFromTo.first << std::pair<int, ChildT *>{ child->GetRow(), child };
        }
        std::stable_sort(children.begin(), children.end(), sorter);

        for (size_t position{ 0 }; auto child : children) {
            child->SetRow(position);
            changedModelIndexesFromTo.second << std::pair<int, ChildT *>{ position, child };

            position++;
        }

        return changedModelIndexesFromTo;
    }

    template<typename T, typename EqComparatorFunctor>
    auto Contains(const T &wantedParam, EqComparatorFunctor comparator) const
    {
        return std::any_of(children.begin(), children.end(), comparator);
    }

    template<typename ValueT, typename FinderFunctor>
    auto FindChild(ValueT value, FinderFunctor finder)
    {
        auto child = std::find_if(children.begin(), children.end(), finder);
        if (child == children.end())
            return static_cast<ChildT *>(nullptr);

        return *child;
    }

  private:
    std::vector<ChildT *> children;
    size_t                dimensionMaxPosition = -1;
};

template<typename ChildT>
class ChildContainer<ChildT, 2> {
    [[nodiscard]] auto GetChild(size_t position_d1, size_t position_d2) { return 0; }

  private:
    std::map<size_t, std::map<size_t, ChildT *>> children;
    std::vector<size_t>                          dimensionSizes;
};
