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

#include "ProjectFilesDialog.h"
#include "ProjectFilesDialog.h.moc"
#include "NonCopyable.h"
#include "ImageMetadata.h"
#include "ImageMetadataLoader.h"
#include "SmartFilenameOrdering.h"
#include <QAbstractListModel>
#include <QSortFilterProxyModel>
#include <QModelIndex>
#include <QItemSelectionModel>
#include <QItemSelection>
#include <QItemSelectionRange>
#include <QFileDialog>
#include <QDir>
#include <QFileInfoList>
#include <QVariant>
#include <QVector>
#include <QVectorIterator>
#include <QMessageBox>
#include <QTimerEvent>
#include <QSettings>
#include <QBrush>
#include <QColor>
#include <QDebug>
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/construct.hpp>
#include <vector>
#include <deque>
#include <algorithm>
#include <utility>
#include <iterator>
#include <stddef.h>

class ProjectFilesDialog::Item
{
public:
	enum Status { STATUS_DEFAULT, STATUS_LOAD_OK, STATUS_LOAD_FAILED };
	
	Item(QFileInfo const& file_info, Qt::ItemFlags flags)
	: m_fileInfo(file_info), m_flags(flags), m_status(STATUS_DEFAULT) {}
	
	QFileInfo const& fileInfo() const { return m_fileInfo; }
	
	Qt::ItemFlags flags() const { return m_flags; }
	
	Status status() const { return m_status; }
	
	void setStatus(Status status) { m_status = status; }
	
	std::vector<ImageMetadata> const& perPageMetadata() const {
		return m_perPageMetadata;
	}
	
	std::vector<ImageMetadata>& perPageMetadata() {
		return m_perPageMetadata;
	}
private:
	QFileInfo m_fileInfo;
	Qt::ItemFlags m_flags;
	std::vector<ImageMetadata> m_perPageMetadata;
	Status m_status;
};


class ProjectFilesDialog::FileList : private QAbstractListModel
{
	DECLARE_NON_COPYABLE(FileList)
public:
	enum LoadStatus { LOAD_OK, LOAD_FAILED, NO_MORE_FILES };
	
	FileList();
	
	virtual ~FileList();
	
	QAbstractItemModel* model() { return this; }
	
	template<typename OutFunc>
	void files(OutFunc out) const;
	
	Item const& item(QModelIndex const& index) { return m_items[index.row()]; }
	
	template<typename OutFunc>
	void items(OutFunc out) const;
	
	template<typename OutFunc>
	void items(QItemSelection const& selection, OutFunc out) const;
	
	size_t count() const { return m_items.size(); }
	
	void clear();
	
	template<typename It>
	void append(It begin, It end);
	
	template<typename It>
	void assign(It begin, It end);
	
	void remove(QItemSelection const& selection);
	
	void prepareForLoadingFiles();
	
	LoadStatus loadNextFile();
private:
	virtual int rowCount(QModelIndex const& parent) const;
	
	virtual QVariant data(QModelIndex const& index, int role) const;
	
	virtual Qt::ItemFlags flags(QModelIndex const& index) const;
	
	std::vector<Item> m_items;
	std::deque<int> m_itemsToLoad;
};


class ProjectFilesDialog::SortedFileList : private QSortFilterProxyModel
{
	DECLARE_NON_COPYABLE(SortedFileList)
public:
	SortedFileList(FileList& delegate);
	
	QAbstractProxyModel* model() { return this; }
private:
	virtual bool lessThan(QModelIndex const& lhs, QModelIndex const& rhs) const;
	
	FileList& m_rDelegate;
};


class ProjectFilesDialog::ItemVisualOrdering
{
public:
	bool operator()(Item const& lhs, Item const& rhs) const;
};


template<typename OutFunc>
void
ProjectFilesDialog::FileList::files(OutFunc out) const
{
	std::vector<Item>::const_iterator it(m_items.begin());
	std::vector<Item>::const_iterator const end(m_items.end());
	for (; it != end; ++it) {
		out(it->fileInfo());
	}
}

