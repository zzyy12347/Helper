#include "DataStore.h"

#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

DataStore::DataStore(const QString &filePath)
    : m_filePath(filePath)
{
}

bool DataStore::load()
{
    QFile file(m_filePath);
    if (!file.exists()) {
        seedDefaults();
        return save();
    }

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const auto document = QJsonDocument::fromJson(file.readAll());
    if (!document.isObject()) {
        seedDefaults();
        return save();
    }

    m_items.clear();
    m_secondCategoriesByFirst.clear();

    const auto groupedSecondCategories = document.object().value("secondCategoriesByFirst").toObject();
    for (auto it = groupedSecondCategories.begin(); it != groupedSecondCategories.end(); ++it) {
        QStringList categories;
        for (const auto &value : it.value().toArray()) {
            const auto category = value.toString().trimmed();
            if (!category.isEmpty() && !categories.contains(category)) {
                categories.push_back(category);
            }
        }
        if (!it.key().trimmed().isEmpty()) {
            m_secondCategoriesByFirst.insert(it.key(), categories);
        }
    }

    if (m_secondCategoriesByFirst.isEmpty()) {
        QStringList legacyCategories;
        const auto secondCategories = document.object().value("secondCategories").toArray();
        for (const auto &value : secondCategories) {
            const auto category = value.toString().trimmed();
            if (!category.isEmpty() && !legacyCategories.contains(category)) {
                legacyCategories.push_back(category);
            }
        }
        if (!legacyCategories.isEmpty()) {
            m_secondCategoriesByFirst.insert(QStringLiteral("召唤物"), legacyCategories);
        }
    }

    const auto items = document.object().value("items").toArray();
    for (const auto &value : items) {
        if (value.isObject()) {
            m_items.push_back(itemFromJson(value.toObject()));
        }
    }

    if (m_items.isEmpty()) {
        seedDefaults();
        return save();
    }
    if (m_secondCategoriesByFirst.isEmpty()) {
        m_secondCategoriesByFirst = {
            {QStringLiteral("召唤物"), {QStringLiteral("地府"), QStringLiteral("中级")}},
            {QStringLiteral("药品/烹饪"), {}},
            {QStringLiteral("家具"), {}},
        };
    }

    return true;
}

bool DataStore::save() const
{
    QFileInfo info(m_filePath);
    QDir().mkpath(info.absolutePath());

    QJsonArray items;
    for (const auto &item : m_items) {
        items.append(itemToJson(item));
    }

    QJsonObject root;
    root.insert("items", items);

    QJsonObject groupedSecondCategories;
    for (auto it = m_secondCategoriesByFirst.begin(); it != m_secondCategoriesByFirst.end(); ++it) {
        QJsonArray categories;
        for (const auto &category : it.value()) {
            categories.append(category);
        }
        groupedSecondCategories.insert(it.key(), categories);
    }
    root.insert("secondCategoriesByFirst", groupedSecondCategories);

    QFile file(m_filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(root).toJson(QJsonDocument::Indented));
    return true;
}

QVector<ItemRecord> &DataStore::items()
{
    return m_items;
}

const QVector<ItemRecord> &DataStore::items() const
{
    return m_items;
}

QMap<QString, QStringList> &DataStore::secondCategoriesByFirst()
{
    return m_secondCategoriesByFirst;
}

const QMap<QString, QStringList> &DataStore::secondCategoriesByFirst() const
{
    return m_secondCategoriesByFirst;
}

QStringList DataStore::categoriesAtLevel(int level, const QStringList &prefix) const
{
    QSet<QString> values;
    for (const auto &item : m_items) {
        for (const auto &path : item.categoryPaths) {
            if (path.size() <= level || prefix.size() > level) {
                continue;
            }

            bool matches = true;
            for (int i = 0; i < prefix.size(); ++i) {
                if (path.value(i) != prefix.at(i)) {
                    matches = false;
                    break;
                }
            }

            if (matches) {
                values.insert(path.at(level));
            }
        }
    }

    auto result = values.values();
    result.sort(Qt::CaseInsensitive);
    return result;
}

