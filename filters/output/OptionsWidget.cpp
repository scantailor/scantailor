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
#include "ApplyColorsDialog.h"
#include "Settings.h"
#include <QVariant>
#include <QColorDialog>
#include <QIcon>
#include <QSize>
#include <Qt>

namespace output
{

OptionsWidget::OptionsWidget(IntrusivePtr<Settings> const& settings)
:	m_ptrSettings(settings),
	m_lightColorPixmap(QSize(16, 16)),
	m_darkColorPixmap(QSize(16, 16))
{
	m_lightColorPixmap.fill(m_colorParams.lightColor());
	m_darkColorPixmap.fill(m_colorParams.darkColor());
	
	setupUi(this);
	
	colorModeSelector->addItem(tr("Black and White"), ColorParams::BLACK_AND_WHITE);
	colorModeSelector->addItem(tr("Bitonal"), ColorParams::BITONAL);
	colorModeSelector->addItem(tr("Color / Grayscale"), ColorParams::COLOR_GRAYSCALE);
	colorModeSelector->addItem(tr("Mixed"), ColorParams::MIXED);
	
	thresholdSelector->addItem(QString::fromAscii("Otsu"), ColorParams::OTSU);
	thresholdSelector->addItem(QString::fromAscii("Mokji"), ColorParams::MOKJI);
	thresholdSelector->addItem(QString::fromAscii("Sauvola"), ColorParams::SAUVOLA);
	thresholdSelector->addItem(QString::fromAscii("Wolf"), ColorParams::WOLF);
	
	updateColorsDisplay();
	
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
	connect(
		applyColorsButton, SIGNAL(clicked()),
		this, SLOT(applyColorsButtonClicked())
	);
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::preUpdateUI(PageId const& page_id)
{
	m_pageId = page_id;
	m_dpi = m_ptrSettings->getDpi(page_id);
	m_colorParams = m_ptrSettings->getColorParams(page_id);
	updateDpiDisplay();
	updateColorsDisplay();
}

void
OptionsWidget::postUpdateUI()
{
}

void
OptionsWidget::colorModeChanged(int const idx)
{
	int const mode = colorModeSelector->itemData(idx).toInt();
	m_colorParams.setColorMode((ColorParams::ColorMode)mode);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	updateColorsDisplay();
	emit reloadRequested();
}

void
OptionsWidget::thresholdModeChanged(int const idx)
{
	int const mode = thresholdSelector->itemData(idx).toInt();
	m_colorParams.setThresholdMode((ColorParams::ThresholdMode)mode);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::changeDpiButtonClicked()
{
	ChangeDpiDialog* dialog = new ChangeDpiDialog(this, m_dpi);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(
		dialog, SIGNAL(accepted(Dpi const&, Scope)),
		this, SLOT(dpiChanged(Dpi const&, Scope))
	);
	dialog->show();
}

void
OptionsWidget::applyColorsButtonClicked()
{
	ApplyColorsDialog* dialog = new ApplyColorsDialog(this);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(
		dialog, SIGNAL(accepted(Scope)),
		this, SLOT(applyColorsConfirmed(Scope))
	);
	dialog->show();
}

void
OptionsWidget::dpiChanged(Dpi const& dpi, Scope const scope)
{
	m_dpi = dpi;
	updateDpiDisplay();
	
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
OptionsWidget::applyColorsConfirmed(Scope const scope)
{
	if (scope == THIS_PAGE_ONLY) {
		return;
	}
	
	m_ptrSettings->setColorParamsForAllPages(m_colorParams);
	emit invalidateAllThumbnails();
}

void
OptionsWidget::lightColorButtonClicked()
{
	QColor const color(QColorDialog::getColor(m_colorParams.lightColor(), this));
	if (!color.isValid()) {
		// The dialog was closed.
		return;
	}
	
	m_colorParams.setLightColor(color.rgb());
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	m_lightColorPixmap.fill(m_colorParams.lightColor());
	lightColorButton->setIcon(createIcon(m_lightColorPixmap));
	
	emit reloadRequested();
}

void
OptionsWidget::darkColorButtonClicked()
{
	QColor const color(QColorDialog::getColor(m_colorParams.darkColor(), this));
	if (!color.isValid()) {
		// The dialog was closed.
		return;
	}
	
	m_colorParams.setDarkColor(color.rgb());
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	m_darkColorPixmap.fill(m_colorParams.darkColor());
	darkColorButton->setIcon(createIcon(m_darkColorPixmap));
	
	emit reloadRequested();
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
OptionsWidget::updateDpiDisplay()
{
	dpiLabel->setText(
		QString::fromAscii("%1 x %2")
		.arg(m_dpi.horizontal()).arg(m_dpi.vertical())
	);
}

void
OptionsWidget::updateColorsDisplay()
{
	colorModeSelector->blockSignals(true);
	thresholdSelector->blockSignals(true);
	
	ColorParams::ColorMode const color_mode = m_colorParams.colorMode();
	int const color_mode_idx = colorModeSelector->findData(color_mode);
	colorModeSelector->setCurrentIndex(color_mode_idx);
	
	bool const show_bitonal_options = (
		color_mode == ColorParams::BLACK_AND_WHITE
		|| color_mode == ColorParams::BITONAL
	);
	
	bitonalOptionsWidget->setVisible(show_bitonal_options);
	lightColorButton->setEnabled(color_mode == ColorParams::BITONAL);
	darkColorButton->setEnabled(color_mode == ColorParams::BITONAL);
	
	if (show_bitonal_options) {
		if (color_mode == ColorParams::BLACK_AND_WHITE) {
			m_lightColorPixmap.fill(Qt::white);
			m_darkColorPixmap.fill(Qt::black);
		} else if (color_mode == ColorParams::BITONAL) {
			m_lightColorPixmap.fill(m_colorParams.lightColor());
			m_darkColorPixmap.fill(m_colorParams.darkColor());
		}
		lightColorButton->setIcon(createIcon(m_lightColorPixmap));
		darkColorButton->setIcon(createIcon(m_darkColorPixmap));
	}
	
	ColorParams::ThresholdMode const threshold_mode = m_colorParams.thresholdMode();
	int const threshold_idx = thresholdSelector->findData(threshold_mode);
	thresholdSelector->setCurrentIndex(threshold_idx);
	
	colorModeSelector->blockSignals(false);
	thresholdSelector->blockSignals(false);
}

} // namespace output
