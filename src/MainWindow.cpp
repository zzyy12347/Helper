#include "MainWindow.h"

#include <QAbstractItemView>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QCoreApplication>
#include <QDateTime>
#include <QDialog>
#include <QDoubleValidator>
#include <QDir>
#include <QFormLayout>
#include <QFrame>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QInputDialog>
#include <QScrollArea>
#include <QSet>
#include <QFileInfo>
#include <QSignalBlocker>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyle>
#include <QTableWidget>
#include <QVBoxLayout>

#include <algorithm>

namespace {
const QStringList kFirstCategories = {
    QStringLiteral("召唤物"),
    QStringLiteral("药品/烹饪"),
    QStringLiteral("家具"),
};

QString trackedDataFilePath()
{
    const QString fallbackPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/items.json";
    const QStringList startPaths = {QCoreApplication::applicationDirPath(), QDir::currentPath()};

    for (const auto &startPath : startPaths) {
        QDir dir(startPath);
        while (true) {
            if (QFileInfo::exists(dir.filePath("CMakeLists.txt")) && QDir(dir.filePath("src")).exists()) {
                return dir.filePath("data/items.json");
            }
            if (!dir.cdUp()) {
                break;
            }
        }
    }

    return fallbackPath;
}

void clearLayoutItems(QLayout *layout)
{
    while (auto *item = layout->takeAt(0)) {
        if (auto *childLayout = item->layout()) {
            clearLayoutItems(childLayout);
        }
        if (auto *widget = item->widget()) {
            delete widget;
        }
        delete item;
    }
}

class TileCheckBox : public QCheckBox {
public:
    explicit TileCheckBox(const QString &text, QWidget *parent = nullptr)
        : QCheckBox(text, parent)
    {
        setCursor(Qt::PointingHandCursor);
        setMinimumSize(132, 38);
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    }

protected:
    bool hitButton(const QPoint &pos) const override
    {
        return rect().contains(pos);
    }

    void paintEvent(QPaintEvent *) override
    {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);

        const auto tileRect = rect().adjusted(1, 1, -1, -1);
        const QColor borderColor = isChecked() ? QColor("#2563eb") : QColor("#d6dee8");
        const QColor fillColor = isChecked() ? QColor("#dbeafe") : QColor("#f8fafc");
        const QColor hoverFillColor = isChecked() ? QColor("#dbeafe") : QColor("#edf6ff");

        painter.setPen(QPen(borderColor, isChecked() ? 2 : 1));
        painter.setBrush(underMouse() ? hoverFillColor : fillColor);
        painter.drawRoundedRect(tileRect, 6, 6);

        const QRect boxRect(10, (height() - 17) / 2, 17, 17);
        painter.setPen(QPen(isChecked() ? QColor("#2563eb") : QColor("#94a3b8"), isChecked() ? 2 : 1));
        painter.setBrush(Qt::white);
        painter.drawRoundedRect(boxRect, 3, 3);

        if (isChecked()) {
            QPen checkPen(QColor("#dc2626"), 2.5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            painter.setPen(checkPen);
            QPainterPath checkPath;
            checkPath.moveTo(boxRect.left() + 4, boxRect.center().y());
            checkPath.lineTo(boxRect.left() + 8, boxRect.bottom() - 4);
            checkPath.lineTo(boxRect.right() - 3, boxRect.top() + 4);
            painter.drawPath(checkPath);
        }

        QFont textFont = painter.font();
        textFont.setPointSize(10);
        painter.setFont(textFont);
        painter.setPen(isChecked() ? QColor("#1d4ed8") : QColor("#0f172a"));
        painter.drawText(QRect(boxRect.right() + 8, 0, width() - boxRect.right() - 14, height()),
                         Qt::AlignVCenter | Qt::AlignLeft,
                         text());
    }
};

}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_store(trackedDataFilePath())
{
    buildUi();
    connectSignals();
    loadData();
}