template<typename OutFunc>
void
ProjectFilesDialog::FileList::items(OutFunc out) const
{
	std::for_each(m_items.begin(), m_items.end(), out);
}

template<typename OutFunc>
void
ProjectFilesDialog::FileList::items(QItemSelection const& selection, OutFunc out) const
{
	QListIterator<QItemSelectionRange> it(selection);
	while (it.hasNext()) {
		QItemSelectionRange const& range = it.next();
		for (int row = range.top(); row <= range.bottom(); ++row) {
			out(m_items[row]);
		}
	}
}

template<typename It>
void
ProjectFilesDialog::FileList::append(It begin, It end)
{
	if (begin == end) {
		return;
	}
	size_t const count = std::distance(begin, end);
	beginInsertRows(QModelIndex(), m_items.size(), m_items.size() + count - 1);
	m_items.insert(m_items.end(), begin, end);
	endInsertRows();
}

template<typename It>
void
ProjectFilesDialog::FileList::assign(It begin, It end)
{
	clear();
	append(begin, end);
}


ProjectFilesDialog::ProjectFilesDialog(QWidget* parent)
:	QDialog(parent),
	m_ptrOffProjectFiles(new FileList),
	m_ptrOffProjectFilesSorted(new SortedFileList(*m_ptrOffProjectFiles)),
	m_ptrInProjectFiles(new FileList),
	m_ptrInProjectFilesSorted(new SortedFileList(*m_ptrInProjectFiles)),
	m_loadTimerId(0),
	m_metadataLoadFailed(false),
	m_autoOutDir(true)
{
	m_supportedExtensions.insert("png");
	m_supportedExtensions.insert("jpg");
	m_supportedExtensions.insert("jpeg");
	m_supportedExtensions.insert("tif");
	m_supportedExtensions.insert("tiff");
	
	setupUi(this);
	offProjectList->setModel(m_ptrOffProjectFilesSorted->model());
	inProjectList->setModel(m_ptrInProjectFilesSorted->model());
	
	connect(inpDirBrowseBtn, SIGNAL(clicked()), this, SLOT(inpDirBrowse()));
	connect(outDirBrowseBtn, SIGNAL(clicked()), this, SLOT(outDirBrowse()));
	connect(
		inpDirLine, SIGNAL(textEdited(QString const&)),
		this, SLOT(inpDirEdited(QString const&))
	);
	connect(
		outDirLine, SIGNAL(textEdited(QString const&)),
		this, SLOT(outDirEdited(QString const&))
	);
	connect(addToProjectBtn, SIGNAL(clicked()), this, SLOT(addToProject()));
	connect(removeFromProjectBtn, SIGNAL(clicked()), this, SLOT(removeFromProject()));
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(onOK()));
}

ProjectFilesDialog::~ProjectFilesDialog()
{
}

QString
ProjectFilesDialog::inputDirectory() const
{
	return inpDirLine->text();
}

QString
ProjectFilesDialog::outputDirectory() const
{
	return outDirLine->text();
}

namespace
{

template<typename Item>
void pushFileInfo(std::vector<ImageFileInfo>& files, Item const& item)
{
	files.push_back(ImageFileInfo(item.fileInfo(), item.perPageMetadata()));
}

bool imageFileInfoLess(ImageFileInfo const& lhs, ImageFileInfo const& rhs)
{
	return SmartFilenameOrdering()(lhs.fileInfo(), rhs.fileInfo());
}

} // anonymous namespace

std::vector<ImageFileInfo>
ProjectFilesDialog::inProjectFiles() const
{
	using namespace boost;
	using namespace boost::lambda;
	
	std::vector<ImageFileInfo> files;
	m_ptrInProjectFiles->items(boost::lambda::bind(&pushFileInfo<Item>, boost::ref(files), _1));
	
	std::sort(files.begin(), files.end(), imageFileInfoLess);
	
	return files;
}

bool
ProjectFilesDialog::isRtlLayout() const
{
	return rtlLayoutCB->isChecked();
}

