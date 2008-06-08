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

#ifndef PAGE_SPLIT_DEPENDENCIES_H_
#define PAGE_SPLIT_DEPENDENCIES_H_

#include "OrthogonalRotation.h"
#include <QSize>

class QString;
class QDomDocument;
class QDomElement;

namespace page_split
{

/**
 * \brief Dependencies of a page parameters.
 *
 * Once dependencies change, the stored page parameters are no longer valid.
 */
class Dependencies
{
	// Member-wise copying is OK.
public:
	Dependencies();
	
	Dependencies(QDomElement const& el);
	
	Dependencies(QSize const& image_size,
		OrthogonalRotation rotation, bool single_page);
	
	bool matches(Dependencies const& other) const;
	
	bool isNull() const { return m_imageSize.isNull(); }
	
	QDomElement toXml(QDomDocument& doc, QString const& tag_name) const;
private:
	QSize m_imageSize;
	OrthogonalRotation m_rotation;
	bool m_singlePage;
};

} // namespace page_split

#endif