void MainWindow::buildUi()
{
    setWindowTitle(QStringLiteral("物品店铺查询"));
    resize(780, 500);
    setMinimumSize(700, 420);

    auto *root = new QWidget(this);
    root->setObjectName(QStringLiteral("appRoot"));
    auto *rootLayout = new QVBoxLayout(root);
    rootLayout->setContentsMargins(18, 18, 18, 14);
    rootLayout->setSpacing(14);

    auto *topPanel = new QWidget();
    topPanel->setObjectName(QStringLiteral("filterPanel"));
    auto *topBar = new QHBoxLayout(topPanel);
    topBar->setContentsMargins(14, 12, 14, 12);
    topBar->setSpacing(8);
    m_alwaysOnTop = new QCheckBox(QStringLiteral("窗口置顶"));
    m_category1 = new QComboBox();
    m_category2 = new QComboBox();
    m_category3 = new QComboBox();

    m_category1->setMinimumWidth(132);
    m_category2->setMinimumWidth(132);
    m_category3->setMinimumWidth(132);

    auto *filterTitle = new QLabel(QStringLiteral("筛选"));
    filterTitle->setObjectName(QStringLiteral("sectionTitle"));
    topBar->addWidget(filterTitle);
    topBar->addSpacing(4);
    topBar->addWidget(m_alwaysOnTop);
    topBar->addSpacing(16);
    topBar->addWidget(new QLabel(QStringLiteral("一级")));
    topBar->addWidget(m_category1);
    topBar->addWidget(new QLabel(QStringLiteral("二级")));
    topBar->addWidget(m_category2);
    topBar->addWidget(new QLabel(QStringLiteral("三级")));
    topBar->addWidget(m_category3);
    topBar->addStretch();

    auto *searchPanel = new QWidget();
    searchPanel->setObjectName(QStringLiteral("resultsPanel"));
    auto *searchLayout = new QVBoxLayout(searchPanel);
    searchLayout->setContentsMargins(14, 12, 14, 14);
    searchLayout->setSpacing(12);
    m_results = new QTableWidget(0, 4);
    m_results->setObjectName(QStringLiteral("resultsTable"));
    m_results->setHorizontalHeaderLabels({
        QStringLiteral("物品"),
        QStringLiteral("店铺编号"),
        QStringLiteral("橱窗号"),
        QStringLiteral("价格"),
    });
    m_results->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    m_results->verticalHeader()->setVisible(false);
    m_results->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_results->setSelectionMode(QAbstractItemView::SingleSelection);
    m_results->setEditTriggers(QAbstractItemView::NoEditTriggers);
    m_results->setAlternatingRowColors(true);
    m_results->setShowGrid(false);
    m_results->setWordWrap(false);
    m_results->setCornerButtonEnabled(false);

    auto *bottomBar = new QHBoxLayout();
    bottomBar->setSpacing(8);
    m_markOutOfStock = new QPushButton(QStringLiteral("标记没货"));
    m_markInStock = new QPushButton(QStringLiteral("恢复有货"));
    m_addItem = new QPushButton(QStringLiteral("新增条目"));
    m_editSelected = new QPushButton(QStringLiteral("修改选中"));
    m_addItem->setProperty("role", "primary");
    m_editSelected->setProperty("role", "primary");
    m_markOutOfStock->setProperty("role", "warning");
    m_markOutOfStock->setIcon(style()->standardIcon(QStyle::SP_MessageBoxWarning));
    m_markInStock->setIcon(style()->standardIcon(QStyle::SP_DialogApplyButton));
    m_addItem->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    m_editSelected->setIcon(style()->standardIcon(QStyle::SP_FileDialogDetailedView));
    for (auto *button : {m_markOutOfStock, m_markInStock, m_addItem, m_editSelected}) {
        button->setCursor(Qt::PointingHandCursor);
        button->setIconSize(QSize(16, 16));
    }

    bottomBar->addWidget(m_markOutOfStock);
    bottomBar->addWidget(m_markInStock);
    bottomBar->addStretch();
    bottomBar->addWidget(m_addItem);
    bottomBar->addWidget(m_editSelected);

    auto *resultTitle = new QLabel(QStringLiteral("搜索结果"));
    resultTitle->setObjectName(QStringLiteral("sectionTitle"));
    searchLayout->addWidget(resultTitle);
    searchLayout->addWidget(m_results);
    searchLayout->addLayout(bottomBar);

    m_editDialog = new QDialog(this);
    m_editDialog->setObjectName(QStringLiteral("editDialog"));
    m_editDialog->setWindowTitle(QStringLiteral("编辑条目"));
    m_editDialog->resize(560, 600);
    m_editDialog->setMinimumSize(520, 540);
    auto *editLayout = new QVBoxLayout(m_editDialog);
    editLayout->setContentsMargins(20, 18, 20, 18);
    editLayout->setSpacing(12);
    m_editorTitle = new QLabel(QStringLiteral("编辑条目"));
    m_editorTitle->setObjectName(QStringLiteral("editorTitle"));
    m_itemName = new QLineEdit();
    m_itemName->setPlaceholderText(QStringLiteral("例如：僵尸"));
    m_itemFirstCategory = new QComboBox();
    m_itemFirstCategory->addItems(kFirstCategories);
    auto *secondCategoryArea = new QScrollArea();
    secondCategoryArea->setObjectName(QStringLiteral("categoryScroll"));
    secondCategoryArea->setWidgetResizable(true);
    secondCategoryArea->setFrameShape(QFrame::NoFrame);
    secondCategoryArea->setMaximumHeight(210);
    m_secondCategoryList = new QWidget();
    m_secondCategoryListLayout = new QVBoxLayout(m_secondCategoryList);
    m_secondCategoryListLayout->setContentsMargins(6, 6, 6, 6);
    m_secondCategoryListLayout->setSpacing(6);
    secondCategoryArea->setWidget(m_secondCategoryList);
    m_addSecondCategory = new QPushButton(QStringLiteral("新增二级类目"));
    m_addSecondCategory->setProperty("role", "secondary");
    m_addSecondCategory->setIcon(style()->standardIcon(QStyle::SP_FileDialogNewFolder));
    m_addSecondCategory->setCursor(Qt::PointingHandCursor);
    auto *categoryPanel = new QWidget();
    auto *categoryLayout = new QVBoxLayout(categoryPanel);
    categoryLayout->setContentsMargins(0, 0, 0, 0);
    categoryLayout->setSpacing(6);
    categoryLayout->addWidget(m_itemFirstCategory);
    categoryLayout->addWidget(secondCategoryArea);
    categoryLayout->addWidget(m_addSecondCategory);

    m_shopId = new QLineEdit();
    m_showcaseId = new QLineEdit();
    m_price = new QLineEdit();
    m_shopId->setPlaceholderText(QStringLiteral("店铺编号"));
    m_showcaseId->setPlaceholderText(QStringLiteral("橱窗号"));
    m_price->setPlaceholderText(QStringLiteral("价格"));
    m_price->setValidator(new QDoubleValidator(0, 99999999, 2, m_price));
    m_stock = new QComboBox();
    m_stock->addItems({QStringLiteral("有货"), QStringLiteral("没货")});

    auto *form = new QFormLayout();
    form->setContentsMargins(0, 0, 0, 0);
    form->setHorizontalSpacing(10);
    form->setVerticalSpacing(9);
    form->addRow(QStringLiteral("物品名称"), m_itemName);
    form->addRow(QStringLiteral("类目"), categoryPanel);
    form->addRow(QStringLiteral("店铺编号"), m_shopId);
    form->addRow(QStringLiteral("橱窗号"), m_showcaseId);
    form->addRow(QStringLiteral("价格"), m_price);
    form->addRow(QStringLiteral("库存"), m_stock);

    auto *editButtons = new QHBoxLayout();
    editButtons->setSpacing(8);
    m_saveItem = new QPushButton(QStringLiteral("保存"));
    m_deleteItem = new QPushButton(QStringLiteral("删除"));
    m_cancelEdit = new QPushButton(QStringLiteral("关闭"));
    m_saveItem->setProperty("role", "primary");
    m_deleteItem->setProperty("role", "danger");
    m_cancelEdit->setProperty("role", "secondary");
    m_saveItem->setIcon(style()->standardIcon(QStyle::SP_DialogSaveButton));
    m_deleteItem->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    m_cancelEdit->setIcon(style()->standardIcon(QStyle::SP_DialogCloseButton));
    for (auto *button : {m_saveItem, m_deleteItem, m_cancelEdit}) {
        button->setCursor(Qt::PointingHandCursor);
        button->setIconSize(QSize(16, 16));
    }
    editButtons->addWidget(m_saveItem);
    editButtons->addWidget(m_deleteItem);
    editButtons->addWidget(m_cancelEdit);

    auto *hint = new QLabel(QStringLiteral("每个物品只能选择一个一级类目，可勾选多个二级类目。"));
    hint->setObjectName(QStringLiteral("hintText"));
    hint->setWordWrap(true);

    editLayout->addWidget(m_editorTitle);
    editLayout->addLayout(form);
    editLayout->addLayout(editButtons);
    editLayout->addWidget(hint);
    editLayout->addStretch();

    rootLayout->addWidget(topPanel);
    rootLayout->addWidget(searchPanel);
    setCentralWidget(root);
    statusBar()->setSizeGripEnabled(false);

    setStyleSheet(QStringLiteral(R"(
        QWidget#appRoot {
            background: #eef2f6;
            color: #1f2933;
            font-family: "Microsoft YaHei UI", "Microsoft YaHei", "Segoe UI";
            font-size: 13px;
        }
        QWidget#filterPanel, QWidget#resultsPanel {
            background: #ffffff;
            border: 1px solid #d9e1ea;
            border-radius: 8px;
        }
        QLabel {
            color: #334155;
        }
        QLabel#sectionTitle, QLabel#editorTitle {
            color: #0f172a;
            font-size: 15px;
            font-weight: 700;
        }
        QLabel#editorTitle {
            font-size: 18px;
            padding-bottom: 4px;
        }
        QLabel#hintText {
            color: #64748b;
            line-height: 1.4;
        }
        QDialog#editDialog {
            background: #f8fafc;
        }
        QDialog#editDialog QLabel {
            color: #334155;
        }
        QComboBox, QLineEdit {
            min-height: 34px;
            padding: 5px 10px;
            border: 1px solid #cbd5e1;
            border-radius: 6px;
            background: #ffffff;
            color: #0f172a;
            selection-background-color: #2563eb;
        }
        QComboBox:hover, QLineEdit:hover {
            border-color: #94a3b8;
        }
        QComboBox:focus, QLineEdit:focus {
            border-color: #2563eb;
            background: #fbfdff;
        }
        QComboBox::drop-down {
            width: 26px;
            border: 0;
        }
        QPushButton {
            min-height: 32px;
            padding: 5px 13px;
            border: 1px solid #cbd5e1;
            border-radius: 6px;
            background: #ffffff;
            color: #1e293b;
            font-weight: 500;
        }
        QPushButton:hover {
            background: #f8fafc;
            border-color: #94a3b8;
        }
        QPushButton:pressed {
            background: #e2e8f0;
        }
        QPushButton:disabled {
            color: #94a3b8;
            background: #f1f5f9;
        }
        QPushButton[role="primary"] {
            background: #2563eb;
            border-color: #2563eb;
            color: #ffffff;
            font-weight: 700;
        }
        QPushButton[role="primary"]:hover {
            background: #1d4ed8;
            border-color: #1d4ed8;
        }
        QPushButton[role="warning"] {
            color: #92400e;
            border-color: #f7c873;
            background: #fff7df;
        }
        QPushButton[role="warning"]:hover {
            background: #ffefc2;
        }
        QPushButton[role="danger"] {
            color: #b42318;
            border-color: #f4b4ad;
            background: #fff5f4;
        }
        QPushButton[role="danger"]:hover {
            background: #ffe7e4;
        }
        QPushButton[role="secondary"] {
            color: #155e75;
            border-color: #a5d8e8;
            background: #effbff;
        }
        QPushButton[role="secondary"]:hover {
            background: #dff6ff;
        }
        QTableWidget {
            background: #ffffff;
            alternate-background-color: #f8fafc;
            border: 1px solid #dbe4ee;
            border-radius: 7px;
            gridline-color: transparent;
            outline: 0;
        }
        QTableWidget::item {
            padding: 8px 10px;
            border-bottom: 1px solid #eef2f7;
        }
        QTableWidget::item:selected {
            background: #dbeafe;
            color: #0f172a;
        }
        QHeaderView::section {
            background: #f1f5f9;
            color: #475569;
            padding: 8px 10px;
            border: 0;
            border-bottom: 1px solid #dbe4ee;
            font-weight: 700;
        }
        QCheckBox {
            spacing: 7px;
            color: #334155;
        }
        QCheckBox::indicator {
            width: 15px;
            height: 15px;
        }
        QStatusBar {
            background: transparent;
            color: #64748b;
        }
        QScrollArea#categoryScroll {
            background: #ffffff;
            border: 1px solid #dbe4ee;
            border-radius: 8px;
        }
        QScrollArea#categoryScroll QWidget {
            background: #ffffff;
        }
        QScrollArea#categoryScroll QCheckBox {
            min-height: 28px;
            padding: 3px 5px;
            font-size: 13px;
        }
        QCheckBox#tileCheck {
            min-width: 92px;
            padding: 7px 10px;
            border: 1px solid #d6dee8;
            border-radius: 6px;
            background: #f8fafc;
        }
        QCheckBox#tileCheck:hover {
            background: #edf6ff;
            border-color: #93c5fd;
        }
        QCheckBox#tileCheck:checked {
            background: #dbeafe;
            border-color: #2563eb;
            color: #1d4ed8;
            font-weight: 700;
        }
    )"));
}

