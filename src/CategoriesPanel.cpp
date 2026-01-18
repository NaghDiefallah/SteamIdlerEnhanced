#include "CategoriesPanel.h"
#include "Categories.h"
#include "Logger.h"
#include "Lang.h"
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMessageBox>
#include <QColorDialog>

CategoriesPanel::CategoriesPanel(QWidget *parent)
    : QWidget(parent), m_mainLayout(nullptr) {
    setupUI();
    setupConnections();
    refreshCategories();
    refreshTags();
}

CategoriesPanel::~CategoriesPanel() {
}

void CategoriesPanel::setupUI() {
    m_mainLayout = new QVBoxLayout(this);
    m_mainLayout->setContentsMargins(12, 12, 12, 12);
    m_mainLayout->setSpacing(16);

    // Categories Section
    QGroupBox* categoryGroup = new QGroupBox(Lang::get("sidebar_categories"), this);
    QVBoxLayout* categoryLayout = new QVBoxLayout(categoryGroup);

    m_categoryList = new QListWidget(this);
    m_categoryList->setMaximumHeight(200);
    categoryLayout->addWidget(m_categoryList);

    QHBoxLayout* categoryBtnLayout = new QHBoxLayout();
    m_categoryNameEdit = new QLineEdit(this);
    m_categoryNameEdit->setPlaceholderText("Category name...");
    m_addCategoryBtn = new QPushButton(Lang::get("btn_idle"), this);
    m_removeCategoryBtn = new QPushButton(Lang::get("btn_stop"), this);
    m_editCategoryBtn = new QPushButton("Edit Color", this);

    categoryBtnLayout->addWidget(m_categoryNameEdit);
    categoryBtnLayout->addWidget(m_addCategoryBtn);
    categoryBtnLayout->addWidget(m_editCategoryBtn);
    categoryBtnLayout->addWidget(m_removeCategoryBtn);
    categoryLayout->addLayout(categoryBtnLayout);

    m_mainLayout->addWidget(categoryGroup);

    // Tags Section
    QGroupBox* tagGroup = new QGroupBox("Tags", this);
    QVBoxLayout* tagLayout = new QVBoxLayout(tagGroup);

    m_tagList = new QListWidget(this);
    m_tagList->setMaximumHeight(200);
    tagLayout->addWidget(m_tagList);

    QHBoxLayout* tagBtnLayout = new QHBoxLayout();
    m_tagNameEdit = new QLineEdit(this);
    m_tagNameEdit->setPlaceholderText("Tag name...");
    m_addTagBtn = new QPushButton(Lang::get("btn_idle"), this);
    m_removeTagBtn = new QPushButton(Lang::get("btn_stop"), this);

    tagBtnLayout->addWidget(m_tagNameEdit);
    tagBtnLayout->addWidget(m_addTagBtn);
    tagBtnLayout->addWidget(m_removeTagBtn);
    tagLayout->addLayout(tagBtnLayout);

    m_mainLayout->addWidget(tagGroup);
    m_mainLayout->addStretch();
}

void CategoriesPanel::setupConnections() {
    connect(m_addCategoryBtn, &QPushButton::clicked, this, &CategoriesPanel::onAddCategory);
    connect(m_removeCategoryBtn, &QPushButton::clicked, this, &CategoriesPanel::onRemoveCategory);
    connect(m_editCategoryBtn, &QPushButton::clicked, this, &CategoriesPanel::onEditCategory);
    connect(m_addTagBtn, &QPushButton::clicked, this, &CategoriesPanel::onAddTag);
    connect(m_removeTagBtn, &QPushButton::clicked, this, &CategoriesPanel::onRemoveTag);
    connect(m_categoryList, &QListWidget::itemSelectionChanged, this, &CategoriesPanel::onCategorySelectionChanged);
    connect(m_tagList, &QListWidget::itemSelectionChanged, this, &CategoriesPanel::onTagSelectionChanged);
    connect(m_categoryList, &QListWidget::itemDoubleClicked, this, &CategoriesPanel::onCategoryDoubleClicked);
}

