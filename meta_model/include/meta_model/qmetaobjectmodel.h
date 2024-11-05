#pragma once

// workaround for QTBUG-83160
#if defined(Q_MOC_RUN)
#    define __cplusplus 202002
#endif

#include <ranges>
#include <vector>
#include <set>

#include <QtCore/QAbstractListModel>
#include <QtCore/QMetaProperty>

#include "core/core.h"

namespace meta_model
{

struct Empty {
    Q_GADGET
};

void update_role_names(QHash<int, QByteArray>& role_names, const QMetaObject& meta);

template<typename TBase>
class QMetaModelBase : public TBase {
public:
    QMetaModelBase(QObject* parent = nullptr): TBase(parent), m_meta(Empty::staticMetaObject) {}
    ~QMetaModelBase() {}
    QHash<int, QByteArray> roleNames() const override { return m_role_names; }
    const QMetaObject&     meta() const { return m_meta; }
    auto                   roleOf(QByteArrayView name) const -> int {
        if (auto it = std::find_if(roleNamesRef().keyValueBegin(),
                                   roleNamesRef().keyValueEnd(),
                                   [name](const auto& el) -> bool {
                                       return el.second == name;
                                   });
            it != roleNamesRef().keyValueEnd()) {
            return it->first;
        }
        return -1;
    }

protected:
    void updateRoleNames(const QMetaObject& meta) {
        this->layoutAboutToBeChanged();
        m_meta = meta;
        meta_model::update_role_names(m_role_names, m_meta);
        this->layoutChanged();
    }
    const QHash<int, QByteArray>& roleNamesRef() const { return m_role_names; }
    auto                          propertyOfRole(int role) const -> std::optional<QMetaProperty> {
        if (auto prop_idx = meta().indexOfProperty(roleNamesRef().value(role).constData());
            prop_idx != -1) {
            return meta().property(prop_idx);
        }
        return std::nullopt;
    }

private:
    QHash<int, QByteArray> m_role_names;
    QMetaObject            m_meta;
};

class QMetaListModelBase : public QMetaModelBase<QAbstractListModel> {
    Q_OBJECT
public:
    QMetaListModelBase(QObject* parent = nullptr);
    virtual ~QMetaListModelBase();

    Q_INVOKABLE virtual QVariant item(int index) const = 0;
};

template<typename TItem, typename IMPL>
class QMetaListModelPre : public QMetaListModelBase {
public:
    using value_type = TItem;
    using param_type = std::conditional_t<std::is_pointer_v<TItem>, TItem, const TItem&>;

    QMetaListModelPre(QObject* parent = nullptr): QMetaListModelBase(parent) {};
    virtual ~QMetaListModelPre() {};

    void insert(int index, param_type item) { insert(index, std::array { item }); }
    template<typename T>
        requires std::ranges::sized_range<T>
    // std::same_as<std::decay_t<typename T::value_type>, TItem>
    void insert(int index, const T& range) {
        auto size = range.size();
        if (size < 1) return;
        beginInsertRows({}, index, index + size - 1);
        auto begin = std::begin(range);
        auto end   = std::end(range);
        if constexpr (std::same_as<decltype(begin), decltype(end)>) {
            crtp_impl().insert_impl(index, begin, end);
        } else {
            crtp_impl().insert_impl(index, range);
        }
        endInsertRows();
    }

    void update(int idx, const value_type& val) {
        auto& item = crtp_impl().at(idx);
        item       = val;
        dataChanged(index(idx, 0), index(idx, 0));
    }

    void remove(int index, int size = 1) {
        if (size < 1) return;
        removeRows(index, size);
    }
    auto removeRows(int row, int count, const QModelIndex& parent = {}) -> bool override {
        if (count < 1) return false;
        beginRemoveRows(parent, row, row + count - 1);
        crtp_impl().erase_impl(row, row + count);
        endRemoveRows();
        return true;
    }
    template<typename Func>
    void remove_if(Func&& func) {
        std::set<int, std::greater<>> indexes;
        for (int i = 0; i < rowCount(); i++) {
            auto& el = crtp_impl().at(i);
            if (func(el)) {
                indexes.insert(i);
            }
        }
        for (auto& i : indexes) {
            removeRow(i);
        }
    }
    void replace(int row, param_type item) {
        crtp_impl().assign(row, item);
        auto idx = index(row);
        dataChanged(idx, idx);
    }

    void resetModel() {
        beginResetModel();
        crtp_impl().reset_impl();
        endResetModel();
    }

