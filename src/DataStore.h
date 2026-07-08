#pragma once

#include <QJsonArray>
#include <QMap>
#include <QString>
#include <QStringList>
#include <QVector>

struct ShopOffer {
    QString shopId;
    QString showcaseId;
    double price = 0.0;
    bool outOfStock = false;
    int outOfStockCount = 0;
    QString outOfStockMarkedAt;
};

struct ItemRecord {
    QString name;
    QVector<QStringList> categoryPaths;
    QVector<ShopOffer> offers;
};

class DataStore {
public:
    explicit DataStore(const QString &filePath);

    bool load();
    bool save() const;

    QVector<ItemRecord> &items();
    const QVector<ItemRecord> &items() const;
    QMap<QString, QStringList> &secondCategoriesByFirst();
    const QMap<QString, QStringList> &secondCategoriesByFirst() const;

    QStringList categoriesAtLevel(int level, const QStringList &prefix) const;
    QStringList itemNamesForPrefix(const QStringList &prefix) const;
    QVector<ItemRecord> search(const QStringList &selectedCategories) const;

    void addOrUpdateItem(const ItemRecord &item);
    bool removeItem(const QString &name);
    bool removeOffer(const QString &itemName, const QString &shopId);
    bool setOfferStock(const QString &itemName, const QString &shopId, bool outOfStock);
    bool refreshExpiredStock();

private:
    static ItemRecord itemFromJson(const QJsonObject &object);
    static QJsonObject itemToJson(const ItemRecord &item);
    void seedDefaults();

    QString m_filePath;
    QVector<ItemRecord> m_items;
    QMap<QString, QStringList> m_secondCategoriesByFirst;
};
