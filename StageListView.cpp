/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "StageListView.h.moc"
#include "StageSequence.h"
#include "ChangedStateItemDelegate.h"
#include "SkinnedButton.h"
#include "BubbleAnimation.h"
#include <QAbstractTableModel>
#include <QStyledItemDelegate>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QScrollBar>
#include <QPainter>
#include <QPalette>
#include <QStyle>
#include <QColor>
#include <QTimer>
#include <QTimerEvent>
#include <QVariant>
#include <Qt>
#include <QDebug>
#include <boost/foreach.hpp>
#include <algorithm>
#include <assert.h>

class StageListView::Model : public QAbstractTableModel
{
public:
	Model(QObject* parent, IntrusivePtr<StageSequence> const& stages);
	
	void updateBatchProcessingAnimation(
		int selected_row, QPixmap const& animation_frame);
	
	void disableBatchProcessingAnimation();
	
	virtual int columnCount(QModelIndex const& parent) const;
	
	virtual int rowCount(QModelIndex const& parent) const;
	
	virtual QVariant data(QModelIndex const& index, int role) const;
private:
	IntrusivePtr<StageSequence> m_ptrStages;
	QPixmap m_curAnimationFrame;
	int m_curSelectedRow;
};


class StageListView::RightColDelegate : public QStyledItemDelegate
{
public:
	RightColDelegate(QObject* parent = 0);
	
	virtual void paint(
		QPainter* painter, QStyleOptionViewItem const& option,
		QModelIndex const& index) const;
};


StageListView::StageListView(QWidget* parent)
:	QTableView(parent),
	m_sizeHint(QTableView::sizeHint()),
	m_pModel(0),
	m_pFirstColDelegate(new ChangedStateItemDelegate<>(this)),
	m_pSecondColDelegate(new ChangedStateItemDelegate<RightColDelegate>(this)),
	m_curBatchAnimationFrame(0),
	m_timerId(0),
	m_batchProcessingPossible(false),
	m_batchProcessingInProgress(false)
{
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	
	// Prevent current item visualization. Not to be confused
	// with selected items.
	m_pFirstColDelegate->flagsForceDisabled(QStyle::State_HasFocus);
	m_pSecondColDelegate->flagsForceDisabled(QStyle::State_HasFocus);
	setItemDelegateForColumn(0, m_pFirstColDelegate);
	setItemDelegateForColumn(1, m_pSecondColDelegate);
	
	QHeaderView* h_header = horizontalHeader();
	h_header->setResizeMode(QHeaderView::Stretch);
	h_header->hide();
	
	QHeaderView* v_header = verticalHeader();
	v_header->setResizeMode(QHeaderView::ResizeToContents);
	v_header->setMovable(false);
	
	m_pLaunchBtn = new SkinnedButton(
		":/icons/play-small.png",
		":/icons/play-small-hovered.png",
		":/icons/play-small-pressed.png",
		this
	);
	m_pLaunchBtn->setStatusTip(tr("Launch batch processing"));
	m_pLaunchBtn->hide();
	
	connect(
		m_pLaunchBtn, SIGNAL(clicked()),
		this, SIGNAL(launchBatchProcessing())
	);
	
	connect(
		verticalScrollBar(), SIGNAL(rangeChanged(int, int)),
		this, SLOT(ensureSelectedRowVisible()), Qt::QueuedConnection
	);
}

StageListView::~StageListView()
{
}

