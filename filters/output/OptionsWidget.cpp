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

#include "OptionsWidget.h"
#include "OptionsWidget.h.moc"
#include "ChangeDpiDialog.h"
#include "ApplyColorsDialog.h"
#include "Settings.h"
#include "Params.h"
#include "DespeckleLevel.h"
#include "ZoneSet.h"
#include "PictureZoneComparator.h"
#include "../../Utils.h"
#include "ScopedIncDec.h"
#include "config.h"
#include <boost/foreach.hpp>
#include <QVariant>
#include <QColorDialog>
#include <QToolTip>
#include <QString>
#include <QCursor>
#include <QPoint>
#include <QSize>
#include <Qt>
#include <QDebug>

namespace output
{

OptionsWidget::OptionsWidget(
	IntrusivePtr<Settings> const& settings,
	PageSelectionAccessor const& page_selection_accessor)
:	m_ptrSettings(settings),
	m_pageSelectionAccessor(page_selection_accessor),
	m_despeckleLevel(DESPECKLE_NORMAL),
	m_lastTab(TAB_OUTPUT),
	m_ignoreThresholdChanges(0)
{
	setupUi(this);

#if !defined(ENABLE_DEWARPING)
	dewarpPanel->hide();
#endif
	
	colorModeSelector->addItem(tr("Black and White"), ColorParams::BLACK_AND_WHITE);
	colorModeSelector->addItem(tr("Color / Grayscale"), ColorParams::COLOR_GRAYSCALE);
	colorModeSelector->addItem(tr("Mixed"), ColorParams::MIXED);
	
	darkerThresholdLink->setText(
		Utils::richTextForLink(darkerThresholdLink->text())
	);
	lighterThresholdLink->setText(
		Utils::richTextForLink(lighterThresholdLink->text())
	);
	thresholdSlider->setToolTip(QString::number(thresholdSlider->value()));
	
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
		whiteMarginsCB, SIGNAL(clicked(bool)),
		this, SLOT(whiteMarginsToggled(bool))
	);
	connect(
		equalizeIlluminationCB, SIGNAL(clicked(bool)),
		this, SLOT(equalizeIlluminationToggled(bool))
	);
	connect(
		dewarpCB, SIGNAL(clicked(bool)),
		this, SLOT(dewarpToggled(bool))
	);
	connect(
		lighterThresholdLink, SIGNAL(linkActivated(QString const&)),
		this, SLOT(setLighterThreshold())
	);
	connect(
		darkerThresholdLink, SIGNAL(linkActivated(QString const&)),
		this, SLOT(setDarkerThreshold())
	);
	connect(
		neutralThresholdBtn, SIGNAL(clicked()),
		this, SLOT(setNeutralThreshold())
	);
	connect(
		thresholdSlider, SIGNAL(valueChanged(int)),
		this, SLOT(bwThresholdChanged())
	);
	connect(
		thresholdSlider, SIGNAL(sliderReleased()),
		this, SLOT(bwThresholdChanged())
	);
	connect(
		applyColorsButton, SIGNAL(clicked()),
		this, SLOT(applyColorsButtonClicked())
	);

