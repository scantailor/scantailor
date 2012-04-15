/*
	Scan Tailor - Interactive post-processing tool for scanned pages.
	Copyright (C)  Joseph Artsimovich <joseph_a@mail.ru>

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

#include "DebugImageView.h"
#include "AbstractCommand.h"
#include "BackgroundExecutor.h"
#include "ImageViewBase.h"
#include "BasicImageView.h"
#include "ProcessingIndicationWidget.h"
#include <QImage>
#include <QPointer>
#include <memory>

class DebugImageView::ImageLoadResult : public AbstractCommand0<void>
{
public:
	ImageLoadResult(QPointer<DebugImageView> const& owner, QImage const& image)
			: m_ptrOwner(owner), m_image(image) {}

	// This method is called from the main thread.
	virtual void operator()() {
		if (DebugImageView* owner = m_ptrOwner) {
			owner->imageLoaded(m_image);
		}
	}
private:
	QPointer<DebugImageView> m_ptrOwner;
	QImage m_image;
};


class DebugImageView::ImageLoader :
	public AbstractCommand0<BackgroundExecutor::TaskResultPtr>
{
public:
	ImageLoader(DebugImageView* owner, QString const& file_path)
			: m_ptrOwner(owner), m_filePath(file_path) {}

	virtual BackgroundExecutor::TaskResultPtr operator()() {
		QImage image(m_filePath);
		return BackgroundExecutor::TaskResultPtr(new ImageLoadResult(m_ptrOwner, image));
	}
private:
	QPointer<DebugImageView> m_ptrOwner;
	QString m_filePath;
};


DebugImageView::DebugImageView(AutoRemovingFile file,
	boost::function<QWidget* (QImage const&)> const& image_view_factory, QWidget* parent)
:	QStackedWidget(parent),
	m_file(file),
	m_imageViewFactory(image_view_factory),
	m_pPlaceholderWidget(new ProcessingIndicationWidget(this)),
	m_isLive(false)
{
	addWidget(m_pPlaceholderWidget);
}

void
DebugImageView::setLive(bool const live)
{
	if (live && !m_isLive) {
		ImageViewBase::backgroundExecutor().enqueueTask(
			BackgroundExecutor::TaskPtr(new ImageLoader(this, m_file.get()))
		);
	} else if (!live && m_isLive) {
		if (QWidget* wgt = currentWidget()) {
			if (wgt != m_pPlaceholderWidget) {
				removeWidget(wgt);
				delete wgt;
			}
		}
	}

	m_isLive = live;
}

void
DebugImageView::imageLoaded(QImage const& image)
{
	if (!m_isLive) {
		return;
	}

	if (currentWidget() == m_pPlaceholderWidget) {
		std::auto_ptr<QWidget> image_view;
		if (m_imageViewFactory.empty()) {
			image_view.reset(new BasicImageView(image));
		} else {
			image_view.reset(m_imageViewFactory(image));
		}
		setCurrentIndex(addWidget(image_view.release()));
	}
}
