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

#include "ApplyDialog.h.moc"

namespace page_layout
{

ApplyDialog::ApplyDialog(QWidget* parent)
:	QDialog(parent),
	m_scope(THIS_PAGE)
{
	setupUi(this);
	
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
	connect(thisPageRB, SIGNAL(pressed()), this, SLOT(thisPageSelected()));
	connect(allPagesRB, SIGNAL(pressed()), this, SLOT(allPagesSelected()));
}

ApplyDialog::~ApplyDialog()
{
}

void
ApplyDialog::thisPageSelected()
{
	m_scope = THIS_PAGE;
}

void
ApplyDialog::allPagesSelected()
{
	m_scope = ALL_PAGES;
}

void
ApplyDialog::onSubmit()
{
	emit accepted(m_scope);
	accept();
}

} // namespace page_layout
