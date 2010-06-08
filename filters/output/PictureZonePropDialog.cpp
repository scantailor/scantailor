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

#include "PictureZonePropDialog.h"
#include "PictureZonePropDialog.h.moc"
#include "Property.h"
#include "PictureLayerProperty.h"

namespace output
{

PictureZonePropDialog::PictureZonePropDialog(
	IntrusivePtr<PropertySet> const& props, QWidget* parent)
:	QDialog(parent),
	m_ptrProps(props)
{
	ui.setupUi(this);

	switch (m_ptrProps->locateOrDefault<PictureLayerProperty>()->layer()) {
		case PictureLayerProperty::NO_OP:
			break;
		case PictureLayerProperty::ERASER1:
			ui.eraser1->setChecked(true);
			break;
		case PictureLayerProperty::PAINTER2:
			ui.painter2->setChecked(true);
			break;
		case PictureLayerProperty::ERASER3:
			ui.eraser3->setChecked(true);
			break;
	}

	connect(ui.eraser1, SIGNAL(toggled(bool)), SLOT(itemToggled(bool)));
	connect(ui.painter2, SIGNAL(toggled(bool)), SLOT(itemToggled(bool)));
	connect(ui.eraser3, SIGNAL(toggled(bool)), SLOT(itemToggled(bool)));
}

void
PictureZonePropDialog::itemToggled(bool selected)
{
	PictureLayerProperty::Layer layer = PictureLayerProperty::NO_OP;

	QObject* const obj = sender();
	if (obj == ui.eraser1) {
		layer = PictureLayerProperty::ERASER1;
	} else if (obj == ui.painter2) {
		layer = PictureLayerProperty::PAINTER2;
	} else if (obj == ui.eraser3) {
		layer = PictureLayerProperty::ERASER3;
	}

	m_ptrProps->locateOrCreate<PictureLayerProperty>()->setLayer(layer);

	emit updated();
}

} // namespace output