bool
ProjectFilesDialog::isDpiFixingForced() const
{
	return forceFixDpi->isChecked();
}

QString
ProjectFilesDialog::sanitizePath(QString const& path)
{
	QString trimmed(path.trimmed());
	if (trimmed.startsWith(QChar('"')) && trimmed.endsWith(QChar('"'))) {
		trimmed.chop(1);
		if (!trimmed.isEmpty()) {
			trimmed.remove(0, 1);
		}
	}
	return trimmed;
}

void
ProjectFilesDialog::inpDirBrowse()
{
	QSettings settings;
	
	QString initial_dir(inpDirLine->text());
	if (initial_dir.isEmpty() || !QDir(initial_dir).exists()) {
		initial_dir = settings.value("lastInputDir").toString();
	}
	if (initial_dir.isEmpty() || !QDir(initial_dir).exists()) {
		initial_dir = QDir::home().absolutePath();
	} else {
		QDir dir(initial_dir);
		if (dir.cdUp()) {
			initial_dir = dir.absolutePath();
		}
	}
	
	QString const dir(
		QFileDialog::getExistingDirectory(
			this, tr("Input Directory"), initial_dir
		)
	);
	
	if (!dir.isEmpty()) {
		setInputDir(dir);
		settings.setValue("lastInputDir", dir);
	}
}

void
ProjectFilesDialog::outDirBrowse()
{
	QString initial_dir(outDirLine->text());
	if (initial_dir.isEmpty() || !QDir(initial_dir).exists()) {
		initial_dir = QDir::home().absolutePath();
	}
	
	QString const dir(
		QFileDialog::getExistingDirectory(
			this, tr("Output Directory"), initial_dir
		)
	);
	
	if (!dir.isEmpty()) {
		setOutputDir(dir);
	}
}

void
ProjectFilesDialog::inpDirEdited(QString const& text)
{
	setInputDir(sanitizePath(text), /* auto_add_files= */false);
}

void
ProjectFilesDialog::outDirEdited(QString const& text)
{
	m_autoOutDir = false;
}

namespace
{

struct FileInfoLess
{
	bool operator()(QFileInfo const& lhs, QFileInfo const& rhs) const {
		if (lhs == rhs) {
			// This takes into account filesystem's case sensitivity.
			return false;
		}
		return lhs.absoluteFilePath() < rhs.absoluteFilePath();
	}
};

template<typename Item, typename ItemList>
void pushItemWithFlags(
	QFileInfo const& file, ItemList& items,
	QSet<QString> const& supported_extensions)
{
	Qt::ItemFlags flags;
	if (supported_extensions.contains(file.suffix().toLower())) {
		flags = Qt::ItemIsSelectable|Qt::ItemIsEnabled;
	}
	items.push_back(Item(file, flags));
}

} // anonymous namespace

void
ProjectFilesDialog::setInputDir(QString const& dir, bool const auto_add_files)
{
	using namespace boost;
	using namespace boost::lambda;
	
	inpDirLine->setText(QDir::toNativeSeparators(dir));
	if (m_autoOutDir) {
		setOutputDir(QDir::cleanPath(QDir(dir).filePath("out")));
	}
	
	QFileInfoList files(QDir(dir).entryInfoList(QDir::Files));
	
	{
		// Filter out files already in project.
		// Here we use simple ordering, which is OK.
		
		std::vector<QFileInfo> new_files(files.begin(), files.end());
		std::vector<QFileInfo> existing_files;
		void (std::vector<QFileInfo>::*push_back) (const QFileInfo&) =
			&std::vector<QFileInfo>::push_back;
		m_ptrInProjectFiles->files(
			boost::lambda::bind(push_back, var(existing_files), _1)
		);
		std::sort(new_files.begin(), new_files.end(), FileInfoLess());
		std::sort(existing_files.begin(), existing_files.end(), FileInfoLess());
		
		files.clear();
		std::set_difference(
			new_files.begin(), new_files.end(),
			existing_files.begin(), existing_files.end(),
			std::back_inserter(files), FileInfoLess()
		);
	}
	
	typedef std::vector<Item> ItemList;
	ItemList items;
	std::for_each(
		files.begin(), files.end(),
		boost::lambda::bind(
			&pushItemWithFlags<Item, ItemList>,
			_1, boost::ref(items), cref(m_supportedExtensions)
		)
	);
	
	m_ptrOffProjectFiles->assign(items.begin(), items.end());
	
	if (auto_add_files && m_ptrInProjectFiles->count() == 0) {
		offProjectList->selectAll();
		addToProject();
	}
}

