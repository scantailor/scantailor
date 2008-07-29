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

#include "OptionsWidget.h.moc"
#include "ChangeDpiDialog.h"
#include "Settings.h"
#include "Dpi.h"
#include <QColorDialog>
#include <QIcon>
#include <QSize>
#include <Qt>

namespace output
{

OptionsWidget::OptionsWidget(IntrusivePtr<Settings> const& settings)
:	m_ptrSettings(settings),
	m_pageParams(Dpi(300, 300)),
	m_lightColor(Qt::white),
	m_darkColor(Qt::black),
	m_lightColorPixmap(QSize(16, 16)),
	m_darkColorPixmap(QSize(16, 16))
{
	m_lightColorPixmap.fill(m_lightColor);
	m_darkColorPixmap.fill(m_darkColor);
	
	setupUi(this);
	
	colorModeSelector->addItem(tr("Black and White"), Params::BLACK_AND_WHITE);
	colorModeSelector->addItem(tr("Bitonal"), Params::BITONAL);
	colorModeSelector->addItem(tr("Color / Grayscale"), Params::COLOR_GRAYSCALE);
	
	// FIXME: by doing this, we put a fake entry into m_ptrSettings!
	colorModeChanged(colorModeSelector->currentIndex());
	
	thresholdSelector->addItem(QString::fromAscii("Otsu"), Params::OTSU);
	thresholdSelector->addItem(QString::fromAscii("Sauvola"), Params::SAUVOLA);
	thresholdSelector->addItem(QString::fromAscii("Wolf"), Params::WOLF);
	
	connect(
		changeDpiButton, SIGNAL(clicked()),
		this, SLOT(changeDpiButtonClicked())
	);
	connect(
		colorModeSelector, SIGNAL(currentIndexChanged(int)),
		this, SLOT(colorModeChanged(int))
	);
	connect(
		lightColorButton, SIGNAL(clicked()),
		this, SLOT(lightColorButtonClicked())
	);
	connect(
		darkColorButton, SIGNAL(clicked()),
		this, SLOT(darkColorButtonClicked())
	);
	connect(
		thresholdSelector, SIGNAL(currentIndexChanged(int)),
		this, SLOT(thresholdModeChanged(int))
	);
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::preUpdateUI(PageId const& page_id)
{
	m_pageId = page_id;
	m_pageParams = m_ptrSettings->getPageParams(page_id);
	updateDpiDisplay(m_pageParams.dpi());
	setEnabled(false);
}

void
OptionsWidget::postUpdateUI()
{
	setEnabled(true);
}

void
OptionsWidget::colorModeChanged(int const idx)
{
	int const mode = colorModeSelector->itemData(idx).toInt();
	Params::ColorMode const old_mode = m_pageParams.colorMode();
	m_pageParams.setColorMode((Params::ColorMode)mode);
	m_ptrSettings->setPageParams(m_pageId, m_pageParams);
	
	switch (mode) {
		case Params::BLACK_AND_WHITE:
			bitonalOptionsWidget->setVisible(true);
			lightColorButton->setEnabled(false);
			darkColorButton->setEnabled(false);
			m_lightColorPixmap.fill(Qt::white);
			m_darkColorPixmap.fill(Qt::black);
			lightColorButton->setIcon(createIcon(m_lightColorPixmap));
			darkColorButton->setIcon(createIcon(m_darkColorPixmap));
			if (old_mode == Params::BITONAL) {
				emit tonesChanged(Qt::white, Qt::black);
			} else {
				emit reloadRequested();
			}
			break;
		case Params::BITONAL:
			bitonalOptionsWidget->setVisible(true);
			lightColorButton->setEnabled(true);
			darkColorButton->setEnabled(true);
			m_lightColorPixmap.fill(m_lightColor);
			m_darkColorPixmap.fill(m_darkColor);
			lightColorButton->setIcon(createIcon(m_lightColorPixmap));
			darkColorButton->setIcon(createIcon(m_darkColorPixmap));
			if (old_mode == Params::BLACK_AND_WHITE) {
				emit tonesChanged(m_lightColor, m_darkColor);
			} else {
				emit reloadRequested();
			}
			break;
		case Params::COLOR_GRAYSCALE:
			bitonalOptionsWidget->setVisible(false);
			emit reloadRequested();
			break;
	}
}

void
OptionsWidget::thresholdModeChanged(int const idx)
{
	int const mode = thresholdSelector->itemData(idx).toInt();
	m_pageParams.setThresholdMode((Params::ThresholdMode)mode);
	m_ptrSettings->setPageParams(m_pageId, m_pageParams);
	emit reloadRequested();
}

void
OptionsWidget::changeDpiButtonClicked()
{
	ChangeDpiDialog* dialog = new ChangeDpiDialog(this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(
		dialog, SIGNAL(accepted(Dpi const&, Scope)),
		this, SLOT(dpiChanged(Dpi const&, Scope))
	);
	dialog->show();
}

void
OptionsWidget::dpiChanged(Dpi const& dpi, Scope const scope)
{
	m_pageParams.setDpi(dpi);
	updateDpiDisplay(dpi);
	
	if (scope == THIS_PAGE_ONLY) {
		m_ptrSettings->setDpi(m_pageId, dpi);
	} else {
		m_ptrSettings->setDpiForAllPages(dpi);
	}
	
	emit reloadRequested(); // This will also update the thumbnail.
	if (scope == ALL_PAGES) {
		emit invalidateAllThumbnails();
	}
}

void
OptionsWidget::lightColorButtonClicked()
{
	QColor const color(QColorDialog::getColor(m_lightColor, this));
	if (!color.isValid()) {
		// The dialog was closed.
		return;
	}
	
	m_lightColor = color;
	m_lightColorPixmap.fill(m_lightColor);
	lightColorButton->setIcon(createIcon(m_lightColorPixmap));
	
	emit tonesChanged(m_lightColor, m_darkColor);
}

void
OptionsWidget::darkColorButtonClicked()
{
	QColor const color(QColorDialog::getColor(m_darkColor, this));
	if (!color.isValid()) {
		// The dialog was closed.
		return;
	}
	
	m_darkColor = color;
	m_darkColorPixmap.fill(m_darkColor);
	darkColorButton->setIcon(createIcon(m_darkColorPixmap));
	
	emit tonesChanged(m_lightColor, m_darkColor);
}

QIcon
OptionsWidget::createIcon(QPixmap const& pixmap)
{
	QIcon icon(pixmap);
	icon.addPixmap(pixmap, QIcon::Disabled, QIcon::Off);
	icon.addPixmap(pixmap, QIcon::Disabled, QIcon::On);
	return icon;
}

void
OptionsWidget::updateDpiDisplay(Dpi const& dpi)
{
	dpiLabel->setText(
		QString::fromAscii("%1 x %2")
		.arg(dpi.horizontal()).arg(dpi.vertical())
	);
}

} // namespace output