void CategoriesPanel::refreshCategories() {
    populateCategoryList();
}

void CategoriesPanel::refreshTags() {
    populateTagList();
}

void CategoriesPanel::populateCategoryList() {
    m_categoryList->clear();
    auto& categories = Categories::instance();
    auto categoryList = categories.getAllCategories();

    for (const auto& cat : categoryList) {
        QString itemText = QString("%1 (%2 games)").arg(cat.name).arg(cat.gameCount);
        QListWidgetItem* item = new QListWidgetItem(itemText, m_categoryList);
        item->setData(Qt::UserRole, cat.id);

        // Set color indicator
        if (cat.color.isValid()) {
            item->setBackground(cat.color);
            // Adjust text color for visibility
            if (cat.color.lightness() < 128) {
                item->setForeground(Qt::white);
            }
        }
    }
}

void CategoriesPanel::populateTagList() {
    m_tagList->clear();
    auto& categories = Categories::instance();
    auto tagList = categories.getAllTags();

    for (const auto& tag : tagList) {
        QString itemText = QString("%1 (%2 games)").arg(tag.name).arg(tag.gameCount);
        QListWidgetItem* item = new QListWidgetItem(itemText, m_tagList);
        item->setData(Qt::UserRole, tag.name);

        // Set color indicator
        if (tag.color.isValid()) {
            item->setBackground(tag.color);
            if (tag.color.lightness() < 128) {
                item->setForeground(Qt::white);
            }
        }
    }
}

void CategoriesPanel::onAddCategory() {
    QString name = m_categoryNameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Category name cannot be empty");
        return;
    }

    QString id = name.toLower().replace(" ", "_");
    Categories::instance().createCategory(id, name, Qt::gray);
    m_categoryNameEdit->clear();
    populateCategoryList();
    Logger::log()->info("Category added: {}", name.toStdString());
}

void CategoriesPanel::onRemoveCategory() {
    auto selected = m_categoryList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a category to remove");
        return;
    }

    QString id = selected[0]->data(Qt::UserRole).toString();
    Categories::instance().deleteCategory(id);
    populateCategoryList();
    Logger::log()->info("Category removed: {}", id.toStdString());
}

void CategoriesPanel::onEditCategory() {
    auto selected = m_categoryList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a category to edit");
        return;
    }

    QString id = selected[0]->data(Qt::UserRole).toString();
    auto cat = Categories::instance().getCategory(id);

    QColor newColor = QColorDialog::getColor(cat.color, this, "Select Category Color");
    if (newColor.isValid()) {
        Categories::instance().updateCategory(id, cat.name, newColor);
        populateCategoryList();
        Logger::log()->info("Category color updated: {}", id.toStdString());
    }
}

void CategoriesPanel::onAddTag() {
    QString name = m_tagNameEdit->text().trimmed();
    if (name.isEmpty()) {
        QMessageBox::warning(this, "Invalid Input", "Tag name cannot be empty");
        return;
    }

    Categories::instance().createTag(name, Qt::gray);
    m_tagNameEdit->clear();
    populateTagList();
    Logger::log()->info("Tag added: {}", name.toStdString());
}

void CategoriesPanel::onRemoveTag() {
    auto selected = m_tagList->selectedItems();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "No Selection", "Please select a tag to remove");
        return;
    }

    QString name = selected[0]->data(Qt::UserRole).toString();
    Categories::instance().deleteTag(name);
    populateTagList();
    Logger::log()->info("Tag removed: {}", name.toStdString());
}

void CategoriesPanel::onCategorySelectionChanged() {
    auto selected = m_categoryList->selectedItems();
    m_removeCategoryBtn->setEnabled(!selected.isEmpty());
    m_editCategoryBtn->setEnabled(!selected.isEmpty());
}

void CategoriesPanel::onTagSelectionChanged() {
    auto selected = m_tagList->selectedItems();
    m_removeTagBtn->setEnabled(!selected.isEmpty());
}

void CategoriesPanel::onCategoryDoubleClicked(QListWidgetItem* item) {
    onEditCategory();
}