void
ProjectFilesDialog::setOutputDir(QString const& dir)
{
	outDirLine->setText(QDir::toNativeSeparators(dir));
}

void
ProjectFilesDialog::addToProject()
{
	using namespace boost::lambda;
	
	QItemSelection const selection(
		m_ptrOffProjectFilesSorted->model()->mapSelectionToSource(
			offProjectList->selectionModel()->selection()
		)
	);
	
	typedef std::vector<Item> ItemList;
	ItemList items;
	
	void (ItemList::*push_back) (const Item&) = &ItemList::push_back;
	m_ptrOffProjectFiles->items(selection, boost::lambda::bind(push_back, var(items), _1));
	
	m_ptrInProjectFiles->append(items.begin(), items.end());
	m_ptrOffProjectFiles->remove(selection);
}

namespace
{

template<typename T, typename C>
void pushItemIfSameDir(C& items, T const& item, QDir const& dir)
{
	if (item.fileInfo().dir() == dir) {
		items.push_back(item);
	}
}

} // anonymous namespace

void
ProjectFilesDialog::removeFromProject()
{
	using namespace boost;
	using namespace boost::lambda;
	
	QDir const input_dir(inpDirLine->text());
	
	QItemSelection const selection(
		m_ptrInProjectFilesSorted->model()->mapSelectionToSource(
			inProjectList->selectionModel()->selection()
		)
	);
	
	typedef std::vector<Item> ItemList;
	ItemList items;
	
	m_ptrInProjectFiles->items(
		selection, boost::lambda::bind(
			&pushItemIfSameDir<Item, ItemList>,
			boost::ref(items), _1, cref(input_dir)
		)
	);
	
	m_ptrOffProjectFiles->append(items.begin(), items.end());
	m_ptrInProjectFiles->remove(selection);
}

void
ProjectFilesDialog::onOK()
{
	if (m_ptrInProjectFiles->count() == 0) {
		QMessageBox::warning(
			this, tr("Error"), tr("No files in project!")
		);
		return;
	}
	
	QDir const inp_dir(inpDirLine->text());
	if (!inp_dir.isAbsolute() || !inp_dir.exists()) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("Input directory is not set or doesn't exist.")
		);
		return;
	}
	
	QDir const out_dir(outDirLine->text());
	if (inp_dir == out_dir) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("Input and output directories can't be the same.")
		);
		return;
	}
	
	if (out_dir.isAbsolute() && !out_dir.exists()) {
		// Maybe create it.
		bool create = m_autoOutDir;
		if (!m_autoOutDir) {
			create = QMessageBox::question(
				this, tr("Create Directory?"),
				tr("Output directory doesn't exist.  Create it?"),
				QMessageBox::Yes|QMessageBox::No
			) == QMessageBox::Yes;
			if (!create) {
				return;
			}
		}
		if (create) {
			if (!out_dir.mkpath(out_dir.path())) {
				QMessageBox::warning(
					this, tr("Error"),
					tr("Unable to create output directory.")
				);
				return;
			}
		}
	}
	if (!out_dir.isAbsolute() || !out_dir.exists()) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("Output directory is not set or doesn't exist.")
		);
		return;
	}
	
	startLoadingMetadata();
}

