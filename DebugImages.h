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

#ifndef DEBUG_IMAGES_H_
#define DEBUG_IMAGES_H_

#include "RefCountable.h"
#include "IntrusivePtr.h"
#include "AutoRemovingFile.h"
#include <boost/function.hpp>
#include <QString>
#include <deque>

class QImage;
class QWidget;

namespace imageproc
{
	class BinaryImage;
}

/**
 * \brief A sequence of image + label pairs.
 */
class DebugImages
{
public:
	void add(QImage const& image, QString const& label,
		boost::function<QWidget* (QImage const&)> const& image_view_factory =
		boost::function<QWidget* (QImage const&)>());
	
	void add(imageproc::BinaryImage const& image, QString const& label,
		boost::function<QWidget* (QImage const&)> const& image_view_factory =
		boost::function<QWidget* (QImage const&)>());
	
	bool empty() const { return m_sequence.empty(); }

	/**
	 * \brief Removes and returns the first item in the sequence.
	 *
	 * The label and viewer widget factory (that may not be bound)
	 * are returned by taking pointers to them as arguments.
	 * Returns a null AutoRemovingFile if image sequence is empty.
	 */
	AutoRemovingFile retrieveNext(QString* label = 0,
		boost::function<QWidget* (QImage const&)>* image_view_factory = 0);
private:
	struct Item : public RefCountable
	{
		AutoRemovingFile file;
		QString label;
		boost::function<QWidget* (QImage const&)> imageViewFactory;

		Item(AutoRemovingFile f, QString const& l,
			boost::function<QWidget* (QImage const&)> const& imf)
		:	file(f), label(l), imageViewFactory(imf) {}
	};

	std::deque<IntrusivePtr<Item> > m_sequence;
};

#endif