void MainWindow::connectSignals()
{
    connect(m_alwaysOnTop, &QCheckBox::toggled, this, [this](bool checked) {
        setWindowFlag(Qt::WindowStaysOnTopHint, checked);
        show();
    });

    connect(m_category1, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        refreshCategoryCombos(0);
        refreshResults();
    });
    connect(m_category2, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        refreshCategoryCombos(1);
        refreshResults();
    });
    connect(m_category3, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        refreshResults();
    });

    connect(m_results, &QTableWidget::cellDoubleClicked, this, [this](int, int) {
        openEditorForCurrentResult();
    });
    connect(m_addItem, &QPushButton::clicked, this, [this] {
        openNewItemEditor();
    });
    connect(m_editSelected, &QPushButton::clicked, this, [this] {
        openEditorForCurrentResult();
    });
    connect(m_saveItem, &QPushButton::clicked, this, [this] {
        saveEditedItem();
    });
    connect(m_deleteItem, &QPushButton::clicked, this, [this] {
        deleteEditedItem();
    });
    connect(m_cancelEdit, &QPushButton::clicked, this, [this] {
        hideEditor();
    });
    connect(m_itemFirstCategory, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] {
        rebuildCategoryMatrix(itemFromEditor().categoryPaths);
    });
    connect(m_addSecondCategory, &QPushButton::clicked, this, [this] {
        addSecondCategory();
    });
    connect(m_markOutOfStock, &QPushButton::clicked, this, [this] {
        markSelectedOffer(true);
    });
    connect(m_markInStock, &QPushButton::clicked, this, [this] {
        markSelectedOffer(false);
    });
}