void
ProjectFilesDialog::startLoadingMetadata()
{
	m_ptrInProjectFiles->prepareForLoadingFiles();
	
	progressBar->setMaximum(m_ptrInProjectFiles->count());
	inpDirLine->setEnabled(false);
	inpDirBrowseBtn->setEnabled(false);
	outDirLine->setEnabled(false);
	outDirBrowseBtn->setEnabled(false);
	addToProjectBtn->setEnabled(false);
	removeFromProjectBtn->setEnabled(false);
	offProjectSelectAllBtn->setEnabled(false);
	inProjectSelectAllBtn->setEnabled(false);
	rtlLayoutCB->setEnabled(false);
	forceFixDpi->setEnabled(false);
	buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
	offProjectList->clearSelection();
	inProjectList->clearSelection();
	m_loadTimerId = startTimer(0);
	m_metadataLoadFailed = false;
}

void
ProjectFilesDialog::timerEvent(QTimerEvent* event)
{
	if (event->timerId() != m_loadTimerId) {
		QWidget::timerEvent(event);
		return;
	}
	
	switch (m_ptrInProjectFiles->loadNextFile()) {
		case FileList::NO_MORE_FILES:
			finishLoadingMetadata();
			break;
		case FileList::LOAD_FAILED:
			m_metadataLoadFailed = true;
			// Fall through.
		case FileList::LOAD_OK:
			progressBar->setValue(progressBar->value() + 1);
			break;
	}
}

void
ProjectFilesDialog::finishLoadingMetadata()
{
	killTimer(m_loadTimerId);
	
	inpDirLine->setEnabled(true);
	inpDirBrowseBtn->setEnabled(true);
	outDirLine->setEnabled(true);
	outDirBrowseBtn->setEnabled(true);
	addToProjectBtn->setEnabled(true);
	removeFromProjectBtn->setEnabled(true);
	offProjectSelectAllBtn->setEnabled(true);
	inProjectSelectAllBtn->setEnabled(true);
	rtlLayoutCB->setEnabled(true);
	forceFixDpi->setEnabled(true);
	buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
	
	if (m_metadataLoadFailed) {
		progressBar->setValue(0);
		QMessageBox::warning(
			this, tr("Error"), tr(
				"Some of the files failed to load.\n"
				"Either we don't support their format, or they are broken.\n"
				"You should remove them from the project."
			)
		);
		return;
	}
	
	accept();
}

/*====================== ProjectFilesDialog::FileList ====================*/

ProjectFilesDialog::FileList::FileList()
{
}

ProjectFilesDialog::FileList::~FileList()
{
}

void
ProjectFilesDialog::FileList::clear()
{
	if (m_items.empty()) {
		return;
	}
	beginRemoveRows(QModelIndex(), 0, m_items.size() - 1);
	m_items.clear();
	endRemoveRows();
}

void
ProjectFilesDialog::FileList::remove(QItemSelection const& selection)
{
	using namespace boost::lambda;
	
	if (selection.isEmpty()) {
		return;
	}
	
	typedef std::pair<int, int> Range;
	QVector<Range> sorted_ranges;
	std::transform(
		selection.begin(), selection.end(),
		std::back_inserter(sorted_ranges),
		boost::lambda::bind(
			constructor<Range>(),
			boost::lambda::bind(&QItemSelectionRange::top, _1),
			boost::lambda::bind(&QItemSelectionRange::bottom, _1)
		)
	);
	
	// This hack is required to make it build with boost 1.44.
	typedef int const Range::* IntMemPtr;
	
	std::sort(
		sorted_ranges.begin(), sorted_ranges.end(),
		boost::lambda::bind((IntMemPtr)&Range::first, _1) <
			boost::lambda::bind((IntMemPtr)&Range::first, _2)
	);
	
	QVectorIterator<Range> it(sorted_ranges);
	int rows_removed = 0;
	while (it.hasNext()) {
		Range const& range = it.next();
		int const first = range.first - rows_removed;
		int const last = range.second - rows_removed;
		beginRemoveRows(QModelIndex(), first, last);
		m_items.erase(m_items.begin() + first, m_items.begin() + (last + 1));
		endRemoveRows();
		rows_removed += last - first + 1;
	}
}

int
ProjectFilesDialog::FileList::rowCount(QModelIndex const&) const
{
	return m_items.size();
}

