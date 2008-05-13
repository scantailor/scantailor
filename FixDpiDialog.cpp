/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2008  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "FixDpiDialog.h.moc"
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QModelIndex>
#include <QItemSelection>
#include <QHeaderView>
#include <QVariant>
#include <QIntValidator>
#include <QSize>
#include <QString>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <vector>
#include <algorithm>
#include <assert.h>

namespace
{

static int const NEED_FIXING_TAB = 0;
static int const ALL_PAGES_TAB = 1;

struct DpiLess
{
	bool operator()(Dpi const& lhs, Dpi const& rhs) const {
		if (lhs.isNull() != rhs.isNull()) {
			return lhs.isNull();
		} else if (lhs.isNull()) {
			assert(rhs.isNull());
			return false;
		}
		if (lhs.horizontal() < rhs.horizontal()) {
			return true;
		} else if (lhs.horizontal() > rhs.horizontal()) {
			return false;
		}
		return lhs.vertical() < rhs.vertical();
	}
};

} // anonymous namespace

class FixDpiDialog::DpiCounts
{
public:
	void add(Dpi const& dpi);
	
	void remove(Dpi const& dpi);
	
	bool haveUndefinedDpi() const;
	
	/**
	 * If all items have the same, non-null DPI,
	 * then return it, otherwise return a null DPI.
	 */
	Dpi ifConsistentDpi() const;
private:
	std::map<Dpi, int, DpiLess> m_counts;
};


class FixDpiDialog::SizeGroup
{
public:
	struct Item
	{
		int fileIdx;
		int imageIdx;
		
		Item(int file_idx, int image_idx)
		: fileIdx(file_idx), imageIdx(image_idx) {}
	};
	
	SizeGroup(QSize const& size) : m_size(size) {}
	
	void append(Item const& item, Dpi const& dpi);
	
	QSize const& size() const { return m_size; }
	
	std::vector<Item> const& items() const { return m_items; }
	
	DpiCounts& dpiCounts() { return m_dpiCounts; }
	
	DpiCounts const& dpiCounts() const { return m_dpiCounts; }
private:
	QSize m_size;
	std::vector<Item> m_items;
	DpiCounts m_dpiCounts;
};


class FixDpiDialog::TreeModel : private QAbstractItemModel
{
public:
	enum Scope { ALL, UNDEFINED };
	
	TreeModel(std::vector<ImageFileInfo> const& files);
	
	std::vector<ImageFileInfo> const& files() const { return m_files; }
	
	QAbstractItemModel* model() { return this; }
	
	bool haveUndefinedDpi() const {
		return m_dpiCounts.haveUndefinedDpi();
	}
	
	bool isVisibleForFilter(QModelIndex const& parent, int row) const;
	
	void applyDpiToSelection(
		Scope scope, Dpi const& dpi, QItemSelection const& selection);
private:
	struct Tag {};
	
	virtual int columnCount(QModelIndex const& parent) const;
	
	virtual int rowCount(QModelIndex const& parent) const;
	
	virtual QModelIndex index(int row, int column, QModelIndex const& parent) const;
	
	virtual QModelIndex parent(QModelIndex const& index) const;
	
	virtual QVariant data(QModelIndex const& index, int role) const;
	
	void applyDpiToAllGroups(Scope scope, Dpi const& dpi);
	
	void applyDpiToGroup(Scope scope, Dpi const& dpi, SizeGroup& group, DpiCounts& total_dpi_counts);
	
	void applyDpiToItem(
		Scope scope, Dpi const& dpi, SizeGroup::Item item,
		DpiCounts& total_dpi_counts, DpiCounts& group_dpi_counts);
	
	void emitAllPagesChanged(QModelIndex const& idx);
	
	void emitSizeGroupChanged(QModelIndex const& idx);
	
	void emitItemChanged(QModelIndex const& idx);
	
	SizeGroup& sizeGroupFor(QSize size);
	
	static QString sizeToString(QSize size);
	