void MainWindow::loadData()
{
    if (!m_store.load()) {
        QMessageBox::warning(this, QStringLiteral("读取失败"), QStringLiteral("数据文件读取失败，已使用空数据启动。"));
    }

    m_secondCategoriesByFirst = m_store.secondCategoriesByFirst();
    rebuildCategoryMatrix();
    refreshCategoryCombos();
    refreshResults();
}

void MainWindow::refreshCategoryCombos(int changedLevel)
{
    const QSignalBlocker block1(m_category1);
    const QSignalBlocker block2(m_category2);
    const QSignalBlocker block3(m_category3);

    const auto current1 = m_category1->currentText();
    const auto current2 = m_category2->currentText();
    const auto current3 = m_category3->currentText();

    auto fillCombo = [](QComboBox *combo, const QStringList &values, const QString &previous) {
        combo->clear();
        combo->addItem(QStringLiteral("全部"));
        combo->addItems(values);
        const int index = combo->findText(previous);
        combo->setCurrentIndex(index >= 0 ? index : 0);
    };

    auto matchesFilters = [](const ItemRecord &item, const QStringList &filters) {
        for (const auto &path : item.categoryPaths) {
            bool matches = true;
            for (int i = 0; i < filters.size(); ++i) {
                if (filters.at(i).isEmpty()) {
                    continue;
                }
                if (i >= path.size() || path.at(i) != filters.at(i)) {
                    matches = false;
                    break;
                }
            }
            if (matches) {
                return true;
            }
        }
        return false;
    };

    auto categoriesForLevel = [this, &matchesFilters](int level, const QStringList &filters) {
        QSet<QString> values;
        for (const auto &item : m_store.items()) {
            for (const auto &path : item.categoryPaths) {
                if (path.size() <= level) {
                    continue;
                }

                ItemRecord pathItem;
                pathItem.categoryPaths = {path};
                if (matchesFilters(pathItem, filters)) {
                    values.insert(path.at(level));
                }
            }
        }
        auto result = values.values();
        result.sort(Qt::CaseInsensitive);
        return result;
    };

    auto itemsForFilters = [this, &matchesFilters](const QStringList &filters) {
        QSet<QString> values;
        for (const auto &item : m_store.items()) {
            if (matchesFilters(item, filters)) {
                values.insert(item.name);
            }
        }
        auto result = values.values();
        result.sort(Qt::CaseInsensitive);
        return result;
    };

    if (changedLevel <= 0) {
        fillCombo(m_category1, kFirstCategories, current1);
    }

    QStringList filtersForSecond;
    if (m_category1->currentIndex() > 0) {
        filtersForSecond << m_category1->currentText();
    } else {
        filtersForSecond << QString();
    }

    if (changedLevel <= 1) {
        QStringList secondValues;
        if (m_category1->currentIndex() > 0) {
            secondValues = m_secondCategoriesByFirst.value(m_category1->currentText());
        } else {
            QSet<QString> values;
            for (const auto &categories : m_secondCategoriesByFirst) {
                for (const auto &category : categories) {
                    values.insert(category);
                }
            }
            secondValues = values.values();
            secondValues.sort(Qt::CaseInsensitive);
        }
        fillCombo(m_category2, secondValues, current2);
    }

    QStringList filtersForThird;
    filtersForThird << (m_category1->currentIndex() > 0 ? m_category1->currentText() : QString());
    filtersForThird << (m_category2->currentIndex() > 0 ? m_category2->currentText() : QString());

    QStringList thirdValues;
    m_category3ShowsItems = false;
    thirdValues = categoriesForLevel(2, filtersForThird);
    if (thirdValues.isEmpty()) {
        thirdValues = itemsForFilters(filtersForThird);
        m_category3ShowsItems = true;
    }

    fillCombo(m_category3, thirdValues, current3);
}

