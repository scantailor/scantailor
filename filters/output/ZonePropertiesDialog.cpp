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

#include "ZonePropertiesDialog.h.moc"

namespace output
{

ZonePropertiesDialog::ZonePropertiesDialog(
	int spline_idx, PictureZone::Type type, QWidget* parent)
:	QDialog(parent),
	m_splineIdx(spline_idx)
{
	ui.setupUi(this);

	switch (type) {
		case PictureZone::NO_OP:
			break;
		case PictureZone::ERASER1:
			ui.eraser1->setChecked(true);
			break;
		case PictureZone::PAINTER2:
			ui.painter2->setChecked(true);
			break;
		case PictureZone::ERASER3:
			ui.eraser3->setChecked(true);
			break;
	}

	connect(ui.eraser1, SIGNAL(toggled(bool)), SLOT(itemToggled(bool)));
	connect(ui.painter2, SIGNAL(toggled(bool)), SLOT(itemToggled(bool)));
	connect(ui.eraser3, SIGNAL(toggled(bool)), SLOT(itemToggled(bool)));
}

void
ZonePropertiesDialog::itemToggled(bool selected)
{
	PictureZone::Type type = PictureZone::NO_OP;

	QObject* const obj = sender();
	if (obj == ui.eraser1) {
		type = PictureZone::ERASER1;
	} else if (obj == ui.painter2) {
		type = PictureZone::PAINTER2;
	} else if (obj == ui.eraser3) {
		type = PictureZone::ERASER3;
	}

	emit typeChanged(m_splineIdx, type);
}

} // namespace output