	static Tag m_allPagesNodeId;
	static Tag m_sizeGroupNodeId;
	
	std::vector<ImageFileInfo> m_files;
	std::vector<SizeGroup> m_sizes;
	DpiCounts m_dpiCounts;
};


class FixDpiDialog::FilterModel : private QSortFilterProxyModel
{
public:
	FilterModel(TreeModel& delegate);
	
	QAbstractProxyModel* model() { return this; }
private:
	virtual bool filterAcceptsRow(
		int source_row, QModelIndex const& source_parent) const;
	
	virtual QVariant data(QModelIndex const& index, int role) const;
	
	TreeModel& m_rDelegate;
};


FixDpiDialog::FixDpiDialog(std::vector<ImageFileInfo> const& files, QWidget* parent)
:	QDialog(parent),
	m_ptrPages(new TreeModel(files)),
	m_ptrUndefinedDpiPages(new FilterModel(*m_ptrPages))
{
	setupUi(this);
	
	dpiCombo->addItem("300 x 300", QSize(300, 300));
	dpiCombo->addItem("400 x 400", QSize(400, 400));
	dpiCombo->addItem("600 x 600", QSize(600, 600));
	
	tabWidget->setTabText(NEED_FIXING_TAB, tr("Need Fixing"));
	tabWidget->setTabText(ALL_PAGES_TAB, tr("All Pages"));
	undefinedDpiView->setModel(m_ptrUndefinedDpiPages->model()),
	undefinedDpiView->header()->hide();
	allPagesView->setModel(m_ptrPages->model());
	allPagesView->header()->hide();
	
	xDpi->setMaxLength(4);
	yDpi->setMaxLength(4);
	QIntValidator* xDpiValidator = new QIntValidator(xDpi);
	xDpiValidator->setBottom(100);
	xDpi->setValidator(xDpiValidator);
	QIntValidator* yDpiValidator = new QIntValidator(yDpi);
	yDpiValidator->setBottom(100);
	yDpi->setValidator(yDpiValidator);
	
	connect(
		tabWidget, SIGNAL(currentChanged(int)),
		this, SLOT(tabChanged(int))
	);
	
	connect(
		undefinedDpiView->selectionModel(),
		SIGNAL(selectionChanged(QItemSelection const&, QItemSelection const&)),
		this, SLOT(selectionChanged(QItemSelection const&))
	);
	connect(
		allPagesView->selectionModel(),
		SIGNAL(selectionChanged(QItemSelection const&, QItemSelection const&)),
		this, SLOT(selectionChanged(QItemSelection const&))
	);
	
	connect(
		dpiCombo, SIGNAL(activated(int)),
		this, SLOT(dpiComboChangedByUser(int))
	);
	
	connect(
		xDpi, SIGNAL(textEdited(QString const&)),
		this, SLOT(dpiValueChanged())
	);
	connect(
		yDpi, SIGNAL(textEdited(QString const&)),
		this, SLOT(dpiValueChanged())
	);
	
	connect(applyBtn, SIGNAL(clicked()), this, SLOT(applyClicked()));
	
	enableDisableOkButton();
}

FixDpiDialog::~FixDpiDialog()
{
}

std::vector<ImageFileInfo> const&
FixDpiDialog::files() const
{
	return m_ptrPages->files();
}

void
FixDpiDialog::tabChanged(int const tab)
{
	QTreeView* views[2];
	views[NEED_FIXING_TAB] = undefinedDpiView;
	views[ALL_PAGES_TAB] = allPagesView;
	updateDpiFromSelection(views[tab]->selectionModel()->selection());
}

void
FixDpiDialog::selectionChanged(QItemSelection const& selection)
{
	updateDpiFromSelection(selection);
}

void
FixDpiDialog::dpiComboChangedByUser(int const index)
{
	QVariant const data(dpiCombo->itemData(index));
	if (data.isValid()) {
		QSize const dpi(data.toSize());
		xDpi->setText(QString::number(dpi.width()));
		yDpi->setText(QString::number(dpi.height()));
		dpiValueChanged();
	}
}