QStringList DataStore::itemNamesForPrefix(const QStringList &prefix) const
{
    QSet<QString> values;
    for (const auto &item : m_items) {
        for (const auto &path : item.categoryPaths) {
            if (prefix.size() > path.size()) {
                continue;
            }

            bool matches = true;
            for (int i = 0; i < prefix.size(); ++i) {
                if (path.value(i) != prefix.at(i)) {
                    matches = false;
                    break;
                }
            }

            if (matches) {
                values.insert(item.name);
                break;
            }
        }
    }

    auto result = values.values();
    result.sort(Qt::CaseInsensitive);
    return result;
}

QVector<ItemRecord> DataStore::search(const QStringList &selectedCategories) const
{
    QVector<ItemRecord> results;
    for (const auto &item : m_items) {
        bool itemMatches = false;
        for (const auto &path : item.categoryPaths) {
            bool pathMatches = true;
            for (int i = 0; i < selectedCategories.size(); ++i) {
                if (selectedCategories.at(i).isEmpty()) {
                    continue;
                }
                if (i >= path.size() || path.at(i) != selectedCategories.at(i)) {
                    pathMatches = false;
                    break;
                }
            }

            if (pathMatches) {
                itemMatches = true;
                break;
            }
        }

        if (itemMatches) {
            results.push_back(item);
        }
    }
    return results;
}

void DataStore::addOrUpdateItem(const ItemRecord &item)
{
    for (auto &existing : m_items) {
        if (existing.name.compare(item.name, Qt::CaseInsensitive) == 0) {
            existing.name = item.name;
            if (!item.categoryPaths.isEmpty()) {
                existing.categoryPaths = item.categoryPaths;
            }

            for (const auto &incomingOffer : item.offers) {
                bool updated = false;
                for (auto &existingOffer : existing.offers) {
                    if (existingOffer.shopId == incomingOffer.shopId) {
                        existingOffer = incomingOffer;
                        updated = true;
                        break;
                    }
                }

                if (!updated) {
                    existing.offers.push_back(incomingOffer);
                }
            }
            return;
        }
    }
    m_items.push_back(item);
}

bool DataStore::removeItem(const QString &name)
{
    for (int i = 0; i < m_items.size(); ++i) {
        if (m_items.at(i).name == name) {
            m_items.removeAt(i);
            return true;
        }
    }
    return false;
}

bool DataStore::removeOffer(const QString &itemName, const QString &shopId)
{
    for (int itemIndex = 0; itemIndex < m_items.size(); ++itemIndex) {
        auto &item = m_items[itemIndex];
        if (item.name.compare(itemName, Qt::CaseInsensitive) != 0) {
            continue;
        }

        for (int offerIndex = 0; offerIndex < item.offers.size(); ++offerIndex) {
            if (item.offers.at(offerIndex).shopId != shopId) {
                continue;
            }

            item.offers.removeAt(offerIndex);
            if (item.offers.isEmpty()) {
                m_items.removeAt(itemIndex);
            }
            return true;
        }
    }
    return false;
}

bool DataStore::setOfferStock(const QString &itemName, const QString &shopId, bool outOfStock)
{
    for (auto &item : m_items) {
        if (item.name != itemName) {
            continue;
        }
        for (auto &offer : item.offers) {
            if (offer.shopId == shopId) {
                if (outOfStock && !offer.outOfStock) {
                    offer.outOfStockCount += 1;
                    offer.outOfStockMarkedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
                }
                if (!outOfStock) {
                    offer.outOfStockMarkedAt.clear();
                }
                offer.outOfStock = outOfStock;
                return true;
            }
        }
    }
    return false;
}

bool DataStore::refreshExpiredStock()
{
    bool changed = false;
    const auto now = QDateTime::currentDateTimeUtc();

    for (auto &item : m_items) {
        for (auto &offer : item.offers) {
            if (!offer.outOfStock || offer.outOfStockCount <= 0 || offer.outOfStockMarkedAt.isEmpty()) {
                continue;
            }

            const auto markedAt = QDateTime::fromString(offer.outOfStockMarkedAt, Qt::ISODate);
            if (!markedAt.isValid()) {
                continue;
            }

            const auto hoursToWait = qint64(offer.outOfStockCount) * 4;
            if (markedAt.secsTo(now) >= hoursToWait * 60 * 60) {
                offer.outOfStock = false;
                offer.outOfStockMarkedAt.clear();
                changed = true;
            }
        }
    }

    return changed;
}

