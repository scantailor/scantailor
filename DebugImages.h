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

#ifndef DEBUG_IMAGES_H_
#define DEBUG_IMAGES_H_

#include "RefCountable.h"
#include "IntrusivePtr.h"
#include "AutoRemovingFile.h"
#include <QString>
#include <deque>

class QImage;

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
	void add(QImage const& image, QString const& label);
	
	void add(imageproc::BinaryImage const& image, QString const& label);
	
	bool empty() const { return m_sequence.empty(); }

	/**
	 * \brief Removes and returns the first item in the sequence.
	 *
	 * Returns a null AutoRemovingFile if image sequence is empty.
	 */
	AutoRemovingFile retrieveNext(QString* label = 0);
private:
	struct Item : public RefCountable
	{
		AutoRemovingFile file;
		QString label;

		Item(AutoRemovingFile f, QString const& l) : file(f), label(l) {}
	};

	std::deque<IntrusivePtr<Item> > m_sequence;
};

#endif