void
FixDpiDialog::dpiValueChanged()
{
	updateDpiCombo();
	
	if (m_xDpiInitialValue == xDpi->text() &&
			m_yDpiInitialValue == yDpi->text()) {
		applyBtn->setEnabled(false);
		return;
	}
	
	if (xDpi->hasAcceptableInput() && yDpi->hasAcceptableInput()) {
		applyBtn->setEnabled(true);
		return;
	}
	
	applyBtn->setEnabled(false);
}

void
FixDpiDialog::applyClicked()
{
	Dpi const dpi(xDpi->text().toInt(), yDpi->text().toInt());
	QItemSelectionModel* selection_model = 0;
	
	if (tabWidget->currentIndex() == ALL_PAGES_TAB) {
		selection_model = allPagesView->selectionModel();
		QItemSelection const selection(selection_model->selection());
		m_ptrPages->applyDpiToSelection(TreeModel::ALL, dpi, selection);
	} else {
		selection_model = undefinedDpiView->selectionModel();
		QItemSelection const selection(
			m_ptrUndefinedDpiPages->model()->mapSelectionToSource(
				selection_model->selection()
			)
		);
		m_ptrPages->applyDpiToSelection(TreeModel::UNDEFINED, dpi, selection);
	}
	
	updateDpiFromSelection(selection_model->selection());
	enableDisableOkButton();
}

void
FixDpiDialog::enableDisableOkButton()
{
	bool const enable = !m_ptrPages->haveUndefinedDpi();
	buttonBox->button(QDialogButtonBox::Ok)->setEnabled(enable);
}

/**
 * This function work with both TreeModel and FilterModel selections.
 * It is assumed that only a single item is selected.
 */
void
FixDpiDialog::updateDpiFromSelection(QItemSelection const& selection)
{
	if (selection.isEmpty()) {
		resetDpiForm();
		dpiCombo->setEnabled(false);
		xDpi->setEnabled(false);
		yDpi->setEnabled(false);
		// applyBtn is managed elsewhere.
		return;
	}
	
	dpiCombo->setEnabled(true);
	xDpi->setEnabled(true);
	yDpi->setEnabled(true);
	
	QVariant const data(selection.front().topLeft().data(Qt::UserRole));
	if (data.isValid()) {
		setDpiForm(Dpi(data.toSize()));
	} else {
		resetDpiForm();
	}
}

void
FixDpiDialog::resetDpiForm()
{
	dpiCombo->setCurrentIndex(0);
	m_xDpiInitialValue.clear();
	m_yDpiInitialValue.clear();
	xDpi->setText(m_xDpiInitialValue);
	yDpi->setText(m_yDpiInitialValue);
	dpiValueChanged();
}

void
FixDpiDialog::setDpiForm(Dpi const dpi)
{
	if (dpi.isNull()) {
		resetDpiForm();
		return;
	}
	
	m_xDpiInitialValue = QString::number(dpi.horizontal());
	m_yDpiInitialValue = QString::number(dpi.vertical());
	xDpi->setText(m_xDpiInitialValue);
	yDpi->setText(m_yDpiInitialValue);
	dpiValueChanged();
}

void
FixDpiDialog::updateDpiCombo()
{
	bool x_ok = true, y_ok = true;
	QSize const dpi(xDpi->text().toInt(&x_ok), yDpi->text().toInt(&y_ok));
	
	if (x_ok && y_ok) {
		int const count = dpiCombo->count();
		for (int i = 0; i < count; ++i) {
			QVariant const data(dpiCombo->itemData(i));
			if (data.isValid()) {
				if (dpi == data.toSize()) {
					dpiCombo->setCurrentIndex(i);
					return;
				}
			}
		}
	}
	
	dpiCombo->setCurrentIndex(0);
}


/*====================== FixDpiDialog::DpiCounts ======================*/