void MainWindow::rebuildCategoryMatrix(const QVector<QStringList> &checkedPaths)
{
    const QSignalBlocker firstCategoryBlocker(m_itemFirstCategory);
    QString firstCategory = m_itemFirstCategory->currentText();
    if (!checkedPaths.isEmpty() && kFirstCategories.contains(checkedPaths.first().value(0))) {
        firstCategory = checkedPaths.first().value(0);
        m_itemFirstCategory->setCurrentText(firstCategory);
    }

    clearLayoutItems(m_secondCategoryListLayout);

    const auto currentSecondCategories = m_secondCategoriesByFirst.value(firstCategory);
    auto *rowLayout = new QHBoxLayout();
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(8);
    int columnCount = 0;

    for (const auto &secondCategory : currentSecondCategories) {
        auto *check = new TileCheckBox(secondCategory);
        check->setProperty("secondCategory", secondCategory);
        check->setChecked(false);
        check->setObjectName(QStringLiteral("tileCheck"));

        for (const auto &path : checkedPaths) {
            if (path.value(0) == firstCategory && path.value(1) == secondCategory) {
                check->setChecked(true);
                break;
            }
        }

        rowLayout->addWidget(check);
        ++columnCount;
        if (columnCount == 3) {
            rowLayout->addStretch();
            m_secondCategoryListLayout->addLayout(rowLayout);
            rowLayout = new QHBoxLayout();
            rowLayout->setContentsMargins(0, 0, 0, 0);
            rowLayout->setSpacing(8);
            columnCount = 0;
        }
    }
    rowLayout->addStretch();
    m_secondCategoryListLayout->addLayout(rowLayout);
    m_secondCategoryListLayout->addStretch();
}

