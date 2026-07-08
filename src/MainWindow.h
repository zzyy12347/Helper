#pragma once

#include "DataStore.h"

#include <QMap>
#include <QMainWindow>

class QCheckBox;
class QComboBox;
class QDialog;
class QLabel;
class QLineEdit;
class QPushButton;
class QScrollArea;
class QTableWidget;
class QVBoxLayout;
class QWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void buildUi();
    void connectSignals();
    void loadData();
    void refreshCategoryCombos(int changedLevel = -1);
    void refreshResults();
    void rebuildCategoryMatrix(const QVector<QStringList> &checkedPaths = {});
    void addSecondCategory();
    void openNewItemEditor();
    void openEditorForCurrentResult();
    void openEditorForOffer(const QString &itemName, const QString &shopId);
    void hideEditor();
    void saveEditedItem();
    void deleteEditedItem();
    void markSelectedOffer(bool outOfStock);
    QStringList selectedCategories() const;
    QString selectedItemNameFilter() const;
    ItemRecord itemFromEditor() const;
    void showMessage(const QString &message);

    DataStore m_store;

    QCheckBox *m_alwaysOnTop = nullptr;
    QComboBox *m_category1 = nullptr;
    QComboBox *m_category2 = nullptr;
    QComboBox *m_category3 = nullptr;
    QTableWidget *m_results = nullptr;

    QDialog *m_editDialog = nullptr;
    QLabel *m_editorTitle = nullptr;
    QLineEdit *m_itemName = nullptr;
    QComboBox *m_itemFirstCategory = nullptr;
    QWidget *m_secondCategoryList = nullptr;
    QVBoxLayout *m_secondCategoryListLayout = nullptr;
    QPushButton *m_addSecondCategory = nullptr;
    QLineEdit *m_shopId = nullptr;
    QLineEdit *m_showcaseId = nullptr;
    QLineEdit *m_price = nullptr;
    QComboBox *m_stock = nullptr;
    QPushButton *m_saveItem = nullptr;
    QPushButton *m_deleteItem = nullptr;
    QPushButton *m_cancelEdit = nullptr;

    QPushButton *m_addItem = nullptr;
    QPushButton *m_editSelected = nullptr;
    QPushButton *m_markOutOfStock = nullptr;
    QPushButton *m_markInStock = nullptr;
    bool m_category3ShowsItems = false;
    QMap<QString, QStringList> m_secondCategoriesByFirst;
    QString m_editingItemName;
    QString m_editingShopId;
};
