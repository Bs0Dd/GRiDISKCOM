#include "filepanelwidget.h"
#include "themedicon.h"

#include <algorithm>

#include <QAction>
#include <QApplication>
#include <QColor>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileInfo>
#include <QFont>
#include <QFontMetrics>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QIcon>
#include <QKeyEvent>
#include <QLineEdit>
#include <QLabel>
#include <QMimeData>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QResizeEvent>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QStyledItemDelegate>
#include <QStyle>
#include <QStyleOptionViewItem>
#include <QTableWidget>
#include <QTableWidgetItem>
#include <QToolButton>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

namespace {
constexpr int kColumnCount = 7;

// Column geometry mirrored from the legacy setup so the layout stays identical.
struct ColumnSpec {
    const char* header;
    int width;
};

const ColumnSpec kColumns[kColumnCount] = {
    {"File name",   155},
    {"File type",   80},
    {"Size",        45},
    {"Version",     80},
    {"Created",     100},
    {"Modified",    100},
    {"Expires",     100},
};



QString formatDate(const QDate& date) {
    // An unset date (e.g. ccos expiry) arrives as an invalid QDate -> blank cell.
    if (!date.isValid())
        return {};
    return date.toString(QStringLiteral("dd.MM.yyyy"));
}

// Builds one non-editable row from a fixed-size text array; used both for the
// ".." pseudo-entry and for real file entries so the two paths stay identical.
void appendRow(QTableWidget* table, const QString (&texts)[kColumnCount]) {
    const int row = table->rowCount();
    table->insertRow(row);
    for (int c = 0; c < kColumnCount; ++c) {
        auto* item = new QTableWidgetItem(texts[c]);
        item->setFlags(item->flags() & ~Qt::ItemIsEditable);
        table->setItem(row, c, item);
    }
}
}  // namespace

// Paints a soft background across an entire hovered row. QTableWidget only
// tints the single cell under the cursor; we override the background brush for
// every cell of the hovered row so the highlight is seamless.
class RowHoverDelegate : public QStyledItemDelegate {
public:
    RowHoverDelegate(const QColor& hoverColor, QObject* parent)
        : QStyledItemDelegate(parent), m_hoverColor(hoverColor) {}

    void setHoverRow(int row) { m_hoverRow = row; }

    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override {
        QStyleOptionViewItem opt = option;
        initStyleOption(&opt, index);
        if (index.row() == m_hoverRow && !(opt.state & QStyle::State_Selected))
            opt.backgroundBrush = m_hoverColor;
        const QWidget* widget = opt.widget ? opt.widget : option.widget;
        QStyle* style = widget ? widget->style() : QApplication::style();
        style->drawControl(QStyle::CE_ItemViewItem, &opt, painter, widget);
    }

private:
    int m_hoverRow = -1;
    QColor m_hoverColor;
};

FilePanelWidget::FilePanelWidget(QWidget* parent) : QWidget(parent) {
    buildUi();
}

FilePanelWidget::~FilePanelWidget() = default;

