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

#ifndef OUTPUT_PICTURE_ZONE_PROP_DIALOG_H_
#define OUTPUT_PICTURE_ZONE_PROP_DIALOG_H_

#include "ui_PictureZonePropDialog.h"
#include "PropertySet.h"
#include "IntrusivePtr.h"
#include <QDialog>

namespace output
{

class PictureZonePropDialog : public QDialog
{
	Q_OBJECT
public:
	PictureZonePropDialog(IntrusivePtr<PropertySet> const& props, QWidget* parent = 0);
signals:
	void updated();
private slots:
	void itemToggled(bool selected);
private:
	Ui::PictureZonePropDialog ui;
	IntrusivePtr<PropertySet> m_ptrProps;
};

} // namespace output

#endif
