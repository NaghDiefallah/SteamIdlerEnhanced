#ifndef CATEGORIESPANEL_H
#define CATEGORIESPANEL_H

#include <QWidget>
#include <QListWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QColorDialog>

class CategoriesPanel : public QWidget {
    Q_OBJECT

public:
    explicit CategoriesPanel(QWidget *parent = nullptr);
    ~CategoriesPanel();

    void refreshCategories();
    void refreshTags();

private slots:
    void onAddCategory();
    void onRemoveCategory();
    void onEditCategory();
    void onAddTag();
    void onRemoveTag();
    void onCategorySelectionChanged();
    void onTagSelectionChanged();
    void onCategoryDoubleClicked(QListWidgetItem* item);

private:
    void setupUI();
    void setupConnections();
    void populateCategoryList();
    void populateTagList();

    // Category management
    QListWidget* m_categoryList;
    QPushButton* m_addCategoryBtn;
    QPushButton* m_removeCategoryBtn;
    QPushButton* m_editCategoryBtn;
    QLineEdit* m_categoryNameEdit;

    // Tag management
    QListWidget* m_tagList;
    QPushButton* m_addTagBtn;
    QPushButton* m_removeTagBtn;
    QLineEdit* m_tagNameEdit;

    QVBoxLayout* m_mainLayout;
};

#endif // CATEGORIESPANEL_H