void MainWindow::addSecondCategory()
{
    bool ok = false;
    const auto name = QInputDialog::getText(this, QStringLiteral("新增二级类目"), QStringLiteral("二级类目名称"), QLineEdit::Normal, {}, &ok).trimmed();
    if (!ok || name.isEmpty()) {
        return;
    }
    const auto firstCategory = m_itemFirstCategory->currentText();
    auto &categories = m_secondCategoriesByFirst[firstCategory];
    if (categories.contains(name)) {
        showMessage(QStringLiteral("这个二级类目已经存在。"));
        return;
    }

    auto currentItem = itemFromEditor();
    categories.push_back(name);
    currentItem.categoryPaths.push_back({firstCategory, name});
    m_store.secondCategoriesByFirst() = m_secondCategoriesByFirst;
    m_store.save();
    rebuildCategoryMatrix(currentItem.categoryPaths);
    refreshCategoryCombos();
    showMessage(QStringLiteral("已新增二级类目。"));
}

void MainWindow::refreshResults()
{
    if (m_store.refreshExpiredStock()) {
        m_store.save();
    }

    struct Row {
        QString itemName;
        QString shopId;
        QString showcaseId;
        double price;
        bool outOfStock;
    };

    QVector<Row> rows;
    const auto itemNameFilter = selectedItemNameFilter();
    const auto matches = m_store.search(selectedCategories());
    for (const auto &item : matches) {
        if (!itemNameFilter.isEmpty() && item.name != itemNameFilter) {
            continue;
        }
        for (const auto &offer : item.offers) {
            rows.push_back({item.name, offer.shopId, offer.showcaseId, offer.price, offer.outOfStock});
        }
    }

    std::sort(rows.begin(), rows.end(), [](const Row &left, const Row &right) {
        if (left.outOfStock != right.outOfStock) {
            return !left.outOfStock;
        }
        return left.price < right.price;
    });

    m_results->setRowCount(rows.size());
    for (int row = 0; row < rows.size(); ++row) {
        const auto &entry = rows.at(row);
        m_results->setItem(row, 0, new QTableWidgetItem(entry.itemName));
        m_results->setItem(row, 1, new QTableWidgetItem(entry.shopId));
        m_results->setItem(row, 2, new QTableWidgetItem(entry.showcaseId));
        auto *priceItem = new QTableWidgetItem(QString::number(entry.price, 'f', 2));
        priceItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        m_results->setItem(row, 3, priceItem);
        m_results->setRowHeight(row, 34);
    }
}

