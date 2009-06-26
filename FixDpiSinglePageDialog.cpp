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
#include <QColor>

FixDpiSinglePageDialog::FixDpiSinglePageDialog(
	ImageId const& image_id, ImageMetadata const& image_metadata,
	bool is_multipage_file, QWidget* parent)
:	QDialog(parent),
	m_metadata(image_metadata)
{
	ui.setupUi(this);
	
	m_normalPalette = ui.xDpi->palette();
	m_errorPalette = m_normalPalette;
	m_errorPalette.setColor(QPalette::Text, Qt::red);
	
	QString const file_name(QFileInfo(image_id.filePath()).fileName());
	QString image_name(file_name);
	if (is_multipage_file) {
		image_name = tr("%1 (page %2)").arg(file_name).arg(image_id.page() + 1);
	}
	ui.text->setText(ui.text->text().arg(image_name));
	
	m_pOkBtn = ui.buttonBox->button(QDialogButtonBox::Ok);
	m_pOkBtn->setEnabled(m_metadata.isDpiOK());
	
	ui.dpiCombo->addItem("300 x 300", QSize(300, 300));
	ui.dpiCombo->addItem("400 x 400", QSize(400, 400));
	ui.dpiCombo->addItem("600 x 600", QSize(600, 600));
	
	ui.xDpi->setMaxLength(4);
	ui.yDpi->setMaxLength(4);
	ui.xDpi->setValidator(new QIntValidator(ui.xDpi));
	ui.yDpi->setValidator(new QIntValidator(ui.yDpi));
	
	if (m_metadata.dpi().horizontal() > 1) {
		ui.xDpi->setText(QString::number(m_metadata.dpi().horizontal()));
	}
	
	if (m_metadata.dpi().vertical() > 1) {
		ui.yDpi->setText(QString::number(m_metadata.dpi().vertical()));
	}
	
	dpiValueChanged();
	
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
		m_metadata.setDpi(Dpi(data.toSize()));
		ui.xDpi->setText(QString::number(m_metadata.dpi().horizontal()));
		ui.yDpi->setText(QString::number(m_metadata.dpi().vertical()));
		decorateDpiInputField(ui.xDpi, m_metadata.horizontalDpiStatus());
		decorateDpiInputField(ui.yDpi, m_metadata.verticalDpiStatus());
	}
	
	m_pOkBtn->setEnabled(m_metadata.isDpiOK());
}

void
FixDpiSinglePageDialog::dpiValueChanged()
{
	m_metadata.setDpi(Dpi(ui.xDpi->text().toInt(), ui.yDpi->text().toInt()));
	decorateDpiInputField(ui.xDpi, m_metadata.horizontalDpiStatus());
	decorateDpiInputField(ui.yDpi, m_metadata.verticalDpiStatus());
	bool const ok = m_metadata.isDpiOK();
	m_pOkBtn->setEnabled(ok);
	
	if (ok) {
		int const count = ui.dpiCombo->count();
		for (int i = 0; i < count; ++i) {
			QVariant const data(ui.dpiCombo->itemData(i));
			if (data.isValid()) {
				if (m_metadata.dpi().toSize() == data.toSize()) {
					ui.dpiCombo->setCurrentIndex(i);
					return;
				}
			}
		}
	}
	
	ui.dpiCombo->setCurrentIndex(0);
}

void
FixDpiSinglePageDialog::decorateDpiInputField(QLineEdit* field, ImageMetadata::DpiStatus dpi_status) const
{
	if (dpi_status == ImageMetadata::DPI_OK) {
		field->setPalette(m_normalPalette);
	} else {
		field->setPalette(m_errorPalette);
	}

	switch (dpi_status) {
		case ImageMetadata::DPI_OK:
		case ImageMetadata::DPI_UNDEFINED:
			field->setToolTip(QString());
			break;
		case ImageMetadata::DPI_TOO_SMALL:
			field->setToolTip(tr("DPI is too small. Even if it's correct, you are not going to get acceptable results with it."));
			break;
		case ImageMetadata::DPI_TOO_SMALL_FOR_THIS_PIXEL_SIZE:
			field->setToolTip(tr("DPI is too small for this pixel size. Such combination would probably lead to out of memory errors."));
			break;
	}
}