void
FixDpiDialog::DpiCounts::add(Dpi const& dpi)
{
	++m_counts[dpi];
}

void
FixDpiDialog::DpiCounts::remove(Dpi const& dpi)
{
	if (--m_counts[dpi] == 0) {
		m_counts.erase(dpi);
	}
}

bool
FixDpiDialog::DpiCounts::haveUndefinedDpi() const
{
	return (m_counts.find(Dpi()) != m_counts.end());
}

Dpi
FixDpiDialog::DpiCounts::ifConsistentDpi() const
{
	if (m_counts.size() != 1) {
		return Dpi();
	}
	return m_counts.begin()->first;
}


/*====================== FixDpiDialog::SizeGroup ======================*/

void
FixDpiDialog::SizeGroup::append(Item const& item, Dpi const& dpi)
{
	m_items.push_back(item);
	m_dpiCounts.add(dpi);
}


/*====================== FixDpiDialog::TreeModel ======================*/

FixDpiDialog::TreeModel::Tag FixDpiDialog::TreeModel::m_allPagesNodeId;
FixDpiDialog::TreeModel::Tag FixDpiDialog::TreeModel::m_sizeGroupNodeId;

FixDpiDialog::TreeModel::TreeModel(std::vector<ImageFileInfo> const& files)
:	m_files(files)
{
	int const num_files = m_files.size();
	for (int i = 0; i < num_files; ++i) {
		ImageFileInfo const& file = m_files[i];
		int const num_images = file.imageInfo().size();
		for (int j = 0; j < num_images; ++j) {
			ImageMetadata const& metadata = file.imageInfo()[j];
			SizeGroup& group = sizeGroupFor(metadata.size());
			group.append(SizeGroup::Item(i, j), metadata.dpi());
			m_dpiCounts.add(metadata.dpi());
		}
	}
}

bool
FixDpiDialog::TreeModel::isVisibleForFilter(QModelIndex const& parent, int row) const
{
	void const* const ptr = parent.internalPointer();
	
	if (!parent.isValid()) {
		// 'All Pages'.
		return m_dpiCounts.haveUndefinedDpi();
	} else if (ptr == &m_allPagesNodeId) {
		// A size group.
		return m_sizes[row].dpiCounts().haveUndefinedDpi();
	} else if (ptr == &m_sizeGroupNodeId) {
		// An image.
		SizeGroup const& group = m_sizes[parent.row()];
		SizeGroup::Item const& item = group.items()[row];
		ImageFileInfo const& file = m_files[item.fileIdx];
		return file.imageInfo()[item.imageIdx].isUndefinedDpi();
	} else {
		// Should not happen.
		return false;
	}
}

void
FixDpiDialog::TreeModel::applyDpiToSelection(
	Scope const scope, Dpi const& dpi, QItemSelection const& selection)
{
	if (selection.isEmpty()) {
		return;
	}
	
	QModelIndex const parent(selection.front().parent());
	int const row = selection.front().top();
	void const* const ptr = parent.internalPointer();
	QModelIndex const idx(index(row, 0, parent));
	
	if (!parent.isValid()) {
		// Apply to all pages.
		applyDpiToAllGroups(scope, dpi);
		emitAllPagesChanged(idx);
	} else if (ptr == &m_allPagesNodeId) {
		// Apply to a size group.
		SizeGroup& group = m_sizes[row];
		applyDpiToGroup(scope, dpi, group, m_dpiCounts);
		emitSizeGroupChanged(index(row, 0, parent));
	} else if (ptr == &m_sizeGroupNodeId) {
		// Images within a size group.
		SizeGroup& group = m_sizes[parent.row()];
		SizeGroup::Item const& item = group.items()[row];
		applyDpiToItem(scope, dpi, item, m_dpiCounts, group.dpiCounts());
		emitItemChanged(idx);
	}
}

int
FixDpiDialog::TreeModel::columnCount(QModelIndex const& parent) const
{
	return 1;
}