ItemRecord DataStore::itemFromJson(const QJsonObject &object)
{
    ItemRecord item;
    item.name = object.value("name").toString();

    const auto categoryPaths = object.value("categoryPaths").toArray();
    for (const auto &pathValue : categoryPaths) {
        QStringList path;
        for (const auto &categoryValue : pathValue.toArray()) {
            const auto category = categoryValue.toString().trimmed();
            if (!category.isEmpty()) {
                path.push_back(category);
            }
        }
        if (!path.isEmpty()) {
            item.categoryPaths.push_back(path);
        }
    }

    if (item.categoryPaths.isEmpty()) {
        QStringList legacyPath;
        const auto categories = object.value("categories").toArray();
        for (const auto &value : categories) {
            const auto category = value.toString().trimmed();
            if (!category.isEmpty()) {
                legacyPath.push_back(category);
            }
        }
        if (!legacyPath.isEmpty()) {
            item.categoryPaths.push_back(legacyPath);
        }
    }

    const auto offers = object.value("offers").toArray();
    for (const auto &value : offers) {
        const auto offerObject = value.toObject();
        ShopOffer offer;
        offer.shopId = offerObject.value("shopId").toString();
        offer.showcaseId = offerObject.value("showcaseId").toString();
        offer.price = offerObject.value("price").toDouble();
        offer.outOfStock = offerObject.value("outOfStock").toBool(false);
        offer.outOfStockCount = offerObject.value("outOfStockCount").toInt(0);
        offer.outOfStockMarkedAt = offerObject.value("outOfStockMarkedAt").toString();
        item.offers.push_back(offer);
    }

    return item;
}

QJsonObject DataStore::itemToJson(const ItemRecord &item)
{
    QJsonObject object;
    object.insert("name", item.name);

    QJsonArray categoryPaths;
    for (const auto &path : item.categoryPaths) {
        QJsonArray pathArray;
        for (const auto &category : path) {
            pathArray.append(category);
        }
        categoryPaths.append(pathArray);
    }
    object.insert("categoryPaths", categoryPaths);

    QJsonArray legacyCategories;
    if (!item.categoryPaths.isEmpty()) {
        for (const auto &category : item.categoryPaths.first()) {
            legacyCategories.append(category);
        }
    }
    object.insert("categories", legacyCategories);

    QJsonArray offers;
    for (const auto &offer : item.offers) {
        QJsonObject offerObject;
        offerObject.insert("shopId", offer.shopId);
        offerObject.insert("showcaseId", offer.showcaseId);
        offerObject.insert("price", offer.price);
        offerObject.insert("outOfStock", offer.outOfStock);
        offerObject.insert("outOfStockCount", offer.outOfStockCount);
        offerObject.insert("outOfStockMarkedAt", offer.outOfStockMarkedAt);
        offers.append(offerObject);
    }
    object.insert("offers", offers);

    return object;
}

void DataStore::seedDefaults()
{
    m_items.clear();
    m_secondCategoriesByFirst = {
        {QStringLiteral("召唤物"), {QStringLiteral("地府"), QStringLiteral("中级")}},
        {QStringLiteral("药品/烹饪"), {}},
        {QStringLiteral("家具"), {}},
    };

    ItemRecord zombie;
    zombie.name = QStringLiteral("僵尸");
    zombie.categoryPaths = {
        {QStringLiteral("召唤物"), QStringLiteral("地府")},
        {QStringLiteral("召唤物"), QStringLiteral("中级")},
    };
    zombie.offers = {
        {QStringLiteral("1001"), QStringLiteral("1"), 88.0, false, 0, {}},
        {QStringLiteral("1002"), QStringLiteral("2"), 76.0, false, 0, {}},
        {QStringLiteral("1003"), QStringLiteral("3"), 70.0, false, 0, {}},
    };

    ItemRecord ghost;
    ghost.name = QStringLiteral("幽灵");
    ghost.categoryPaths = {{QStringLiteral("召唤物"), QStringLiteral("地府")}};
    ghost.offers = {
        {QStringLiteral("2001"), QStringLiteral("1"), 66.0, false, 0, {}},
        {QStringLiteral("2002"), QStringLiteral("2"), 59.0, false, 0, {}},
    };

    m_items = {zombie, ghost};
}
