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

#include "TabbedImageView.h.moc"

namespace output
{

TabbedImageView::TabbedImageView(QWidget* parent)
:	QTabWidget(parent)
{
	connect(this, SIGNAL(currentChanged(int)), SLOT(tabChangedSlot(int)));
}

void
TabbedImageView::addTab(QWidget* widget, QString const& label, ImageViewTab tab)
{
	QTabWidget::addTab(widget, label);
	m_registry[widget] = tab;
}

void
TabbedImageView::setCurrentTab(ImageViewTab const tab)
{
	int const cnt = count();
	for (int i = 0; i < cnt; ++i) {
		QWidget* wgt = widget(i);
		std::map<QWidget*, ImageViewTab>::const_iterator it(m_registry.find(wgt));
		if (it != m_registry.end()) {
			if (it->second == tab) {
				setCurrentIndex(i);
				break;
			}
		}
	}
}

void
TabbedImageView::tabChangedSlot(int const idx)
{
	QWidget* wgt = widget(idx);
	std::map<QWidget*, ImageViewTab>::const_iterator it(m_registry.find(wgt));
	if (it != m_registry.end()) {
		emit tabChanged(it->second);
	}
}

} // namespace output

