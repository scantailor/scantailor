/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef NOFOCUSITEMDELEGATE_H_
#define NOFOCUSITEMDELEGATE_H_

#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QStyle>

/**
 * \brief A decoration of an existing item delegate suppresing focus visualization.
 */
template<typename T = QStyledItemDelegate>
class NoFocusItemDelegate : public T
{
public:
	NoFocusItemDelegate(QObject* parent = 0) : T(parent) {}
	
	virtual void paint(QPainter* painter,
			QStyleOptionViewItem const& option,
			QModelIndex const& index) const {
		if (option.state & QStyle::State_HasFocus) {
			QStyleOptionViewItem new_opt(option);
			new_opt.state &= ~QStyle::State_HasFocus;
			T::paint(painter, new_opt, index);
		} else {
			T::paint(painter, option, index);
		}
	}
};

#endif
