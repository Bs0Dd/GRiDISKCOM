#ifndef FILEPANELWIDGET_H
#define FILEPANELWIDGET_H

#include <QDate>
#include <QString>
#include <QVector>
#include <QWidget>

class QGroupBox;
class QLabel;
class QLineEdit;
class QAction;
class QStackedWidget;
class QTimer;
class QToolButton;
class QTableWidget;
class RowHoverDelegate;

struct PanelFileEntry {
    QString name;
    QString type;
    quint64 size = 0;
    QString version;
    QDate creationDate;
    QDate modificationDate;
    QDate expirationDate;
};

// Reusable file-list panel: a titled table for loaded disks and a drive-icon
// placeholder before an image is opened. Pure UI -- the host feeds entries and
// drives selection.
class FilePanelWidget : public QWidget {
    Q_OBJECT
public:
    explicit FilePanelWidget(QWidget* parent = nullptr);
    ~FilePanelWidget() override;

    void setFiles(const QVector<PanelFileEntry>& files, bool inSubdir);
    void setTitle(const QString& title);
    // Hidden when empty. A non-empty value also implies "a disk is loaded".
    void setStatusText(const QString& text);
    // Tells the panel whether a disk image is loaded, so an empty loaded disk
    // still shows the file table while the unopened panel shows a drive icon.
    void setDiskPresent(bool present);
    void setHddMode(bool enabled);
    void toggleSearch();

    // Per-directory view state, restored by the host after setFiles().
    int verticalScrollValue() const;
    int currentFileIndex() const;  // -1 if none / ".."
    void restoreSelection(int fileIndex, int scroll);  // fileIndex<0 => top row, scroll<0 => leave as-is

    QVector<int> selectedFileIndices() const;

signals:
    void fileDoubleClicked(int index);
    void fileRightClicked(int index, const QPoint& globalPos);  // globalPos for QMenu::exec
    void goUpToParent();     // ".." double-click or Esc
    void panelActivated();   // table gained focus
    void urlsDropped(const QStringList& files);  // folders are ignored
    void openRequested();    // click on empty placeholder while no disk is loaded
    void partitionSwitchRequested();
    void searchRequested(const QString& query);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onCellActivated(int row, int column);
    void onContextMenuRequested(const QPoint& pos);
    void onSearchTextChanged(const QString& text);
    void onSearchToggled(bool visible);

private:
    void buildUi();
    void rebuildTable();
    void updateEmptyIcon();
    void updateTitle();
    void setDragOver(bool on);
    void setHoverRow(int row);

    int visualRowToFileIndex(int row) const;  // table row -> m_files index (-1 for "..")

    // eventFilter dispatchers, one per watched object.
    bool handleTableEvent(QEvent* event);
    bool handleViewportEvent(QEvent* event);
    bool handleEmptyLabelEvent(QEvent* event);

    QGroupBox* m_groupBox = nullptr;
    QLabel* m_titleLabel = nullptr;
    QToolButton* m_partitionButton = nullptr;
    QToolButton* m_searchButton = nullptr;
    QLineEdit* m_searchField = nullptr;
    QAction* m_searchHelpAction = nullptr;
    QTimer* m_searchTimer = nullptr;
    QStackedWidget* m_stack = nullptr;
    QTableWidget* m_table = nullptr;
    QLabel* m_emptyLabel = nullptr;
    QLabel* m_statusLabel = nullptr;
    QLabel* m_dropOverlay = nullptr;  // "Drop here" + border, shown while a drag hovers
    RowHoverDelegate* m_delegate = nullptr;

    QVector<PanelFileEntry> m_files;
    bool m_inSubdir = false;
    bool m_diskPresent = false;
    bool m_dragOver = false;
    int m_hoverRow = -1;
    QString m_titleText;
};

#endif // FILEPANELWIDGET_H