void MainWindow::openNewItemEditor()
{
    m_editingItemName.clear();
    m_editingShopId.clear();
    m_editorTitle->setText(QStringLiteral("新增条目"));
    m_itemName->clear();
    {
        const QSignalBlocker firstCategoryBlocker(m_itemFirstCategory);
        m_itemFirstCategory->setCurrentIndex(0);
        rebuildCategoryMatrix();
    }
    m_shopId->clear();
    m_showcaseId->clear();
    m_price->clear();
    m_stock->setCurrentText(QStringLiteral("有货"));
    m_deleteItem->setEnabled(false);
    m_editDialog->show();
    m_editDialog->raise();
    m_editDialog->activateWindow();
    m_itemName->setFocus();
}

void MainWindow::openEditorForCurrentResult()
{
    const int row = m_results->currentRow();
    if (row < 0 || !m_results->item(row, 0) || !m_results->item(row, 1)) {
        showMessage(QStringLiteral("请先选中一条搜索结果。"));
        return;
    }

    openEditorForOffer(m_results->item(row, 0)->text(), m_results->item(row, 1)->text());
}

void MainWindow::openEditorForOffer(const QString &itemName, const QString &shopId)
{
    for (const auto &item : m_store.items()) {
        if (item.name != itemName) {
            continue;
        }

        m_editingItemName = item.name;
        m_editingShopId = shopId;

        m_editorTitle->setText(QStringLiteral("修改条目"));
        m_itemName->setText(item.name);
        {
            const QSignalBlocker firstCategoryBlocker(m_itemFirstCategory);
            rebuildCategoryMatrix(item.categoryPaths);
        }

        const ShopOffer *selectedOffer = nullptr;
        for (const auto &offer : item.offers) {
            if (offer.shopId == shopId) {
                selectedOffer = &offer;
                break;
            }
        }
        if (!selectedOffer && !item.offers.isEmpty()) {
            selectedOffer = &item.offers.first();
            m_editingShopId = selectedOffer->shopId;
        }

        if (selectedOffer) {
            const auto &offer = *selectedOffer;
            m_shopId->setText(offer.shopId);
            m_showcaseId->setText(offer.showcaseId);
            m_price->setText(QString::number(offer.price, 'f', 2));
            m_stock->setCurrentText(offer.outOfStock ? QStringLiteral("没货") : QStringLiteral("有货"));
        } else {
            m_shopId->clear();
            m_showcaseId->clear();
            m_price->clear();
            m_stock->setCurrentText(QStringLiteral("有货"));
        }
        m_deleteItem->setEnabled(true);
        m_editDialog->show();
        m_editDialog->raise();
        m_editDialog->activateWindow();
        m_shopId->setFocus();
        return;
    }

    showMessage(QStringLiteral("没有找到这个物品。"));
}

void MainWindow::hideEditor()
{
    m_editDialog->hide();
}

void MainWindow::saveEditedItem()
{
    const auto item = itemFromEditor();
    if (item.name.trimmed().isEmpty()) {
        showMessage(QStringLiteral("请填写物品名称。"));
        return;
    }
    if (item.categoryPaths.isEmpty()) {
        showMessage(QStringLiteral("请至少填写一条类目路径。"));
        return;
    }

    const QString savedShopId = item.offers.isEmpty() ? QString() : item.offers.first().shopId;
    if (!m_editingItemName.isEmpty() && !m_editingShopId.isEmpty()
        && (m_editingItemName.compare(item.name, Qt::CaseInsensitive) != 0 || savedShopId != m_editingShopId)) {
        m_store.removeOffer(m_editingItemName, m_editingShopId);
    }

    m_store.addOrUpdateItem(item);
    if (!m_store.save()) {
        showMessage(QStringLiteral("保存失败，请检查数据文件权限。"));
        return;
    }

    refreshCategoryCombos();
    refreshResults();
    hideEditor();
    m_editingItemName.clear();
    m_editingShopId.clear();
    showMessage(QStringLiteral("已保存。"));
}