void FilePanelWidget::buildUi() {
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(4, 0, 4, 4);
    layout->setSpacing(4);

    m_groupBox = new QGroupBox(this);
    QFont titleFont = m_groupBox->font();
    titleFont.setPointSize(10);
    titleFont.setBold(false);
    m_groupBox->setFont(titleFont);
    m_groupBox->setTitle({});
    auto* boxLayout = new QVBoxLayout(m_groupBox);
    auto* titleLayout = new QHBoxLayout;
    titleLayout->setContentsMargins(0, 0, 0, 0);
    titleLayout->setSpacing(2);

    m_titleLabel = new QLabel(m_groupBox);
    m_titleLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    titleLayout->addWidget(m_titleLabel);

    m_partitionButton = new QToolButton(m_groupBox);
    m_partitionButton->setIcon(themedSvgIcon(":/resources/switch-partition.svg", m_partitionButton->palette()));
    m_partitionButton->setToolTip("Switch disk partition");
    m_partitionButton->setAccessibleName("Switch disk partition");
    m_partitionButton->setVisible(false);
    titleLayout->addWidget(m_partitionButton);

    m_searchButton = new QToolButton(m_groupBox);
    m_searchButton->setCheckable(true);
    m_searchButton->setIcon(themedSvgIcon(":/resources/search.svg", m_searchButton->palette()));
    m_searchButton->setStyleSheet(
        "QToolButton:checked { border: 1px solid palette(highlight); border-radius: 3px; }");
    m_searchButton->setToolTip("Search");
    m_searchButton->setAccessibleName("Search");
    m_searchButton->setVisible(false);
    titleLayout->addWidget(m_searchButton);
    boxLayout->addLayout(titleLayout);

    m_searchField = new QLineEdit(m_groupBox);
    m_searchField->setPlaceholderText("Search files");
    m_searchField->setValidator(new QRegularExpressionValidator(
        QRegularExpression(QStringLiteral("[\\x00-\\xFF]*")), m_searchField));
    m_searchHelpAction = m_searchField->addAction(
        themedSvgIcon(":/resources/help.svg", m_searchField->palette()), QLineEdit::TrailingPosition);
    m_searchHelpAction->setToolTip(
        "Search syntax:\n"
        "Use ` to separate path components.\n"
        "Use * as a wildcard; matching is case-insensitive.\n"
        "Without *, text matches any part of a name or type.\n"
        "Example: *~Font~ searches the current folder.\n"
        "Example: `*`*~Font~ searches all folders from root.");
    m_searchHelpAction->setVisible(false);
    m_searchField->hide();
    boxLayout->addWidget(m_searchField);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(200);

    m_stack = new QStackedWidget(this);
    m_stack->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // --- table --------------------------------------------------------------
    m_table = new QTableWidget(this);
    m_table->setColumnCount(kColumnCount);
    m_table->verticalHeader()->hide();
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setContextMenuPolicy(Qt::CustomContextMenu);
    m_table->setAutoFillBackground(true);
    QFont tableFont = m_table->font();
    tableFont.setFamily(QStringLiteral("Arial"));
    tableFont.setPointSize(9);
    tableFont.setBold(false);
    m_table->setFont(tableFont);

    for (int i = 0; i < kColumnCount; ++i) {
        m_table->setHorizontalHeaderItem(i, new QTableWidgetItem(QString::fromLatin1(kColumns[i].header)));
        if (kColumns[i].width > 0)
            m_table->horizontalHeader()->resizeSection(i, kColumns[i].width);
    }
    QFont headerFont = m_table->horizontalHeader()->font();
    headerFont.setBold(true);
    m_table->horizontalHeader()->setFont(headerFont);

    QColor hover = palette().color(QPalette::Highlight);
    hover.setAlpha(60);
    m_delegate = new RowHoverDelegate(hover, m_table);
    m_table->setItemDelegate(m_delegate);
    m_table->setMouseTracking(true);
    m_table->viewport()->setMouseTracking(true);

    m_stack->addWidget(m_table);

    // --- empty placeholder --------------------------------------------------
    m_emptyLabel = new QLabel(this);
    m_emptyLabel->setAlignment(Qt::AlignCenter);
    m_stack->addWidget(m_emptyLabel);
    boxLayout->addWidget(m_stack);
    layout->addWidget(m_groupBox);

    // --- status line --------------------------------------------------------
    m_statusLabel = new QLabel(this);
    QFont statusFont = m_statusLabel->font();
    statusFont.setPointSize(9);
    m_statusLabel->setFont(statusFont);
    m_statusLabel->setVisible(false);
    layout->addWidget(m_statusLabel);

    // --- drop overlay ------------------------------------------------------
    // A layout-less child shown on top while a drag hovers the panel: an opaque
    // panel with a highlight border and centered "Drop here" text. It doubles as
    // both the border highlight and the drop hint, and works the same whether the
    // panel is empty (covers the icon) or full (covers the table). resizeEvent
    // keeps it pinned to rect().
    m_dropOverlay = new QLabel(QStringLiteral("Drop here"), this);
    m_dropOverlay->setAlignment(Qt::AlignCenter);
    QFont dropFont = m_dropOverlay->font();
    dropFont.setPointSize(14);
    dropFont.setBold(true);
    m_dropOverlay->setFont(dropFont);
    m_dropOverlay->setStyleSheet(QStringLiteral(
        "QLabel { background: palette(window); color: palette(text);"
        " border: 3px solid palette(highlight); border-radius: 5px; }"));
    m_dropOverlay->hide();

    connect(m_table, &QTableWidget::cellActivated, this, &FilePanelWidget::onCellActivated);
    connect(m_partitionButton, &QToolButton::clicked, this, &FilePanelWidget::partitionSwitchRequested);
    connect(m_searchButton, &QToolButton::toggled, this, &FilePanelWidget::onSearchToggled);
    connect(m_searchField, &QLineEdit::textChanged, this, &FilePanelWidget::onSearchTextChanged);
    connect(m_searchTimer, &QTimer::timeout, this, [this] { emit searchRequested(m_searchField->text()); });
    connect(m_table, &QTableWidget::customContextMenuRequested, this,
            &FilePanelWidget::onContextMenuRequested);
    m_table->installEventFilter(this);
    m_table->viewport()->installEventFilter(this);
    // QLabel doesn't propagate mouse events to its parent, so filter the
    // placeholder to turn a click into openRequested().
    m_emptyLabel->installEventFilter(this);

    setAcceptDrops(true);  // children don't accept drops => events bubble here

    // Redirect focus to the table: panelWidget->setFocus() then lands on the
    // table, whose FocusIn bolds the title and emits panelActivated().
    setFocusProxy(m_table);

    rebuildTable();
}