int
FixDpiDialog::TreeModel::rowCount(QModelIndex const& parent) const
{
	void const* const ptr = parent.internalPointer();
	
	if (!parent.isValid()) {
		// The single 'All Pages' item.
		return 1;
	} else if (ptr == &m_allPagesNodeId) {
		// Size groups.
		return m_sizes.size();
	} else if (ptr == &m_sizeGroupNodeId) {
		// Images within a size group.
		return m_sizes[parent.row()].items().size();
	} else {
		// Children of an image.
		return 0;
	}
}

QModelIndex
FixDpiDialog::TreeModel::index(int const row, int const column, QModelIndex const& parent) const
{
	void const* const ptr = parent.internalPointer();
	
	if (!parent.isValid()) {
		// The 'All Pages' item.
		return createIndex(row, column, &m_allPagesNodeId);
	} else if (ptr == &m_allPagesNodeId) {
		// A size group.
		return createIndex(row, column, &m_sizeGroupNodeId);
	} else if (ptr == &m_sizeGroupNodeId) {
		// An image within some size group.
		return createIndex(row, column, (void*)&m_sizes[parent.row()]);
	}
	
	return QModelIndex();
}

QModelIndex
FixDpiDialog::TreeModel::parent(QModelIndex const& index) const
{
	void const* const ptr = index.internalPointer();
	
	if (!index.isValid()) {
		// Should not happen.
		return QModelIndex();
	} else if (ptr == &m_allPagesNodeId) {
		// 'All Pages' -> tree root.
		return QModelIndex();
	} else if (ptr == &m_sizeGroupNodeId) {
		// Size group -> 'All Pages'.
		return createIndex(0, index.column(), &m_allPagesNodeId);
	} else {
		// Image -> size group.
		SizeGroup const* group = static_cast<SizeGroup const*>(ptr);
		return createIndex(group - &m_sizes[0], index.column(), &m_sizeGroupNodeId);
	}
}

QVariant
FixDpiDialog::TreeModel::data(QModelIndex const& index, int const role) const
{
	void const* const ptr = index.internalPointer();
	
	if (!index.isValid()) {
		// Should not happen.
		return QVariant();
	} else if (ptr == &m_allPagesNodeId) {
		// 'All Pages'.
		if (role == Qt::DisplayRole) {
			return tr("All Pages");
		} else if (role == Qt::UserRole) {
			return m_dpiCounts.ifConsistentDpi().toSize();
		}
	} else if (ptr == &m_sizeGroupNodeId) {
		// Size group.
		SizeGroup const& group = m_sizes[index.row()];
		if (role == Qt::DisplayRole) {
			return sizeToString(group.size());
		} else if (role == Qt::UserRole) {
			return group.dpiCounts().ifConsistentDpi().toSize();
		}
	} else {
		// Image.
		SizeGroup const* group = static_cast<SizeGroup const*>(ptr);
		SizeGroup::Item const& item = group->items()[index.row()];
		ImageFileInfo const& file = m_files[item.fileIdx];
		if (role == Qt::DisplayRole) {
			QString const& fname = file.fileInfo().fileName();
			if (file.imageInfo().size() == 1) {
				return fname;
			} else {
				return tr("%1 (page %2)").arg(fname).arg(item.imageIdx + 1);
			}
		} else if (role == Qt::UserRole) {
			Dpi const dpi(file.imageInfo()[item.imageIdx].dpi());
			return dpi.toSize();
		}
	}
	
	return QVariant();
}

void
FixDpiDialog::TreeModel::applyDpiToAllGroups(Scope const scope, Dpi const& dpi)
{
	int const num_groups = m_sizes.size();
	for (int i = 0; i < num_groups; ++i) {
		applyDpiToGroup(scope, dpi, m_sizes[i], m_dpiCounts);
	}
}

