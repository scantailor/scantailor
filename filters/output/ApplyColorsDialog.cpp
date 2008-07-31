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

#include "ApplyColorsDialog.h.moc"

namespace output
{

ApplyColorsDialog::ApplyColorsDialog(QWidget* parent)
:	QDialog(parent)
{
	setupUi(this);
	
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ApplyColorsDialog::~ApplyColorsDialog()
{
}

void
ApplyColorsDialog::onSubmit()
{
	Scope const scope = allPagesRB->isChecked() ? ALL_PAGES : THIS_PAGE_ONLY;
	emit accepted(scope);
	
	// We assume the default connection from accepted() to accept()
	// was removed.
	accept();
}

} // namespace output
