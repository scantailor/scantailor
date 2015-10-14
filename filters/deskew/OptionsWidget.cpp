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
#include "Settings.h"
#include "ScopedIncDec.h"
#include <QString>
#include <Qt>
#include <math.h>
#include "ApplyDialog.h"
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif

namespace deskew
{

double const OptionsWidget::MAX_ANGLE = 45.0;

OptionsWidget::OptionsWidget(IntrusivePtr<Settings> const& settings,
						PageSelectionAccessor const& page_selection_accessor)
:	m_ptrSettings(settings),
	m_ignoreAutoManualToggle(0),
	m_ignoreSpinBoxChanges(0),
	m_pageSelectionAccessor(page_selection_accessor)
{
	setupUi(this);
	angleSpinBox->setSuffix(QChar(0x00B0)); // the degree symbol
	angleSpinBox->setRange(-MAX_ANGLE, MAX_ANGLE);
	angleSpinBox->adjustSize();
	setSpinBoxUnknownState();
	
	connect(
		angleSpinBox, SIGNAL(valueChanged(double)),
		this, SLOT(spinBoxValueChanged(double))
	);
	connect(autoBtn, SIGNAL(toggled(bool)), this, SLOT(modeChanged(bool)));
	connect(
		applyDeskewBtn, SIGNAL(clicked()),
		this, SLOT(showDeskewDialog())
	);
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::showDeskewDialog()
{
	ApplyDialog* dialog = new ApplyDialog(
		this, m_pageId, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowTitle(tr("Apply Deskew"));
	connect(
		dialog, SIGNAL(appliedTo(std::set<PageId> const&)),
		this, SLOT(appliedTo(std::set<PageId> const&))
	);
	connect(
		dialog, SIGNAL(appliedToAllPages(std::set<PageId> const&)),
		this, SLOT(appliedToAllPages(std::set<PageId> const&))
	);
	dialog->show();
}

void
OptionsWidget::appliedTo(std::set<PageId> const& pages)
{
	if (pages.empty()) {
		return;
	}
	
	Params const params(
		m_uiData.effectiveDeskewAngle(),
		m_uiData.dependencies(), m_uiData.mode()
	);
	m_ptrSettings->setDegress(pages, params);
	BOOST_FOREACH(PageId const& page_id, pages) {
		emit invalidateThumbnail(page_id);
	}
}

void
OptionsWidget::appliedToAllPages(std::set<PageId> const& pages)
{
	if (pages.empty()) {
		return;
	}
	
	Params const params(
		m_uiData.effectiveDeskewAngle(),
		m_uiData.dependencies(), m_uiData.mode()
	);
	m_ptrSettings->setDegress(pages, params);
	emit invalidateAllThumbnails();
}

void
OptionsWidget::manualDeskewAngleSetExternally(double const degrees)
{
	m_uiData.setEffectiveDeskewAngle(degrees);
	m_uiData.setMode(MODE_MANUAL);
	updateModeIndication(MODE_MANUAL);
	setSpinBoxKnownState(degreesToSpinBox(degrees));
	commitCurrentParams();
	
	emit invalidateThumbnail(m_pageId);
}

void
OptionsWidget::preUpdateUI(PageId const& page_id)
{
	ScopedIncDec<int> guard(m_ignoreAutoManualToggle);
	
	m_pageId = page_id;
	setSpinBoxUnknownState();
	autoBtn->setChecked(true);
	autoBtn->setEnabled(false);
	manualBtn->setEnabled(false);
}

void
OptionsWidget::postUpdateUI(UiData const& ui_data)
{
	m_uiData = ui_data;
	autoBtn->setEnabled(true);
	manualBtn->setEnabled(true);
	updateModeIndication(ui_data.mode());
	setSpinBoxKnownState(degreesToSpinBox(ui_data.effectiveDeskewAngle()));
}

void
OptionsWidget::spinBoxValueChanged(double const value)
{
	if (m_ignoreSpinBoxChanges) {
		return;
	}
	
	double const degrees = spinBoxToDegrees(value);
	m_uiData.setEffectiveDeskewAngle(degrees);
	m_uiData.setMode(MODE_MANUAL);
	updateModeIndication(MODE_MANUAL);
	commitCurrentParams();
	
	emit manualDeskewAngleSet(degrees);
	emit invalidateThumbnail(m_pageId);
}

void
OptionsWidget::modeChanged(bool const auto_mode)
{
	if (m_ignoreAutoManualToggle) {
		return;
	}
	
	if (auto_mode) {
		m_uiData.setMode(MODE_AUTO);
		m_ptrSettings->clearPageParams(m_pageId);
		emit reloadRequested();
	} else {
		m_uiData.setMode(MODE_MANUAL);
		commitCurrentParams();
	}
}

void
OptionsWidget::updateModeIndication(AutoManualMode const mode)
{
	ScopedIncDec<int> guard(m_ignoreAutoManualToggle);
	
	if (mode == MODE_AUTO) {
		autoBtn->setChecked(true);
	} else {
		manualBtn->setChecked(true);
	}
}

void
OptionsWidget::setSpinBoxUnknownState()
{
	ScopedIncDec<int> guard(m_ignoreSpinBoxChanges);
	
	angleSpinBox->setSpecialValueText("?");
	angleSpinBox->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
	angleSpinBox->setValue(angleSpinBox->minimum());
	angleSpinBox->setEnabled(false);
}

void
OptionsWidget::setSpinBoxKnownState(double const angle)
{
	ScopedIncDec<int> guard(m_ignoreSpinBoxChanges);
	
	angleSpinBox->setSpecialValueText("");
	angleSpinBox->setValue(angle);
	
	// Right alignment doesn't work correctly, so we use the left one.
	angleSpinBox->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);
	angleSpinBox->setEnabled(true);
}

void
OptionsWidget::commitCurrentParams()
{
	Params const params(
		m_uiData.effectiveDeskewAngle(),
		m_uiData.dependencies(), m_uiData.mode()
	);
	m_ptrSettings->setPageParams(m_pageId, params);
}

double
OptionsWidget::spinBoxToDegrees(double const sb_value)
{
	// The spin box shows the angle in a usual geometric way,
	// with positive angles going counter-clockwise.
	// Internally, we operate with angles going clockwise,
	// because the Y axis points downwards in computer graphics.
	return -sb_value;
}

double
OptionsWidget::degreesToSpinBox(double const degrees)
{
	// See above.
	return -degrees;
}


/*========================== OptionsWidget::UiData =========================*/

OptionsWidget::UiData::UiData()
:	m_effDeskewAngle(0.0),
	m_mode(MODE_AUTO)
{
}

OptionsWidget::UiData::~UiData()
{
}

void
OptionsWidget::UiData::setEffectiveDeskewAngle(double const degrees)
{
	m_effDeskewAngle = degrees;
}

double
OptionsWidget::UiData::effectiveDeskewAngle() const
{
	return m_effDeskewAngle;
}

void
OptionsWidget::UiData::setDependencies(Dependencies const& deps)
{
	m_deps = deps;
}

Dependencies const&
OptionsWidget::UiData::dependencies() const
{
	return m_deps;
}

void
OptionsWidget::UiData::setMode(AutoManualMode const mode)
{
	m_mode = mode;
}

AutoManualMode
OptionsWidget::UiData::mode() const
{
	return m_mode;
}

} // namespace deskew