void
FixDpiDialog::TreeModel::applyDpiToGroup(
	Scope const scope, Dpi const& dpi,
	SizeGroup& group, DpiCounts& total_dpi_counts)
{
	DpiCounts& group_dpi_counts = group.dpiCounts();
	std::vector<SizeGroup::Item> const& items = group.items();
	int const num_items = items.size();
	for (int i = 0; i < num_items; ++i) {
		applyDpiToItem(
			scope, dpi, items[i],
			total_dpi_counts, group_dpi_counts
		);
	}
}

void
FixDpiDialog::TreeModel::applyDpiToItem(
	Scope const scope, Dpi const& dpi, SizeGroup::Item const item,
	DpiCounts& total_dpi_counts, DpiCounts& group_dpi_counts)
{
	ImageFileInfo& file = m_files[item.fileIdx];
	ImageMetadata& image = file.imageInfo()[item.imageIdx];
	
	if (scope == UNDEFINED && !image.dpi().isNull()) {
		return;
	}
	
	total_dpi_counts.add(dpi);
	group_dpi_counts.add(dpi);
	total_dpi_counts.remove(image.dpi());
	group_dpi_counts.remove(image.dpi());
	
	image.setDpi(dpi);
}

void
FixDpiDialog::TreeModel::emitAllPagesChanged(QModelIndex const& idx)
{
	int const num_groups = m_sizes.size();
	for (int i = 0; i < num_groups; ++i) {
		QModelIndex const group_node(index(i, 0, idx));
		int const num_items = rowCount(group_node);
		for (int j = 0; j < num_items; ++j) {
			QModelIndex const image_node(index(j, 0, group_node));
			emit dataChanged(image_node, image_node);
		}
		emit dataChanged(group_node, group_node);
	}
	
	// The 'All Pages' node.
	emit dataChanged(idx, idx);
}

void
FixDpiDialog::TreeModel::emitSizeGroupChanged(QModelIndex const& idx)
{
	// Every item in this size group.
	emit dataChanged(index(0, 0, idx), index(rowCount(idx), 0, idx));
	
	// The size group itself.
	emit dataChanged(idx, idx);
	
	// The 'All Pages' node.
	QModelIndex const all_pages_node(idx.parent());
	emit dataChanged(all_pages_node, all_pages_node);
}

void
FixDpiDialog::TreeModel::emitItemChanged(QModelIndex const& idx)
{
	// The item itself.
	emit dataChanged(idx, idx);
	
	// The size group node.
	QModelIndex const group_node(idx.parent());
	emit dataChanged(group_node, group_node);
	
	// The 'All Pages' node.
	QModelIndex const all_pages_node(group_node.parent());
	emit dataChanged(all_pages_node, all_pages_node);
}

FixDpiDialog::SizeGroup&
FixDpiDialog::TreeModel::sizeGroupFor(QSize const size)
{
	using namespace boost::lambda;
	
	std::vector<SizeGroup>::iterator const it(
		std::find_if(
			m_sizes.begin(), m_sizes.end(),
			bind(&SizeGroup::size, _1) == size
		)
	);
	if (it != m_sizes.end()) {
		return *it;
	} else {
		m_sizes.push_back(SizeGroup(size));
		return m_sizes.back();
	}
}

QString
FixDpiDialog::TreeModel::sizeToString(QSize const size)
{
	return QString("%1 x %2 px").arg(size.width()).arg(size.height());
}

/*====================== FixDpiDialog::FilterModel ======================*/

FixDpiDialog::FilterModel::FilterModel(TreeModel& delegate)
:	m_rDelegate(delegate)
{
	setDynamicSortFilter(true);
	setSourceModel(delegate.model());
}

bool
FixDpiDialog::FilterModel::filterAcceptsRow(
	int const source_row, QModelIndex const& source_parent) const
{
	return m_rDelegate.isVisibleForFilter(source_parent, source_row);
}

QVariant
FixDpiDialog::FilterModel::data(QModelIndex const& index, int const role) const
{
	if (role == Qt::UserRole) {
		// FilterModel only displays items with undefined dpi.
		return QSize();
	}
	return QSortFilterProxyModel::data(index, role);
}