void FilePanelWidget::setFiles(const QVector<PanelFileEntry>& files, bool inSubdir) {
    m_files = files;
    m_inSubdir = inSubdir;
    rebuildTable();
}

void FilePanelWidget::rebuildTable() {
    m_table->setRowCount(0);

    if (m_files.isEmpty() && !m_diskPresent) {
        m_stack->setCurrentWidget(m_emptyLabel);
        updateEmptyIcon();
        return;
    }
    m_stack->setCurrentWidget(m_table);

    if (m_inSubdir) {
        const QString parentRow[kColumnCount] = {
            QStringLiteral(".."), QStringLiteral("Subject"),
            {}, {}, {}, {}, {}
        };
        appendRow(m_table, parentRow);
    }

    for (const auto& entry : m_files) {
        const QString texts[kColumnCount] = {
            entry.name,
            entry.type,
            QString::number(entry.size),
            entry.version,
            formatDate(entry.creationDate),
            formatDate(entry.modificationDate),
            formatDate(entry.expirationDate),
        };
        appendRow(m_table, texts);
    }
}

void FilePanelWidget::restoreSelection(int fileIndex, int scroll) {
    if (m_table->rowCount() == 0)
        return;
    // fileIndex < 0 (no selection / focus on the ".." row) falls back to the
    // top row. In a subdir the ".." entry lives at row 0, and -1 + 1 == 0, so
    // the parent-row focus is preserved naturally.
    const int row = m_inSubdir ? fileIndex + 1 : fileIndex;
    m_table->setCurrentCell(qMax(0, row), 0);

    // Defer the scroll restore to the next event-loop tick: the table must
    // finish laying out (row heights / scrollbar range) before the value can
    // stick. Doing it synchronously right after inserting rows is what made
    // the scroll "snap back". The deferred call also runs after any auto-scroll
    // triggered by setCurrentCell(), so it wins.
    if (scroll >= 0) {
        QTimer::singleShot(0, this, [this, scroll]() {
            m_table->verticalScrollBar()->setValue(scroll);
        });
    }
}

