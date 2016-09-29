/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C)  Joseph Artsimovich <joseph.artsimovich@gmail.com>

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

#include "FixDpiDialog.h"
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
#include <QColor>
#include <Qt>
#include <QDebug>
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#endif
#include <vector>
#include <algorithm>
#include <assert.h>

// To be able to use it in QVariant
Q_DECLARE_METATYPE(ImageMetadata)

static int const NEED_FIXING_TAB = 0;
static int const ALL_PAGES_TAB = 1;

// Requests a group of ImageMetadata objects folded into one.
static int const AGGREGATE_METADATA_ROLE = Qt::UserRole;

// Same as the one above, but only objects with .isDpiOK() == false
// will be considered.
static int const AGGREGATE_NOT_OK_METADATA_ROLE = Qt::UserRole + 1;


/**
 * This class computes an aggregate ImageMetadata object from a group of other
 * ImageMetadata objects.  If all ImageMetadata objects in a group are equal,
 * that will make it an aggregate metadata.  Otherwise, a null (default
 * constructed) ImageMetadata() object will be considered
 * the DPIs within the group are not consistent,
 * the aggregate Image metadata object will have zeros both for size and for
 * DPI values.  If the DPIs are consistent but sizes are not, the aggregate
 * ImageMetadata will have the consistent DPI and zero size.
 */
class FixDpiDialog::DpiCounts
{
public:
	void add(ImageMetadata const& metadata);
	
	void remove(ImageMetadata const& metadata);
	
	/**
	 * Checks if all ImageMetadata objects return true for ImageMetadata::isDpiOK().
	 */
	bool allDpisOK() const;
	
	/**
	 * If all ImageMetadata objects are equal, one of them will be returned.
	 * Otherwise, a default-constructed ImageMetadata() object will be returned.
	 */
	ImageMetadata aggregate(Scope scope) const;
private:
	struct MetadataComparator
	{
		bool operator()(ImageMetadata const& lhs, ImageMetadata const& rhs) const;
	};
	
	typedef std::map<ImageMetadata, int, MetadataComparator> Map;
	
	Map m_counts;
};


/**
 * This comparator puts objects that are not OK to the front.
 */
bool FixDpiDialog::DpiCounts::MetadataComparator::operator()(
	ImageMetadata const& lhs, ImageMetadata const& rhs) const
{
	bool const lhs_ok = lhs.isDpiOK();
	bool const rhs_ok = rhs.isDpiOK();
	if (lhs_ok != rhs_ok) {
		return rhs_ok;
	}
	
	if (lhs.size().width() < rhs.size().width()) {
		return true;
	} else if (lhs.size().width() > rhs.size().width()) {
		return false;
	} else if (lhs.size().height() < rhs.size().height()) {
		return true;
	} else if (lhs.size().height() > rhs.size().height()) {
		return false;
	} else if (lhs.dpi().horizontal() < rhs.dpi().horizontal()) {
		return true;
	} else if (lhs.dpi().horizontal() > rhs.dpi().horizontal()) {
		return false;
	} else {
		return lhs.dpi().vertical() < rhs.dpi().vertical();
	}
}


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
	
	void append(Item const& item, ImageMetadata const& metadata);
	
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
	TreeModel(std::vector<ImageFileInfo> const& files);
	
	std::vector<ImageFileInfo> const& files() const { return m_files; }
	
	QAbstractItemModel* model() { return this; }
	
	bool allDpisOK() const {
		return m_dpiCounts.allDpisOK();
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
		Scope scope, ImageMetadata const& new_metadata, SizeGroup::Item item,
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
	
	m_normalPalette = xDpi->palette();
	m_errorPalette = m_normalPalette;
	m_errorPalette.setColor(QPalette::Text, Qt::red);
	
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
	xDpi->setValidator(new QIntValidator(xDpi));
	yDpi->setValidator(new QIntValidator(yDpi));
	
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
	
	Dpi const dpi(xDpi->text().toInt(), yDpi->text().toInt());
	ImageMetadata const metadata(m_selectedItemPixelSize, dpi);
	
	decorateDpiInputField(xDpi, metadata.horizontalDpiStatus());
	decorateDpiInputField(yDpi, metadata.verticalDpiStatus());
	
	if (m_xDpiInitialValue == xDpi->text() &&
			m_yDpiInitialValue == yDpi->text()) {
		applyBtn->setEnabled(false);
		return;
	}
	
	if (metadata.isDpiOK()) {
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
		m_ptrPages->applyDpiToSelection(ALL, dpi, selection);
	} else {
		selection_model = undefinedDpiView->selectionModel();
		QItemSelection const selection(
			m_ptrUndefinedDpiPages->model()->mapSelectionToSource(
				selection_model->selection()
			)
		);
		m_ptrPages->applyDpiToSelection(NOT_OK, dpi, selection);
	}
	
	updateDpiFromSelection(selection_model->selection());
	enableDisableOkButton();
}