void
StageListView::setStages(IntrusivePtr<StageSequence> const& stages)
{
	if (QAbstractItemModel* m = model()) {
		// Q*View classes don't own their models.
		m->deleteLater();
	}
	
	m_pModel = new Model(this, stages);
	setModel(m_pModel);
	
	QHeaderView* h_header = horizontalHeader();
	QHeaderView* v_header = verticalHeader();
	h_header->setResizeMode(0, QHeaderView::Stretch);
	h_header->setResizeMode(1, QHeaderView::Fixed);
	if (v_header->count() != 0) {
		// Make the cells in the last column square.
		int const square_side = v_header->sectionSize(0);
		h_header->resizeSection(1, square_side);
		int const reduced_square_side = std::max(1, square_side - 6);
		createBatchAnimationSequence(reduced_square_side);
	} else {
		// Just to avoid special cases elsewhere.
		createBatchAnimationSequence(1);
	}
	m_curBatchAnimationFrame = 0;
	
	updateRowSpans();
	
	// Limit the vertical size to make it just enough to get
	// rid of the scrollbars, but not more.
	int height = verticalHeader()->length();
	height += this->height() - viewport()->height();
	m_sizeHint.setHeight(height);
	QSizePolicy sp(QSizePolicy::Preferred, QSizePolicy::Maximum);
	sp.setVerticalStretch(1);
	setSizePolicy(sp);
	updateGeometry();
}

void
StageListView::setBatchProcessingPossible(bool const possible)
{
	if (m_batchProcessingPossible == possible) {
		return;
	}
	m_batchProcessingPossible = possible;
	
	if (possible) {
		placeLaunchButton(selectedRow());
	} else {
		removeLaunchButton(selectedRow());
	}
}

void
StageListView::setBatchProcessingInProgress(bool const in_progress)
{
	if (m_batchProcessingInProgress == in_progress) {
		return;
	}
	m_batchProcessingInProgress = in_progress;
	
	if (in_progress) {
		removeLaunchButton(selectedRow());
		updateRowSpans(); // Join columns.
		
		// Some styles (Oxygen) visually separate items in a selected row.
		// We really don't want that, so we pretend the items are not selected.
		m_pFirstColDelegate->flagsForceDisabled(QStyle::State_Selected);
		m_pSecondColDelegate->flagsForceDisabled(QStyle::State_Selected);
		
		initiateBatchAnimationFrameRendering();
		m_timerId = startTimer(180);
	} else {
		updateRowSpans(); // Separate columns.
		placeLaunchButton(selectedRow());
		
		m_pFirstColDelegate->removeChanges(QStyle::State_Selected);
		m_pSecondColDelegate->removeChanges(QStyle::State_Selected);
		
		if (m_pModel) {
			m_pModel->disableBatchProcessingAnimation();
		}
		killTimer(m_timerId);
		m_timerId = 0;
	}
}

void
StageListView::timerEvent(QTimerEvent* event)
{
	if (event->timerId() != m_timerId) {
		QTableView::timerEvent(event);
		return;
	}
	
	initiateBatchAnimationFrameRendering();
}

void
StageListView::initiateBatchAnimationFrameRendering()
{
	if (!m_pModel || !m_batchProcessingInProgress) {
		return;
	}
	
	int const selected_row = selectedRow();
	if (selected_row == -1) {
		return;
	}
	
	m_pModel->updateBatchProcessingAnimation(
		selected_row, m_batchAnimationPixmaps[m_curBatchAnimationFrame]
	);
	if (++m_curBatchAnimationFrame == (int)m_batchAnimationPixmaps.size()) {
		m_curBatchAnimationFrame = 0;
	}
}

void
StageListView::selectionChanged(
	QItemSelection const& selected,
	QItemSelection const& deselected)
{
	// Call the base version.
	QTableView::selectionChanged(selected, deselected);
	
	if (!deselected.isEmpty()) {
		removeLaunchButton(deselected.front().topLeft().row());
	}
	
	if (!selected.isEmpty()) {
		placeLaunchButton(selected.front().topLeft().row());
	}
}

void
StageListView::ensureSelectedRowVisible()
{
	// This loop won't run more than one iteration.
	BOOST_FOREACH(QModelIndex const& idx, selectionModel()->selectedRows(0)) {
		scrollTo(idx, EnsureVisible);
	}
}

void
StageListView::removeLaunchButton(int const row)
{
	if (row == -1) {
		return;
	}
	
	m_pLaunchBtn->hide();
	m_pLaunchBtn->setParent(this);
	setIndexWidget(m_pModel->index(row, 0), 0);
}