void MainWindow::deleteEditedItem()
{
    const auto name = m_itemName->text().trimmed();
    if (name.isEmpty()) {
        return;
    }

    if (QMessageBox::question(this, QStringLiteral("删除确认"), QStringLiteral("确定删除“%1”？").arg(name)) != QMessageBox::Yes) {
        return;
    }

    if (!m_editingItemName.isEmpty() && !m_editingShopId.isEmpty()) {
        m_store.removeOffer(m_editingItemName, m_editingShopId);
    } else {
        m_store.removeItem(name);
    }
    m_store.save();
    refreshCategoryCombos();
    refreshResults();
    hideEditor();
    m_editingItemName.clear();
    m_editingShopId.clear();
}

void MainWindow::markSelectedOffer(bool outOfStock)
{
    const int row = m_results->currentRow();
    if (row < 0) {
        showMessage(QStringLiteral("请先选中一条搜索结果。"));
        return;
    }

    const auto itemName = m_results->item(row, 0)->text();
    const auto shopId = m_results->item(row, 1)->text();
    if (!m_store.setOfferStock(itemName, shopId, outOfStock)) {
        showMessage(QStringLiteral("没有找到这条店铺报价。"));
        return;
    }

    m_store.save();
    refreshResults();
    showMessage(outOfStock ? QStringLiteral("已标记没货，系统会按历史次数自动恢复。") : QStringLiteral("已恢复有货。"));
}

QStringList MainWindow::selectedCategories() const
{
    QStringList categories;
    categories << (m_category1->currentIndex() > 0 ? m_category1->currentText() : QString());
    categories << (m_category2->currentIndex() > 0 ? m_category2->currentText() : QString());
    categories << (!m_category3ShowsItems && m_category3->currentIndex() > 0 ? m_category3->currentText() : QString());
    return categories;
}

QString MainWindow::selectedItemNameFilter() const
{
    if (m_category3ShowsItems && m_category3->currentIndex() > 0) {
        return m_category3->currentText();
    }
    return {};
}

ItemRecord MainWindow::itemFromEditor() const
{
    ItemRecord item;
    item.name = m_itemName->text().trimmed();

    const auto firstCategory = m_itemFirstCategory->currentText();
    for (int i = 0; i < m_secondCategoryListLayout->count(); ++i) {
        auto *layout = m_secondCategoryListLayout->itemAt(i)->layout();
        if (!layout) {
            continue;
        }
        for (int j = 0; j < layout->count(); ++j) {
            auto *check = qobject_cast<QCheckBox *>(layout->itemAt(j)->widget());
            if (check && check->isChecked()) {
                item.categoryPaths.push_back({firstCategory, check->property("secondCategory").toString()});
            }
        }
    }

    if (!m_shopId->text().trimmed().isEmpty()) {
        ShopOffer offer;
        offer.shopId = m_shopId->text().trimmed();
        offer.showcaseId = m_showcaseId->text().trimmed();
        offer.price = m_price->text().trimmed().toDouble();
        offer.outOfStock = m_stock->currentText().contains(QStringLiteral("没"));

        for (const auto &existingItem : m_store.items()) {
            if (existingItem.name != item.name) {
                continue;
            }
            for (const auto &existingOffer : existingItem.offers) {
                if (existingOffer.shopId == offer.shopId) {
                    offer.outOfStockCount = existingOffer.outOfStockCount;
                    offer.outOfStockMarkedAt = existingOffer.outOfStockMarkedAt;
                    break;
                }
            }
            break;
        }

        if (offer.outOfStock) {
            if (offer.outOfStockCount <= 0 || offer.outOfStockMarkedAt.isEmpty()) {
                offer.outOfStockCount += 1;
                offer.outOfStockMarkedAt = QDateTime::currentDateTimeUtc().toString(Qt::ISODate);
            }
        } else {
            offer.outOfStockMarkedAt.clear();
        }
        item.offers.push_back(offer);
    }

    return item;
}

void MainWindow::showMessage(const QString &message)
{
    statusBar()->showMessage(message, 3500);
}
