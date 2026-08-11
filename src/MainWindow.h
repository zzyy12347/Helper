#pragma once

#include "DataStore.h"

#include <QByteArray>
#include <QMap>
#include <QMainWindow>
#include <QRect>

class QCheckBox;
class QComboBox;
class QDialog;
class QLabel;
class QLineEdit;
class OcrRunner;
class QPushButton;
class QScrollArea;
class ScreenCaptureOverlay;
class QTableWidget;
class QVBoxLayout;
class QWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private:
    enum class CaptureMode {
        None,
        SingleShot,
        FixedRegionSetup,
    };

    void buildUi();
    void connectSignals();
    void loadData();
    void refreshCategoryCombos(int changedLevel = -1);
    void refreshResults();
    void rebuildCategoryMatrix(const QVector<QStringList> &checkedPaths = {});
    void addSecondCategory();
    void openSecondCategoryManager();
    void openNewItemEditor();
    void openEditorForCurrentResult();
    void openEditorForOffer(const QString &itemName, const QString &shopId);
    void hideEditor();
    void saveEditedItem();
    void deleteEditedItem();
    void markSelectedOffer(bool outOfStock);
    void startSingleCaptureRecognition();
    void toggleFixedRegionCapture();
    void beginCapture(CaptureMode mode);
    void handleCaptureSelection(const QRect &globalRect);
    void runRecognitionForRegion(const QRect &globalRect, bool triggeredByHotkey);
    void applyItemAutoSearch(const ItemRecord &item);
    void loadOcrConfig();
    void persistOcrConfig();
    bool updateGlobalHotkey();
    void unregisterGlobalHotkey();
    QStringList selectedCategories() const;
    QString selectedItemNameFilter() const;
    ItemRecord itemFromEditor() const;
    void showMessage(const QString &message);
    bool nativeEvent(const QByteArray &eventType, void *message, long *result) override;

    DataStore m_store;

    QCheckBox *m_alwaysOnTop = nullptr;
    QComboBox *m_category1 = nullptr;
    QComboBox *m_category2 = nullptr;
    QComboBox *m_category3 = nullptr;
    QPushButton *m_manageSecondCategories = nullptr;
    QLabel *m_resultSummary = nullptr;
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
    QPushButton *m_captureRecognition = nullptr;
    QPushButton *m_fixedRegionRecognition = nullptr;
    QPushButton *m_markOutOfStock = nullptr;
    QPushButton *m_markInStock = nullptr;
    ScreenCaptureOverlay *m_captureOverlay = nullptr;
    OcrRunner *m_ocrRunner = nullptr;
    bool m_category3ShowsItems = false;
    QMap<QString, QStringList> m_secondCategoriesByFirst;
    QString m_editingItemName;
    QString m_editingShopId;
    CaptureMode m_captureMode = CaptureMode::None;
    QRect m_fixedCaptureRegion;
    bool m_fixedCaptureEnabled = false;
    QString m_hotkey = QStringLiteral("F8");
    bool m_hotkeyRegistered = false;
};
