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

#include "FixDpiSinglePageDialog.h.moc"
#include "ImageId.h"
#include <QPushButton>
#include <QIntValidator>
#include <QVariant>
#include <QFileInfo>

FixDpiSinglePageDialog::FixDpiSinglePageDialog(
	ImageId const& image_id, Dpi const& dpi,
	bool is_multipage_file, QWidget* parent)
:	QDialog(parent),
	m_dpi(dpi)
{
	ui.setupUi(this);
	
	QString const file_name(QFileInfo(image_id.filePath()).fileName());
	QString image_name(file_name);
	if (is_multipage_file) {
		image_name = tr("%1 (page %1)").arg(file_name).arg(image_id.page() + 1);
	}
	ui.text->setText(ui.text->text().arg(image_name));
	
	m_pOkBtn = ui.buttonBox->button(QDialogButtonBox::Ok);
	m_pOkBtn->setEnabled(isDpiOK());
	
	ui.dpiCombo->addItem("300 x 300", QSize(300, 300));
	ui.dpiCombo->addItem("400 x 400", QSize(400, 400));
	ui.dpiCombo->addItem("600 x 600", QSize(600, 600));
	
	ui.xDpi->setMaxLength(4);
	ui.yDpi->setMaxLength(4);
	QIntValidator* xDpiValidator = new QIntValidator(ui.xDpi);
	xDpiValidator->setBottom(100);
	ui.xDpi->setValidator(xDpiValidator);
	QIntValidator* yDpiValidator = new QIntValidator(ui.yDpi);
	yDpiValidator->setBottom(100);
	ui.yDpi->setValidator(yDpiValidator);
	
	connect(
		ui.dpiCombo, SIGNAL(activated(int)),
		this, SLOT(dpiComboChangedByUser(int))
	);
	connect(
		ui.xDpi, SIGNAL(textEdited(QString const&)),
		this, SLOT(dpiValueChanged())
	);
	connect(
		ui.yDpi, SIGNAL(textEdited(QString const&)),
		this, SLOT(dpiValueChanged())
	);
}

void
FixDpiSinglePageDialog::dpiComboChangedByUser(int idx)
{
	QVariant const data(ui.dpiCombo->itemData(idx));
	if (data.isValid()) {
		m_dpi = Dpi(data.toSize());
		ui.xDpi->setText(QString::number(m_dpi.horizontal()));
		ui.yDpi->setText(QString::number(m_dpi.vertical()));
	}
	
	m_pOkBtn->setEnabled(isDpiOK());
}

void
FixDpiSinglePageDialog::dpiValueChanged()
{
	bool x_ok = true, y_ok = true;
	m_dpi = Dpi(ui.xDpi->text().toInt(&x_ok), ui.yDpi->text().toInt(&y_ok));
	m_pOkBtn->setEnabled(isDpiOK());
	
	if (x_ok && y_ok) {
		int const count = ui.dpiCombo->count();
		for (int i = 0; i < count; ++i) {
			QVariant const data(ui.dpiCombo->itemData(i));
			if (data.isValid()) {
				if (m_dpi.toSize() == data.toSize()) {
					ui.dpiCombo->setCurrentIndex(i);
					return;
				}
			}
		}
	}
	
	ui.dpiCombo->setCurrentIndex(0);
}

bool
FixDpiSinglePageDialog::isDpiOK() const
{
	return isDpiOK(m_dpi.horizontal()) && isDpiOK(m_dpi.vertical());
}

bool
FixDpiSinglePageDialog::isDpiOK(int dpi)
{
	return dpi >= 100 && dpi <= 1600;
}
