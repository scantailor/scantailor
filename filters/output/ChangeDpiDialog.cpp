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

#include "ChangeDpiDialog.h.moc"
#include "Dpi.h"
#include <QIntValidator>
#include <QMessageBox>

namespace output
{

ChangeDpiDialog::ChangeDpiDialog(QWidget* parent, Dpi const& dpi)
:	QDialog(parent)
{
	setupUi(this);
	
	xDpi->setText(QString::number(dpi.horizontal()));
	yDpi->setText(QString::number(dpi.vertical()));
	
	xDpi->setMaxLength(4);
	yDpi->setMaxLength(4);
	QIntValidator* xDpiValidator = new QIntValidator(xDpi);
	xDpi->setValidator(xDpiValidator);
	QIntValidator* yDpiValidator = new QIntValidator(yDpi);
	yDpi->setValidator(yDpiValidator);
	
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ChangeDpiDialog::~ChangeDpiDialog()
{
}

void
ChangeDpiDialog::onSubmit()
{
	if (xDpi->text().isEmpty()) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("Horizontal DPI is not set.")
		);
		return;
	}
	if (yDpi->text().isEmpty()) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("Vertical DPI is not set.")
		);
		return;
	}
	
	Dpi const dpi(xDpi->text().toInt(), yDpi->text().toInt());
	if (dpi.horizontal() < 72 || dpi.vertical() < 72) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("DPI is too low!")
		);
		return;
	}
	
	if (dpi.horizontal() > 1200 || dpi.vertical() > 1200) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("DPI is too high!")
		);
		return;
	}
	
	Scope const scope = allPagesRB->isChecked() ? ALL_PAGES : THIS_PAGE_ONLY;
	
	emit accepted(dpi, scope);
	
	// We assume the default connection from accepted() to accept()
	// was removed.
	accept();
}

} // namespace output