int FilePanelWidget::verticalScrollValue() const {
    return m_table->verticalScrollBar()->value();
}

void FilePanelWidget::setTitle(const QString& title) {
    m_titleText = title;
    updateTitle();
}

void FilePanelWidget::setHddMode(bool enabled) {
    m_partitionButton->setVisible(enabled);
    updateTitle();
}

void FilePanelWidget::toggleSearch() {
    if (m_diskPresent)
        m_searchButton->toggle();
}

void FilePanelWidget::updateTitle() {
    const QFontMetrics metrics(m_titleLabel->font());
    int buttonsWidth = 0;
    if (m_partitionButton->isVisible())
        buttonsWidth += m_partitionButton->sizeHint().width();
    if (m_searchButton->isVisible())
        buttonsWidth += m_searchButton->sizeHint().width();
    const int availableWidth = qMax(0, m_groupBox->contentsRect().width() - buttonsWidth - 16);
    m_titleLabel->setText(metrics.elidedText(m_titleText, Qt::ElideRight, availableWidth));
}

void FilePanelWidget::setStatusText(const QString& text) {
    m_statusLabel->setText(text);
    m_statusLabel->setVisible(!text.isEmpty());
}

void FilePanelWidget::setDiskPresent(bool present) {
    m_diskPresent = present;
    m_searchButton->setVisible(present);
    if (!present) {
        m_searchButton->setChecked(false);
        const QSignalBlocker blocker(m_searchField);
        m_searchField->clear();
        m_searchTimer->stop();
        m_searchField->hide();
        m_searchHelpAction->setVisible(false);
    }
    updateTitle();
    if (m_files.isEmpty())
        updateEmptyIcon();
}

void FilePanelWidget::updateEmptyIcon() {
    const QIcon icon = QApplication::style()->standardIcon(QStyle::SP_DriveFDIcon);
    m_emptyLabel->setPixmap(icon.pixmap(64, 64));
}



int FilePanelWidget::visualRowToFileIndex(int row) const {
    return m_inSubdir ? row - 1 : row;
}

int FilePanelWidget::currentFileIndex() const {
    if (m_files.isEmpty())
        return -1;

    QTableWidgetItem* current = m_table->currentItem();
    if (current == nullptr)
        return -1;

    const int idx = visualRowToFileIndex(current->row());
    return idx < 0 ? -1 : idx;
}

QVector<int> FilePanelWidget::selectedFileIndices() const {
    QVector<int> result;
    if (m_files.isEmpty())
        return result;

    const auto rows = m_table->selectionModel()->selectedRows();
    result.reserve(rows.size());
    for (const auto& index : rows) {
        const int idx = visualRowToFileIndex(index.row());
        if (idx >= 0 && idx < m_files.size())
            result.append(idx);
    }
    std::sort(result.begin(), result.end());
    result.erase(std::unique(result.begin(), result.end()), result.end());
    return result;
}

void FilePanelWidget::onCellActivated(int row, int /*column*/) {
    const int idx = visualRowToFileIndex(row);
    if (idx < 0) {
        if (m_inSubdir)  // ".." row -> step one directory up
            emit goUpToParent();
        return;
    }
    emit fileDoubleClicked(idx);
}

void FilePanelWidget::onSearchTextChanged(const QString& /*text*/) {
    m_searchTimer->start();
}

void FilePanelWidget::onSearchToggled(bool visible) {
    m_searchField->setVisible(visible);
    m_searchHelpAction->setVisible(visible);
    if (visible) {
        m_searchField->setFocus();
        emit searchRequested(m_searchField->text());
    } else {
        m_searchTimer->stop();
        emit searchRequested({});  // Empty query tells the host to clear the filter.
    }
}