	connect(
		despeckleOffBtn, SIGNAL(clicked()),
		this, SLOT(despeckleOffSelected())
	);
	connect(
		despeckleCautiousBtn, SIGNAL(clicked()),
		this, SLOT(despeckleCautiousSelected())
	);
	connect(
		despeckleNormalBtn, SIGNAL(clicked()),
		this, SLOT(despeckleNormalSelected())
	);
	connect(
		despeckleAggressiveBtn, SIGNAL(clicked()),
		this, SLOT(despeckleAggressiveSelected())
	);
	connect(
		applyDespeckleButton, SIGNAL(clicked()),
		this, SLOT(applyDespeckleButtonClicked())
	);
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::preUpdateUI(PageId const& page_id)
{
	Params const params(m_ptrSettings->getParams(page_id));
	m_pageId = page_id;
	m_outputDpi = params.outputDpi();
	m_colorParams = params.colorParams();
	m_despeckleLevel = params.despeckleLevel();
	updateDpiDisplay();
	updateColorsDisplay();
}

void
OptionsWidget::postUpdateUI()
{
}

void
OptionsWidget::tabChanged(ImageViewTab const tab)
{
	m_lastTab = tab;
	reloadIfNecessary();
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
OptionsWidget::whiteMarginsToggled(bool const checked)
{
	ColorGrayscaleOptions opt(m_colorParams.colorGrayscaleOptions());
	opt.setWhiteMargins(checked);
	if (!checked) {
		opt.setNormalizeIllumination(false);
		equalizeIlluminationCB->setChecked(false);
	}
	m_colorParams.setColorGrayscaleOptions(opt);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	equalizeIlluminationCB->setEnabled(checked);
	emit reloadRequested();
}

void
OptionsWidget::equalizeIlluminationToggled(bool const checked)
{
	ColorGrayscaleOptions opt(m_colorParams.colorGrayscaleOptions());
	opt.setNormalizeIllumination(checked);
	m_colorParams.setColorGrayscaleOptions(opt);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::dewarpToggled(bool const checked)
{
	BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
	opt.setDewarp(checked);
	m_colorParams.setBlackWhiteOptions(opt);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::setLighterThreshold()
{
	thresholdSlider->setValue(thresholdSlider->value() - 1);
}

void
OptionsWidget::setDarkerThreshold()
{
	thresholdSlider->setValue(thresholdSlider->value() + 1);
}

void
OptionsWidget::setNeutralThreshold()
{
	thresholdSlider->setValue(0);
}

void
OptionsWidget::bwThresholdChanged()
{
	int const value = thresholdSlider->value();
	QString const tooltip_text(QString::number(value));
	thresholdSlider->setToolTip(tooltip_text);
	
	if (m_ignoreThresholdChanges) {
		return;
	}
	
	// Show the tooltip immediately.
	QPoint const center(thresholdSlider->rect().center());
	QPoint tooltip_pos(thresholdSlider->mapFromGlobal(QCursor::pos()));
	if (tooltip_pos.x() < 0 || tooltip_pos.x() >= thresholdSlider->width()) {
		tooltip_pos.setX(center.x());
	}
	if (tooltip_pos.y() < 0 || tooltip_pos.y() >= thresholdSlider->height()) {
		tooltip_pos.setY(center.y());
	}
	tooltip_pos = thresholdSlider->mapToGlobal(tooltip_pos);
	QToolTip::showText(tooltip_pos, tooltip_text, thresholdSlider);
	
	if (thresholdSlider->isSliderDown()) {
		// Wait for it to be released.
		// We could have just disabled tracking, but in that case we wouldn't
		// be able to show tooltips with a precise value.
		return;
	}

	BlackWhiteOptions opt(m_colorParams.blackWhiteOptions());
	if (opt.thresholdAdjustment() == value) {
		// Didn't change.
		return;
	}

	opt.setThresholdAdjustment(value);
	m_colorParams.setBlackWhiteOptions(opt);
	m_ptrSettings->setColorParams(m_pageId, m_colorParams);
	emit reloadRequested();
}

void
OptionsWidget::changeDpiButtonClicked()
{
	ChangeDpiDialog* dialog = new ChangeDpiDialog(
		this, m_outputDpi, m_pageId, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(
		dialog, SIGNAL(accepted(std::set<PageId> const&, Dpi const&)),
		this, SLOT(dpiChanged(std::set<PageId> const&, Dpi const&))
	);
	connect(
		dialog, SIGNAL(acceptedForAllPages(Dpi const&)),
		this, SLOT(dpiChangedForAllPages(Dpi const&))
	);
	dialog->show();
}

void
OptionsWidget::applyColorsButtonClicked()
{
	ApplyColorsDialog* dialog = new ApplyColorsDialog(
		this, m_pageId, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(
		dialog, SIGNAL(accepted(std::set<PageId> const&)),
		this, SLOT(applyColorsConfirmed(std::set<PageId> const&))
	);
	connect(
		dialog, SIGNAL(acceptedForAllPages()),
		this, SLOT(applyColorsToAllPagesConfirmed())
	);
	dialog->show();
}

void
OptionsWidget::dpiChanged(std::set<PageId> const& pages, Dpi const& dpi)
{
	updateDpiDisplay();
	
	BOOST_FOREACH(PageId const& page_id, pages) {
		m_ptrSettings->setDpi(page_id, dpi);
		emit invalidateThumbnail(page_id);
	}
	
	if (pages.find(m_pageId) != pages.end()) {
		m_outputDpi = dpi;
		emit reloadRequested();
	}
}

void
OptionsWidget::dpiChangedForAllPages(Dpi const& dpi)
{
	m_outputDpi = dpi;
	updateDpiDisplay();

	m_ptrSettings->setDpiForAllPages(dpi);
	emit invalidateAllThumbnails();
	emit reloadRequested();
}

void
OptionsWidget::applyColorsConfirmed(std::set<PageId> const& pages)
{
	BOOST_FOREACH(PageId const& page_id, pages) {
		m_ptrSettings->setColorParams(page_id, m_colorParams);
		emit invalidateThumbnail(page_id);
	}
	
	if (pages.find(m_pageId) != pages.end()) {
		emit reloadRequested();
	}
}

void
OptionsWidget::applyColorsToAllPagesConfirmed()
{
	m_ptrSettings->setColorParamsForAllPages(m_colorParams);
	emit invalidateAllThumbnails();

	emit reloadRequested();
}

void
OptionsWidget::despeckleOffSelected()
{
	handleDespeckleLevelChange(DESPECKLE_OFF);
}

void
OptionsWidget::despeckleCautiousSelected()
{
	handleDespeckleLevelChange(DESPECKLE_CAUTIOUS);
}

void
OptionsWidget::despeckleNormalSelected()
{
	handleDespeckleLevelChange(DESPECKLE_NORMAL);
}

void
OptionsWidget::despeckleAggressiveSelected()
{
	handleDespeckleLevelChange(DESPECKLE_AGGRESSIVE);
}

void
OptionsWidget::handleDespeckleLevelChange(DespeckleLevel const level)
{
	m_despeckleLevel = level;
	m_ptrSettings->setDespeckleLevel(m_pageId, level);

	bool handled = false;
	emit despeckleLevelChanged(level, &handled);
	
	if (handled) {
		// This means we are on the "Despeckling" tab.
		emit invalidateThumbnail(m_pageId);
	} else {
		emit reloadRequested();
	}
}

void
OptionsWidget::applyDespeckleButtonClicked()
{
	ApplyColorsDialog* dialog = new ApplyColorsDialog(
		this, m_pageId, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowTitle(tr("Apply Despeckling Level"));
	connect(
		dialog, SIGNAL(accepted(std::set<PageId> const&)),
		this, SLOT(applyDespeckleConfirmed(std::set<PageId> const&))
	);
	connect(
		dialog, SIGNAL(acceptedForAllPages()),
		this, SLOT(applyDespeckleToAllPagesConfirmed())
	);
	dialog->show();
}

void
OptionsWidget::applyDespeckleConfirmed(std::set<PageId> const& pages)
{
	BOOST_FOREACH(PageId const& page_id, pages) {
		m_ptrSettings->setDespeckleLevel(page_id, m_despeckleLevel);
		emit invalidateThumbnail(page_id);
	}
	
	if (pages.find(m_pageId) != pages.end()) {
		emit reloadRequested();
	}
}

void
OptionsWidget::applyDespeckleToAllPagesConfirmed()
{
	m_ptrSettings->setDespeckleLevelForAllPages(m_despeckleLevel);
	emit invalidateAllThumbnails();
	emit reloadRequested();
}

void
OptionsWidget::reloadIfNecessary()
{
	ZoneSet saved_zones;
	DespeckleLevel saved_despeckle_level = DESPECKLE_NORMAL;
	
	std::auto_ptr<OutputParams> output_params(m_ptrSettings->getOutputParams(m_pageId));
	if (output_params.get()) {
		saved_zones = output_params->zones();
		saved_despeckle_level = output_params->outputImageParams().despeckleLevel();
	}

	if (!PictureZoneComparator::equal(saved_zones, m_ptrSettings->zonesForPage(m_pageId))) {
		emit reloadRequested();
	} else if (saved_despeckle_level != m_ptrSettings->getDespeckleLevel(m_pageId)) {
		emit reloadRequested();
	}
}

void
OptionsWidget::updateDpiDisplay()
{
	if (m_outputDpi.horizontal() != m_outputDpi.vertical()) {
		dpiLabel->setText(
			QString::fromAscii("%1 x %2")
			.arg(m_outputDpi.horizontal()).arg(m_outputDpi.vertical())
		);
	} else {
		dpiLabel->setText(QString::number(m_outputDpi.horizontal()));
	}
}

void
OptionsWidget::updateColorsDisplay()
{
	colorModeSelector->blockSignals(true);
	
	ColorParams::ColorMode const color_mode = m_colorParams.colorMode();
	int const color_mode_idx = colorModeSelector->findData(color_mode);
	colorModeSelector->setCurrentIndex(color_mode_idx);
	
	bool color_grayscale_options_visible = false;
	bool bw_options_visible = false;
	switch (color_mode) {
		case ColorParams::BLACK_AND_WHITE:
			bw_options_visible = true;
			break;
		case ColorParams::COLOR_GRAYSCALE:
			color_grayscale_options_visible = true;
			break;
		case ColorParams::MIXED:
			bw_options_visible = true;
			break;
	}
	
	colorGrayscaleOptions->setVisible(color_grayscale_options_visible);
	if (color_grayscale_options_visible) {
		ColorGrayscaleOptions const opt(
			m_colorParams.colorGrayscaleOptions()
		);
		whiteMarginsCB->setChecked(opt.whiteMargins());
		equalizeIlluminationCB->setChecked(opt.normalizeIllumination());
		equalizeIlluminationCB->setEnabled(opt.whiteMargins());
	}
	
	bwOptions->setVisible(bw_options_visible);
	despecklePanel->setVisible(bw_options_visible);
	if (bw_options_visible) {
		dewarpCB->setChecked(
			m_colorParams.blackWhiteOptions().dewarp()
		);

		switch (m_despeckleLevel) {
			case DESPECKLE_OFF:
				despeckleOffBtn->setChecked(true);
				break;
			case DESPECKLE_CAUTIOUS:
				despeckleCautiousBtn->setChecked(true);
				break;
			case DESPECKLE_NORMAL:
				despeckleNormalBtn->setChecked(true);
				break;
			case DESPECKLE_AGGRESSIVE:
				despeckleAggressiveBtn->setChecked(true);
				break;
		}

		ScopedIncDec<int> const guard(m_ignoreThresholdChanges);
		thresholdSlider->setValue(
			m_colorParams.blackWhiteOptions().thresholdAdjustment()
		);
	}
	
	colorModeSelector->blockSignals(false);
}

} // namespace output
