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

#include "LoadFileTask.h"
#include "filters/fix_orientation/Task.h"
#include "TaskStatus.h"
#include "FilterResult.h"
#include "ErrorWidget.h"
#include "FilterUiInterface.h"
#include "AbstractFilter.h"
#include "FilterOptionsWidget.h"
#include "ThumbnailPixmapCache.h"
#include "ProjectPages.h"
#include "PageInfo.h"
#include "Dpi.h"
#include "Dpm.h"
#include "FilterData.h"
#include "ImageLoader.h"
#include <QCoreApplication>
#include <QFile>
#include <QDir>
#include <QImage>
#include <QString>
#include <QTextDocument> // for Qt::escape()
#include <assert.h>

using namespace imageproc;

class LoadFileTask::ErrorResult : public FilterResult
{
	Q_DECLARE_TR_FUNCTIONS(LoadFileTask) 
public:
	ErrorResult(QString const& file_path);
	
	virtual void updateUI(FilterUiInterface* ui);
	
	virtual IntrusivePtr<AbstractFilter> filter() {
		return IntrusivePtr<AbstractFilter>();
	}
private:
	QString m_filePath;
	bool m_fileExists;
};


LoadFileTask::LoadFileTask(
	Type type, PageInfo const& page,
	IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
	IntrusivePtr<ProjectPages> const& pages,
	IntrusivePtr<fix_orientation::Task> const& next_task)
:	BackgroundTask(type),
	m_ptrThumbnailCache(thumbnail_cache),
	m_imageId(page.imageId()),
	m_imageMetadata(page.metadata()),
	m_ptrPages(pages),
	m_ptrNextTask(next_task)
{
	assert(m_ptrNextTask);
}

LoadFileTask::~LoadFileTask()
{
}

FilterResultPtr
LoadFileTask::operator()()
{
	QImage image(ImageLoader::load(m_imageId));
	
	try {
		throwIfCancelled();
		
		if (image.isNull()) {
			return FilterResultPtr(new ErrorResult(m_imageId.filePath()));
		} else {
			updateImageSizeIfChanged(image);
			overrideDpi(image);
			m_ptrThumbnailCache->ensureThumbnailExists(m_imageId, image);
			return m_ptrNextTask->process(*this, FilterData(image));
		}
	} catch (CancelledException const&) {
		return FilterResultPtr();
	}
}

void
LoadFileTask::updateImageSizeIfChanged(QImage const& image)
{
	// The user might just replace a file with another one.
	// In that case, we update its size that we store.
	// Note that we don't do the same about DPI, because
	// a DPI mismatch between the image and the stored value
	// may indicate that the DPI was overridden.
	// TODO: do something about DPIs when we have the ability
	// to change DPIs at any point in time (not just when
	// creating a project).
	if (image.size() != m_imageMetadata.size()) {
		m_imageMetadata.setSize(image.size());
		m_ptrPages->updateImageMetadata(m_imageId, m_imageMetadata);
	}
}

void
LoadFileTask::overrideDpi(QImage& image) const
{
	// Beware: QImage will have a default DPI when loading
	// an image that doesn't specify one.
	Dpm const dpm(m_imageMetadata.dpi());
	image.setDotsPerMeterX(dpm.horizontal());
	image.setDotsPerMeterY(dpm.vertical());
}


/*======================= LoadFileTask::ErrorResult ======================*/

LoadFileTask::ErrorResult::ErrorResult(QString const& file_path)
:	m_filePath(QDir::toNativeSeparators(file_path))
,	m_fileExists(QFile::exists(file_path))
{
}

void
LoadFileTask::ErrorResult::updateUI(FilterUiInterface* ui)
{
	class ErrWidget : public ErrorWidget
	{
	public:
		ErrWidget(IntrusivePtr<AbstractCommand0<void> > const& relinking_dialog_requester,
			QString const& text, Qt::TextFormat fmt = Qt::AutoText)
			: ErrorWidget(text, fmt), m_ptrRelinkingDialogRequester(relinking_dialog_requester) {}
	private:
		virtual void linkActivated(QString const&) {
			(*m_ptrRelinkingDialogRequester)();
		}

		IntrusivePtr<AbstractCommand0<void> > m_ptrRelinkingDialogRequester;
	};

	QString err_msg;
	Qt::TextFormat fmt = Qt::AutoText;
	if (m_fileExists) {
		err_msg = tr("The following file could not be loaded:\n%1").arg(m_filePath);
		fmt = Qt::PlainText;
	} else {
		err_msg = tr(
			"The following file doesn't exist:<br>%1<br>"
			"<br>"
			"Use the <a href=\"#relink\">Relinking Tool</a> to locate it."
		).arg(Qt::escape(m_filePath));
		fmt = Qt::RichText;
	}
	ui->setImageWidget(new ErrWidget(ui->relinkingDialogRequester(), err_msg, fmt), ui->TRANSFER_OWNERSHIP);
	ui->setOptionsWidget(new FilterOptionsWidget, ui->TRANSFER_OWNERSHIP);
}
