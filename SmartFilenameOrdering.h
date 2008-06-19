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

#ifndef SMARTFILENAMEORDERING_H_
#define SMARTFILENAMEORDERING_H_

class QFileInfo;

class SmartFilenameOrdering
{
public:
	SmartFilenameOrdering() {}
	
	/**
	 * \brief Compare filenames using a set of heuristic rules.
	 *
	 * This function tries to mimic the way humans would order filenames.
	 * For example, "2.png" will go before "12.png".  While doing so,
	 * it still provides the usual guarantees of a comparison predicate,
	 * such as two different file paths will never be ruled equivalent.
	 *
	 * \return true if \p lhs should go before \p rhs.
	 */
	bool operator()(QFileInfo const& lhs, QFileInfo const& rhs) const;
};

#endif