void
StageListView::placeLaunchButton(int row)
{
	if (row == -1) {
		return;
	}
	
	std::auto_ptr<QWidget> outer_widget(new QWidget);
	QHBoxLayout* layout;
	outer_widget->setLayout((layout = new QHBoxLayout()));
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);
	layout->addStretch(1);
	layout->addWidget(m_pLaunchBtn);
	m_pLaunchBtn->show();
	setIndexWidget(m_pModel->index(row, 0), outer_widget.release());
}

void
StageListView::createBatchAnimationSequence(int const square_side)
{
	int const num_frames = 8;
	m_batchAnimationPixmaps.resize(num_frames);
	
	QColor const head_color(palette().color(QPalette::Window).darker(200));
	QColor const tail_color(palette().color(QPalette::Window).darker(130));
	
	BubbleAnimation animation(num_frames);
	for (int i = 0; i < num_frames; ++i) {
		QPixmap& pixmap = m_batchAnimationPixmaps[i];
		if (pixmap.width() != square_side || pixmap.height() != square_side) {
			pixmap = QPixmap(square_side, square_side);
		}
		pixmap.fill(Qt::transparent);
		animation.nextFrame(head_color, tail_color, &pixmap);
	}
}

void
StageListView::updateRowSpans()
{
	if (!m_pModel) {
		return;
	}
	
	int const count = m_pModel->rowCount(QModelIndex());
	for (int i = 0; i < count; ++i) {
		setSpan(i, 0, 1, m_batchProcessingInProgress ? 1 : 2);
	}
}

int
StageListView::selectedRow() const
{
	QModelIndexList const selection(selectionModel()->selectedRows(0));
	if (selection.empty()) {
		return - 1;
	}
	return selection.front().row();
}


/*========================= StageListView::Model ======================*/

StageListView::Model::Model(
	QObject* parent, IntrusivePtr<StageSequence> const& stages)
:	QAbstractTableModel(parent),
	m_ptrStages(stages),
	m_curSelectedRow(0)
{
	assert(m_ptrStages.get());
}

void
StageListView::Model::updateBatchProcessingAnimation(
	int const selected_row, QPixmap const& animation_frame)
{
	int const max_row = std::max(selected_row, m_curSelectedRow);
	m_curSelectedRow = selected_row;
	m_curAnimationFrame = animation_frame;
	emit dataChanged(index(0, 1), index(max_row, 1));
}

void
StageListView::Model::disableBatchProcessingAnimation()
{
	m_curAnimationFrame = QPixmap();
	emit dataChanged(index(0, 1), index(m_curSelectedRow, 1));
}

int
StageListView::Model::columnCount(QModelIndex const& parent) const
{
	return 2;
}

int
StageListView::Model::rowCount(QModelIndex const& parent) const
{
	return m_ptrStages->count();
}

QVariant
StageListView::Model::data(QModelIndex const& index, int const role) const
{
	if (role == Qt::DisplayRole) {
		if (index.column() == 0) {
			return m_ptrStages->filterAt(index.row())->getName();
		}
	}
	if (role == Qt::UserRole) {
		if (index.column() == 1) {
			if (index.row() <= m_curSelectedRow) {
				return m_curAnimationFrame;
			}
		}
	}
	return QVariant();
}


/*================= StageListView::RightColDelegate ===================*/

StageListView::RightColDelegate::RightColDelegate(QObject* parent)
:	QStyledItemDelegate(parent)
{
}

void
StageListView::RightColDelegate::paint(
	QPainter* painter, QStyleOptionViewItem const& option,
	QModelIndex const& index) const
{
	QStyledItemDelegate::paint(painter, option, index);
	
	QVariant const var(index.data(Qt::UserRole));
	if (!var.isNull()) {
		QPixmap const pixmap(var.value<QPixmap>());
		if (!pixmap.isNull()) {
			QRect r(pixmap.rect());
			r.moveCenter(option.rect.center());
			painter->drawPixmap(r, pixmap);
		}
	}
}
