#pragma once

#include <variant>
#include "meta_model/qmetaobjectmodel.h"

namespace meta_model
{

template<typename... Ts>
class VariantListModel : public QMetaListModelPre<std::variant<Ts...>, VariantListModel<Ts...>> {
    friend class QMetaListModelPre<std::variant<Ts...>, VariantListModel<Ts...>>;

public:
    VariantListModel(QObject* parent = nullptr)
        : QMetaListModelPre<std::variant<Ts...>, VariantListModel<Ts...>>(parent) {}
    virtual ~VariantListModel() {}

    using value_type     = std::variant<Ts...>;
    using container_type = std::variant<std::vector<Ts>...>;
    using iterator       = std::variant<typename std::vector<Ts>::iterator...>;

    // override
    virtual QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override {
        if (auto prop = this->propertyOfRole(role); prop) {
            QVariant var;
            std::visit(
                [&var, &index, &prop](auto& items) {
                    var = prop.value().readOnGadget(&items.at(index.row()));
                },
                m_items);
            return var;
        }
        return {};
    };

    auto begin() const {
        iterator it;
        std::visit(
            [&it](auto& var) {
                it = std::begin(var);
            },
            m_items);
        return it;
    }
    auto end() const {
        iterator it;
        std::visit(
            [&it](auto& var) {
                it = std::end(var);
            },
            m_items);
        return it;
    }
    auto begin() {
        iterator it;
        std::visit(
            [&it](auto& var) {
                it = std::begin(var);
            },
            m_items);
        return it;
    }
    auto end() {
        iterator it;
        std::visit(
            [&it](auto& var) {
                it = std::end(var);
            },
            m_items);
        return it;
    }
    auto size() const {
        std::size_t it;
        std::visit(
            [&it](auto& var) {
                it = std::size(var);
            },
            m_items);
        return it;
    }
    auto at(std::size_t idx) const { return m_items.at(idx); }
    void assign(std::size_t idx, value_type v) {
        std::visit(
            [this, idx](auto& var) {
                std::get<std::vector<std::decay_t<decltype(var)>>>(m_items).at(idx) = var;
            },
            v);
    }

private:
    template<typename Tin>
    void insert_impl(std::size_t idx, Tin beg, Tin end) {
        std::visit(
            [idx, beg, end](auto& items) {
                items.insert(std::begin(items + idx), beg, end);
            },
            m_items);
    }

    void erase_impl(std::size_t begin, std::size_t last) {
        std::visit(
            [this, begin, last](auto& v) {
                auto it = std::begin(v);
                v.erase(it + begin, it + last);
            },
            m_items);
    }

    template<typename T>
    void reset_impl(const T& items) {
        auto size = items.size();
        std::visit(
            [this, size](auto& v) {
                v.clear();
                v.resize(size);
            },
            m_items);
        insert_impl(0, std::begin(items), std::end(items));
    }

private:
    container_type m_items;
};

} // namespace meta_model