QVariant
ProjectFilesDialog::FileList::data(QModelIndex const& index, int const role) const
{
	Item const& item = m_items[index.row()];
	switch (role) {
		case Qt::DisplayRole:
			return item.fileInfo().fileName();
		case Qt::ForegroundRole:
			switch (item.status()) {
				case Item::STATUS_DEFAULT:
					return QVariant();
				case Item::STATUS_LOAD_OK:
					return QBrush(QColor(0x00, 0xff, 0x00));
				case Item::STATUS_LOAD_FAILED:
					return QBrush(QColor(0xff, 0x00, 0x00));
			}
			break;
	}
	return QVariant();
}

Qt::ItemFlags
ProjectFilesDialog::FileList::flags(QModelIndex const& index) const
{
	return m_items[index.row()].flags();
}

void
ProjectFilesDialog::FileList::prepareForLoadingFiles()
{
	using namespace boost::lambda;
	
	std::deque<int> item_indexes;
	int const num_items = m_items.size();
	for (int i = 0; i < num_items; ++i) {
		item_indexes.push_back(i);
	}
	
	std::sort(
		item_indexes.begin(), item_indexes.end(),
		boost::lambda::bind(
			&ItemVisualOrdering::operator(), ItemVisualOrdering(),
			var(m_items)[_1], var(m_items)[_2]
		)
	);
	
	m_itemsToLoad.swap(item_indexes);
}

ProjectFilesDialog::FileList::LoadStatus
ProjectFilesDialog::FileList::loadNextFile()
{
	using namespace boost::lambda;
	
	if (m_itemsToLoad.empty()) {
		return NO_MORE_FILES;
	}
	
	int const item_idx = m_itemsToLoad.front();
	Item& item = m_items[item_idx];
	std::vector<ImageMetadata> per_page_metadata;
	QString const file_path(item.fileInfo().absoluteFilePath());
	void (std::vector<ImageMetadata>::*push_back) (const ImageMetadata&) =
		&std::vector<ImageMetadata>::push_back;
	ImageMetadataLoader::Status const st = ImageMetadataLoader::load(
		file_path, boost::lambda::bind(
			push_back, var(per_page_metadata), _1
		)
	);
	
	LoadStatus status;
	
	if (st == ImageMetadataLoader::LOADED) {
		status = LOAD_OK;
		item.perPageMetadata().swap(per_page_metadata);
		item.setStatus(Item::STATUS_LOAD_OK);
	} else {
		status = LOAD_FAILED;
		item.setStatus(Item::STATUS_LOAD_FAILED);
	}
	QModelIndex const idx(index(item_idx, 0));
	emit dataChanged(idx, idx);
	
	m_itemsToLoad.pop_front();
	
	return status;
}


/*================= ProjectFilesDialog::SortedFileList ===================*/

ProjectFilesDialog::SortedFileList::SortedFileList(FileList& delegate)
:	m_rDelegate(delegate)
{
	setSourceModel(delegate.model());
	setDynamicSortFilter(true);
	sort(0);
}

bool
ProjectFilesDialog::SortedFileList::lessThan(
	QModelIndex const& lhs, QModelIndex const& rhs) const
{
	Item const& lhs_item = m_rDelegate.item(lhs);
	Item const& rhs_item = m_rDelegate.item(rhs);
	
	return ItemVisualOrdering()(lhs_item, rhs_item);
}


/*=============== ProjectFilesDialog::ItemVisualOrdering =================*/

bool
ProjectFilesDialog::ItemVisualOrdering::operator()(
	Item const& lhs, Item const& rhs) const
{
	bool const lhs_failed = (lhs.status() == Item::STATUS_LOAD_FAILED);
	bool const rhs_failed = (rhs.status() == Item::STATUS_LOAD_FAILED);
	if (lhs_failed != rhs_failed) {
		// Failed ones go to the top.
		return lhs_failed;
	}
	
	return SmartFilenameOrdering()(lhs.fileInfo(), rhs.fileInfo());
}
