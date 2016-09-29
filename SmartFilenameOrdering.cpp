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

#include "SmartFilenameOrdering.h"
#include <QFileInfo>
#include <QRegExp>
#include <QString>

bool
SmartFilenameOrdering::operator()(QFileInfo const& lhs, QFileInfo const& rhs) const
{
	// First compare directories.
	if (int comp = lhs.absolutePath().compare(rhs.absolutePath())) {
		return comp < 0;
	}
	
	QString const lhs_fname(lhs.fileName());
	QString const rhs_fname(rhs.fileName());
	QChar const* lhs_ptr = lhs_fname.constData();
	QChar const* rhs_ptr = rhs_fname.constData();
	while (!lhs_ptr->isNull() && !rhs_ptr->isNull()) {
		bool const lhs_is_digit = lhs_ptr->isDigit();
		bool const rhs_is_digit = rhs_ptr->isDigit();
		if (lhs_is_digit != rhs_is_digit) {
			// Digits have priority over non-digits.
			return lhs_is_digit;
		}
		
		if (lhs_is_digit && rhs_is_digit) {
			unsigned long lhs_number = 0;
			do {
				lhs_number = lhs_number * 10 + lhs_ptr->digitValue();
				++lhs_ptr;
				// Note: isDigit() implies !isNull()
			} while (lhs_ptr->isDigit());
			
			unsigned long rhs_number = 0;
			do {
				rhs_number = rhs_number * 10 + rhs_ptr->digitValue();
				++rhs_ptr;
				// Note: isDigit() implies !isNull()
			} while (rhs_ptr->isDigit());
			
			if (lhs_number != rhs_number) {
				return lhs_number < rhs_number;
			} else {
				continue;
			}
		}
		
		if (lhs_ptr->isNull() != rhs_ptr->isNull()) {
			return *lhs_ptr < *rhs_ptr;
		}
		
		++lhs_ptr;
		++rhs_ptr;
	}
	
	if (!lhs_ptr->isNull() || !rhs_ptr->isNull()) {
		return lhs_ptr->isNull();
	}
	
	// OK, the smart comparison indicates the file names are equal.
	// However, if they aren't symbol-to-symbol equal, we can't treat
	// them as equal, so let's do a usual comparison now.
	return lhs_fname < rhs_fname;
}
