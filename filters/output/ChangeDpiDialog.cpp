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
#include <QVariant>
#include <QIntValidator>
#include <QMessageBox>
#include <boost/foreach.hpp>
#include <algorithm>

namespace output
{

ChangeDpiDialog::ChangeDpiDialog(QWidget* parent, Dpi const& dpi)
:	QDialog(parent)
{
	setupUi(this);
	
	dpiSelector->setValidator(new QIntValidator(dpiSelector));
	
	static int const common_dpis[] = {
		300, 400, 600
	};
	
	int const requested_dpi = std::max(dpi.horizontal(), dpi.vertical());
	
	int selected_index = -1;
	BOOST_FOREACH(int const cdpi, common_dpis) {
		if (cdpi == requested_dpi) {
			selected_index = dpiSelector->count();
		}
		dpiSelector->addItem(QString::number(cdpi), cdpi);
	}
	
	m_customItemIdx = dpiSelector->count();
	dpiSelector->addItem(tr("Custom"));
	
	if (selected_index == -1) {
		selected_index = m_customItemIdx;
		dpiSelector->setEditable(true);
		// It looks like we need to set a new validator
		// every time we make the combo box editable.
		dpiSelector->setValidator(
			new QIntValidator(0, 9999, dpiSelector)
		);
	}
	dpiSelector->setCurrentIndex(selected_index);
	
	connect(
		dpiSelector, SIGNAL(currentIndexChanged(int)),
		this, SLOT(dpiSelectionChanged(int))
	);
	connect(
		dpiSelector, SIGNAL(editTextChanged(QString const&)),
		this, SLOT(customDpiChanged(QString const&))
	);
	connect(buttonBox, SIGNAL(accepted()), this, SLOT(onSubmit()));
}

ChangeDpiDialog::~ChangeDpiDialog()
{
}

void
ChangeDpiDialog::dpiSelectionChanged(int const index)
{
	QVariant const var(dpiSelector->itemData(index));
	dpiSelector->setEditable(var.isNull());
	if (var.isNull()) {
		dpiSelector->setItemText(m_customItemIdx, "600");
		// It looks like we need to set a new validator
		// every time we make the combo box editable.
		dpiSelector->setValidator(
			new QIntValidator(0, 9999, dpiSelector)
		);
	} else {
		dpiSelector->setItemText(m_customItemIdx, tr("Custom"));
	}
}

void
ChangeDpiDialog::customDpiChanged(QString const& dpi_str)
{
	dpiSelector->setItemText(m_customItemIdx, dpi_str);
}

void
ChangeDpiDialog::onSubmit()
{
	QString const dpi_str(dpiSelector->currentText());
	if (dpi_str.isEmpty()) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("Custom DPI is not set.")
		);
		return;
	}
	
	int const dpi = dpi_str.toInt();
	if (dpi < 72) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("DPI is too low!")
		);
		return;
	}
	
	if (dpi > 1200) {
		QMessageBox::warning(
			this, tr("Error"),
			tr("DPI is too high!")
		);
		return;
	}
	
	Scope const scope = allPagesRB->isChecked() ? ALL_PAGES : THIS_PAGE_ONLY;
	
	emit accepted(Dpi(dpi, dpi), scope);
	
	// We assume the default connection from accepted() to accept()
	// was removed.
	accept();
}

} // namespace output