void
FixDpiDialog::enableDisableOkButton()
{
	bool const enable = m_ptrPages->allDpisOK();
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
	
	// FilterModel may replace AGGREGATE_METADATA_ROLE with AGGREGATE_NOT_OK_METADATA_ROLE.
	QVariant const data(selection.front().topLeft().data(AGGREGATE_METADATA_ROLE));
	if (data.isValid()) {
		setDpiForm(data.value<ImageMetadata>());
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
FixDpiDialog::setDpiForm(ImageMetadata const& metadata)
{
	Dpi const dpi(metadata.dpi());
	
	if (dpi.isNull()) {
		resetDpiForm();
		return;
	}
	
	m_xDpiInitialValue = QString::number(dpi.horizontal());
	m_yDpiInitialValue = QString::number(dpi.vertical());
	m_selectedItemPixelSize = metadata.size();
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

void
FixDpiDialog::decorateDpiInputField(QLineEdit* field, ImageMetadata::DpiStatus dpi_status) const
{
	if (dpi_status == ImageMetadata::DPI_OK) {
		field->setPalette(m_normalPalette);
	} else {
		field->setPalette(m_errorPalette);
	}

	switch (dpi_status) {
		case ImageMetadata::DPI_OK:
		case ImageMetadata::DPI_UNDEFINED:
			field->setToolTip(QString());
			break;
		case ImageMetadata::DPI_TOO_LARGE:
			field->setToolTip(tr("DPI is too large and most likely wrong."));
			break;
		case ImageMetadata::DPI_TOO_SMALL:
			field->setToolTip(tr("DPI is too small. Even if it's correct, you are not going to get acceptable results with it."));
			break;
		case ImageMetadata::DPI_TOO_SMALL_FOR_THIS_PIXEL_SIZE:
			field->setToolTip(tr("DPI is too small for this pixel size. Such combination would probably lead to out of memory errors."));
			break;
	}
}


/*====================== FixDpiDialog::DpiCounts ======================*/

void
FixDpiDialog::DpiCounts::add(ImageMetadata const& metadata)
{
	++m_counts[metadata];
}

void
FixDpiDialog::DpiCounts::remove(ImageMetadata const& metadata)
{
	if (--m_counts[metadata] == 0) {
		m_counts.erase(metadata);
	}
}

bool
FixDpiDialog::DpiCounts::allDpisOK() const
{
	// We put wrong DPIs to the front, so if the first one is OK,
	// the others are OK as well.
	Map::const_iterator const it(m_counts.begin());
	return (it == m_counts.end() || it->first.isDpiOK());
}

ImageMetadata
FixDpiDialog::DpiCounts::aggregate(Scope const scope) const
{
	Map::const_iterator const it(m_counts.begin());
	
	if (it == m_counts.end()) {
		return ImageMetadata();
	}
	
	if (scope == NOT_OK && it->first.isDpiOK()) {
		// If this one is OK, the following ones are OK as well.
		return ImageMetadata();
	}
	
	Map::const_iterator next(it);
	++next;
	
	if (next == m_counts.end()) {
		return it->first;
	}
	
	if (scope == NOT_OK && next->first.isDpiOK()) {
		// If this one is OK, the following ones are OK as well.
		return it->first;
	}
	
	return ImageMetadata();
}



/*====================== FixDpiDialog::SizeGroup ======================*/

void
FixDpiDialog::SizeGroup::append(Item const& item, ImageMetadata const& metadata)
{
	m_items.push_back(item);
	m_dpiCounts.add(metadata);
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
			group.append(SizeGroup::Item(i, j), metadata);
			m_dpiCounts.add(metadata);
		}
	}
}

bool
FixDpiDialog::TreeModel::isVisibleForFilter(QModelIndex const& parent, int row) const
{
	void const* const ptr = parent.internalPointer();
	
	if (!parent.isValid()) {
		// 'All Pages'.
		return !m_dpiCounts.allDpisOK();
	} else if (ptr == &m_allPagesNodeId) {
		// A size group.
		return !m_sizes[row].dpiCounts().allDpisOK();
	} else if (ptr == &m_sizeGroupNodeId) {
		// An image.
		SizeGroup const& group = m_sizes[parent.row()];
		SizeGroup::Item const& item = group.items()[row];
		ImageFileInfo const& file = m_files[item.fileIdx];
		return !file.imageInfo()[item.imageIdx].isDpiOK();
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
		ImageMetadata const metadata(group.size(), dpi);
		applyDpiToItem(scope, metadata, item, m_dpiCounts, group.dpiCounts());
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
			return FixDpiDialog::tr("All Pages");
		} else if (role == AGGREGATE_METADATA_ROLE) {
			return QVariant::fromValue(m_dpiCounts.aggregate(ALL));
		} else if (role == AGGREGATE_NOT_OK_METADATA_ROLE) {
			return QVariant::fromValue(m_dpiCounts.aggregate(NOT_OK));
		}
	} else if (ptr == &m_sizeGroupNodeId) {
		// Size group.
		SizeGroup const& group = m_sizes[index.row()];
		if (role == Qt::DisplayRole) {
			return sizeToString(group.size());
		} else if (role == AGGREGATE_METADATA_ROLE) {
			return QVariant::fromValue(group.dpiCounts().aggregate(ALL));
		} else if (role == AGGREGATE_NOT_OK_METADATA_ROLE) {
			return QVariant::fromValue(group.dpiCounts().aggregate(NOT_OK));
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
				return FixDpiDialog::tr(
					"%1 (page %2)"
				).arg(fname).arg(item.imageIdx + 1);
			}
		} else if (role == AGGREGATE_METADATA_ROLE || role == AGGREGATE_NOT_OK_METADATA_ROLE) {
			return QVariant::fromValue(file.imageInfo()[item.imageIdx]);
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
	ImageMetadata const metadata(group.size(), dpi);
	std::vector<SizeGroup::Item> const& items = group.items();
	int const num_items = items.size();
	for (int i = 0; i < num_items; ++i) {
		applyDpiToItem(
			scope, metadata, items[i],
			total_dpi_counts, group_dpi_counts
		);
	}
}

void
FixDpiDialog::TreeModel::applyDpiToItem(
	Scope const scope, ImageMetadata const& new_metadata, SizeGroup::Item const item,
	DpiCounts& total_dpi_counts, DpiCounts& group_dpi_counts)
{
	ImageFileInfo& file = m_files[item.fileIdx];
	ImageMetadata& old_metadata = file.imageInfo()[item.imageIdx];
	
	if (scope == NOT_OK && old_metadata.isDpiOK()) {
		return;
	}
	
	total_dpi_counts.add(new_metadata);
	group_dpi_counts.add(new_metadata);
	total_dpi_counts.remove(old_metadata);
	group_dpi_counts.remove(old_metadata);
	
	old_metadata = new_metadata;
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
FixDpiDialog::FilterModel::data(QModelIndex const& index, int role) const
{
	if (role == AGGREGATE_METADATA_ROLE) {
		role = AGGREGATE_NOT_OK_METADATA_ROLE;
	}
	return QSortFilterProxyModel::data(index, role);
}
