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

#ifndef DEBUG_IMAGE_VIEW_H_
#define DEBUG_IMAGE_VIEW_H_

#include "AutoRemovingFile.h"
#include <QStackedWidget>
#include <QWidget>
#include <boost/intrusive/list.hpp>
#include <boost/function.hpp>

class QImage;

class DebugImageView :
	public QStackedWidget,
	public boost::intrusive::list_base_hook<
		boost::intrusive::link_mode<boost::intrusive::auto_unlink>
	>
{
public:
	DebugImageView(AutoRemovingFile file,
		boost::function<QWidget* (QImage const&)> const& image_view_factory =
		boost::function<QWidget* (QImage const&)>(), QWidget* parent = 0);

	/**
	 * Tells this widget to either display the actual image or just
	 * a placeholder.
	 */
	void setLive(bool live);
private:
	class ImageLoadResult;
	class ImageLoader;

	void imageLoaded(QImage const& image);

	AutoRemovingFile m_file;
	boost::function<QWidget* (QImage const&)> m_imageViewFactory;
	QWidget* m_pPlaceholderWidget;
	bool m_isLive;
};

#endif
