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

#ifndef CHANGEDSTATEITEMDELEGATE_H_
#define CHANGEDSTATEITEMDELEGATE_H_

#include <QStyledItemDelegate>
#include <QStyleOptionViewItem>
#include <QStyle>

/**
 * \brief A decoration of an existing item delegate
 *        that forces certain item states.
 */
template<typename T = QStyledItemDelegate>
class ChangedStateItemDelegate : public T
{
public:
	ChangedStateItemDelegate(QObject* parent = 0)
	: T(parent), m_changedFlags(), m_changedMask() {}
	
	void flagsForceEnabled(QStyle::State flags) {
		m_changedFlags |= flags;
		m_changedMask |= flags;
	}
	
	void flagsForceDisabled(QStyle::State flags) {
		m_changedFlags &= ~flags;
		m_changedMask |= flags;
	}
	
	void removeChanges(QStyle::State remove) {
		m_changedMask &= ~remove;
	}
	
	void removeAllChanges() {
		m_changedMask = QStyle::State();
	}
	
	virtual void paint(QPainter* painter,
			QStyleOptionViewItem const& option,
			QModelIndex const& index) const {
		
		QStyle::State const orig_state = option.state;
		
		QStyle::State const new_state = (orig_state & ~m_changedMask)
				| (m_changedFlags & m_changedMask);
		
		// Evil but necessary: the alternative solution of modifying
		// a copy doesn't work, as option doesn't really point to
		// QStyleOptionViewItem, but to one of its subclasses.
		QStyleOptionViewItem& non_const_opt = const_cast<QStyleOptionViewItem&>(option);
		
		non_const_opt.state = new_state;
		T::paint(painter, non_const_opt, index);
		non_const_opt.state = orig_state;
	}
private:
	QStyle::State m_changedFlags;
	QStyle::State m_changedMask;
};

#endif