void FilePanelWidget::onContextMenuRequested(const QPoint& pos) {
    QTableWidgetItem* item = m_table->itemAt(pos);
    if (item == nullptr)
        return;
    const int idx = visualRowToFileIndex(item->row());
    if (idx < 0)
        return;
    emit fileRightClicked(idx, m_table->viewport()->mapToGlobal(pos));
}

bool FilePanelWidget::eventFilter(QObject* watched, QEvent* event) {
    if (watched == m_table)
        return handleTableEvent(event) || QWidget::eventFilter(watched, event);
    if (watched == m_table->viewport())
        return handleViewportEvent(event) || QWidget::eventFilter(watched, event);
    if (watched == m_emptyLabel)
        return handleEmptyLabelEvent(event) || QWidget::eventFilter(watched, event);
    return QWidget::eventFilter(watched, event);
}

bool FilePanelWidget::handleTableEvent(QEvent* event) {
    if (event->type() == QEvent::FocusIn) {
        emit panelActivated();
    } else if (event->type() == QEvent::KeyPress) {
        auto* keyEvent = static_cast<QKeyEvent*>(event);
        if (keyEvent->key() == Qt::Key_Escape && m_inSubdir) {
            emit goUpToParent();
            return true;
        }
    }
    return false;
}

bool FilePanelWidget::handleViewportEvent(QEvent* event) {
    if (event->type() == QEvent::MouseMove) {
        auto* me = static_cast<QMouseEvent*>(event);
        setHoverRow(m_table->rowAt(me->pos().y()));
    } else if (event->type() == QEvent::Leave) {
        setHoverRow(-1);
    }
    return false;
}

bool FilePanelWidget::handleEmptyLabelEvent(QEvent* event) {
    // Only react to an actual press (not a stray release after a drag) and
    // only when no disk is loaded.
    if (event->type() == QEvent::MouseButtonPress && !m_diskPresent) {
        emit openRequested();
        return true;
    }
    return false;
}

void FilePanelWidget::setHoverRow(int row) {
    if (row == m_hoverRow)
        return;
    m_hoverRow = row;
    m_delegate->setHoverRow(row);
    m_table->viewport()->update();
}

void FilePanelWidget::setDragOver(bool on) {
    if (on == m_dragOver)
        return;
    m_dragOver = on;
    m_dropOverlay->setVisible(on);
    if (on) {
        m_dropOverlay->setGeometry(rect());
        m_dropOverlay->raise();
    }
}

void FilePanelWidget::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
        setDragOver(true);
    } else {
        event->ignore();
    }
}

void FilePanelWidget::dragMoveEvent(QDragMoveEvent* event) {
    if (event->mimeData()->hasUrls())
        event->acceptProposedAction();
    else
        event->ignore();
}

void FilePanelWidget::dragLeaveEvent(QDragLeaveEvent* event) {
    setDragOver(false);
    event->accept();
}

void FilePanelWidget::dropEvent(QDropEvent* event) {
    setDragOver(false);

    QStringList files;
    const QMimeData* mimeData = event->mimeData();
    if (mimeData && mimeData->hasUrls()) {
        for (const auto& url : mimeData->urls()) {
            const QString path = url.toLocalFile();
            if (path.isEmpty())
                continue;
            if (QFileInfo(path).isDir())
                continue;  // folders are ignored at the panel level
            files.append(path);
        }
    }

    if (!files.isEmpty())
        emit urlsDropped(files);
    event->acceptProposedAction();
}

void FilePanelWidget::resizeEvent(QResizeEvent* event) {
    QWidget::resizeEvent(event);
    updateTitle();
    // The drop overlay is a layout-less child kept on top, so it has to be
    // pinned to the panel manually. This is the idiomatic Qt pattern for a
    // widget that must cover its parent without influencing its geometry.
    if (m_dropOverlay->isVisible())
        m_dropOverlay->setGeometry(rect());
}