    template<typename T>
        requires std::ranges::sized_range<T>
    void resetModel(const std::optional<T>& items) {
        beginResetModel();
        if (items) {
            crtp_impl().reset_impl(items.value());
        } else {
            crtp_impl().reset_impl();
        }
        endResetModel();
    }

    template<typename T>
        requires std::ranges::sized_range<T>
    // std::same_as<std::decay_t<typename T::value_type>, TItem>
    void resetModel(const T& items) {
        beginResetModel();
        crtp_impl().reset_impl(items);
        endResetModel();
    }

    QVariant item(int idx) const override {
        if ((usize)std::max(idx, 0) >= crtp_impl().size()) return {};
        if constexpr (ycore::is_specialization_of_v<value_type, std::variant>) {
            return std::visit(
                [](const auto& v) -> QVariant {
                    return QVariant::fromValue(v);
                },
                crtp_impl().at(idx));
        } else {
            return QVariant::fromValue(crtp_impl().at(idx));
        }
    }
    virtual int rowCount(const QModelIndex& = QModelIndex()) const override {
        return crtp_impl().size();
    }

    template<typename T, typename IdFn>
        requires std::ranges::sized_range<T> &&
                 std::same_as<std::decay_t<typename T::value_type>, TItem> &&
                 std::convertible_to<std::invoke_result_t<IdFn, TItem>, std::string>
    void convertModel(const T& items, IdFn&& id_fn) {
        std::set<std::string> id_set;
        for (auto& el : items) {
            id_set.insert(id_fn(el));
        }

        for (std::size_t i = 0; i < crtp_impl().size();) {
            auto id = id_fn(crtp_impl().at(i));
            if (id_set.contains(id))
                i++;
            else
                remove(i);
        }

        for (std::size_t i = 0; i < items.size(); i++) {
            auto in = items[i];
            if ((std::size_t)rowCount() == i) {
                insert(i, in);
            } else {
                auto& out = crtp_impl().at(i);
                if (id_fn(in) != id_fn(out))
                    insert(i, in);
                else if (in != out)
                    replace(i, in);
            }
        }

        for (std::size_t i = items.size(); i < crtp_impl().size(); i++) {
            remove(i);
        }
    }

private:
    auto&       crtp_impl() { return *static_cast<IMPL*>(this); }
    const auto& crtp_impl() const { return *static_cast<const IMPL*>(this); }
};

template<typename TItem, typename CRTP, typename Allocator = std::allocator<TItem>>
class QMetaListModel : public QMetaListModelPre<TItem, CRTP> {
    friend class QMetaListModelPre<TItem, CRTP>;

public:
    using base_type      = QMetaListModelPre<TItem, CRTP>;
    using allocator_type = Allocator;
    using container_type = std::vector<TItem>;
    using iterator       = container_type::iterator;
    using param_type     = base_type::param_type;

    QMetaListModel(QObject* parent = nullptr, Allocator allc = Allocator())
        : base_type(parent), m_items(allc) {}
    virtual ~QMetaListModel() {}

    auto        begin() const { return std::begin(m_items); }
    auto        end() const { return std::end(m_items); }
    auto        begin() { return std::begin(m_items); }
    auto        end() { return std::end(m_items); }
    auto        size() const { return std::size(m_items); }
    const auto& at(std::size_t idx) const { return m_items.at(idx); }
    auto&       at(std::size_t idx) { return m_items.at(idx); }
    void        assign(std::size_t idx, const TItem& t) { m_items.at(idx) = t; }
    auto        find(param_type t) const { return std::find(begin(), end(), t); }
    auto        find(param_type t) { return std::find(begin(), end(), t); }

protected:
    template<typename Tin>
    void insert_impl(std::size_t it, Tin beg, Tin end) {
        m_items.insert(begin() + it, beg, end);
    }

    template<typename TRange>
    void insert_impl(std::size_t it, const TRange& range) {
        std::vector<TItem, Allocator> tmp(m_items.get_allocator());
        tmp.reserve(range.size());
        std::ranges::copy(range, std::back_inserter(tmp));
        m_items.insert(begin() + it, tmp.begin(), tmp.end());
    }

    void erase_impl(std::size_t index, std::size_t last) {
        auto it = m_items.begin();
        m_items.erase(it + index, it + last);
    }

    void reset_impl() { m_items.clear(); }

    template<typename T>
    void reset_impl(const T& items) {
        m_items.clear();
        insert_impl(0, std::begin(items), std::end(items));
    }

private:
    std::vector<TItem, Allocator> m_items;
};

} // namespace meta_model