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

#include "DebugImages.h"
#include "imageproc/BinaryImage.h"
#include <QImage>
#include <QImageWriter>
#include <QTemporaryFile>
#include <QDir>

void
DebugImages::add(
	QImage const& image, QString const& label,
	boost::function<QWidget* (QImage const&)> const& image_view_factory)
{
	QTemporaryFile file(QDir::tempPath()+"/scantailor-dbg-XXXXXX.png");
	if (!file.open()) {
		return;
	}

	AutoRemovingFile arem_file(file.fileName());
	file.setAutoRemove(false);

	QImageWriter writer(&file, "png");
	writer.setCompression(2); // Trade space for speed.
	if (!writer.write(image)) {
		return;
	}

	m_sequence.push_back(IntrusivePtr<Item>(new Item(arem_file, label, image_view_factory)));
}

void
DebugImages::add(
	imageproc::BinaryImage const& image, QString const& label,
	boost::function<QWidget* (QImage const&)> const& image_view_factory)
{
	add(image.toQImage(), label, image_view_factory);
}

AutoRemovingFile
DebugImages::retrieveNext(QString* label, boost::function<QWidget* (QImage const&)>* image_view_factory)
{
	if (m_sequence.empty()) {
		return AutoRemovingFile();
	}

	AutoRemovingFile file(m_sequence.front()->file);
	if (label) {
		*label = m_sequence.front()->label;
	}
	if (image_view_factory) {
		*image_view_factory = m_sequence.front()->imageViewFactory;
	}

	m_sequence.pop_front();

	return file;
}
