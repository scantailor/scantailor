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
#include "ApplyDialog.h"
#include "../../Utils.h"
#include "ScopedIncDec.h"
#include "PageInfo.h"
#include "PageId.h"
#include "imageproc/Constants.h"
#ifndef Q_MOC_RUN
#include <boost/foreach.hpp>
#endif
#include <QPixmap>
#include <QString>
#include <QSettings>
#include <QVariant>
#include <assert.h>

using namespace imageproc::constants;

namespace page_layout
{

OptionsWidget::OptionsWidget(
	IntrusivePtr<Settings> const& settings,
	PageSelectionAccessor const& page_selection_accessor)
:	m_ptrSettings(settings),
	m_pageSelectionAccessor(page_selection_accessor),
	m_mmToUnit(1.0),
	m_unitToMM(1.0),
	m_ignoreMarginChanges(0),
	m_leftRightLinked(true),
	m_topBottomLinked(true)
{
	{
		QSettings app_settings;
		m_leftRightLinked = app_settings.value("margins/leftRightLinked", true).toBool();
		m_topBottomLinked = app_settings.value("margins/topBottomLinked", true).toBool();
	}

	m_chainIcon.addPixmap(
		QPixmap(QString::fromAscii(":/icons/stock-vchain-24.png"))
	);
	m_brokenChainIcon.addPixmap(
		QPixmap(QString::fromAscii(":/icons/stock-vchain-broken-24.png"))
	);
	
	setupUi(this);
	updateLinkDisplay(topBottomLink, m_topBottomLinked);
	updateLinkDisplay(leftRightLink, m_leftRightLinked);
	enableDisableAlignmentButtons();
	
	Utils::mapSetValue(
		m_alignmentByButton, alignTopLeftBtn,
		Alignment(Alignment::TOP, Alignment::LEFT)
	);
	Utils::mapSetValue(
		m_alignmentByButton, alignTopBtn,
		Alignment(Alignment::TOP, Alignment::HCENTER)
	);
	Utils::mapSetValue(
		m_alignmentByButton, alignTopRightBtn,
		Alignment(Alignment::TOP, Alignment::RIGHT)
	);
	Utils::mapSetValue(
		m_alignmentByButton, alignLeftBtn,
		Alignment(Alignment::VCENTER, Alignment::LEFT)
	);
	Utils::mapSetValue(
		m_alignmentByButton, alignCenterBtn,
		Alignment(Alignment::VCENTER, Alignment::HCENTER)
	);
	Utils::mapSetValue(
		m_alignmentByButton, alignRightBtn,
		Alignment(Alignment::VCENTER, Alignment::RIGHT)
	);
	Utils::mapSetValue(
		m_alignmentByButton, alignBottomLeftBtn,
		Alignment(Alignment::BOTTOM, Alignment::LEFT)
	);
	Utils::mapSetValue(
		m_alignmentByButton, alignBottomBtn,
		Alignment(Alignment::BOTTOM, Alignment::HCENTER)
	);
	Utils::mapSetValue(
		m_alignmentByButton, alignBottomRightBtn,
		Alignment(Alignment::BOTTOM, Alignment::RIGHT)
	);
	
	connect(
		unitsComboBox, SIGNAL(currentIndexChanged(int)),
		this, SLOT(unitsChanged(int))
	);
	connect(
		topMarginSpinBox, SIGNAL(valueChanged(double)),
		this, SLOT(vertMarginsChanged(double))
	);
	connect(
		bottomMarginSpinBox, SIGNAL(valueChanged(double)),
		this, SLOT(vertMarginsChanged(double))
	);
	connect(
		leftMarginSpinBox, SIGNAL(valueChanged(double)),
		this, SLOT(horMarginsChanged(double))
	);
	connect(
		rightMarginSpinBox, SIGNAL(valueChanged(double)),
		this, SLOT(horMarginsChanged(double))
	);
	connect(
		topBottomLink, SIGNAL(clicked()),
		this, SLOT(topBottomLinkClicked())
	);
	connect(
		leftRightLink, SIGNAL(clicked()),
		this, SLOT(leftRightLinkClicked())
	);
	connect(
		applyMarginsBtn, SIGNAL(clicked()),
		this, SLOT(showApplyMarginsDialog())
	);
	connect(
		alignWithOthersCB, SIGNAL(toggled(bool)),
		this, SLOT(alignWithOthersToggled())
	);
	connect(
		applyAlignmentBtn, SIGNAL(clicked()),
		this, SLOT(showApplyAlignmentDialog())
	);
	
	typedef AlignmentByButton::value_type KeyVal;
	BOOST_FOREACH (KeyVal const& kv, m_alignmentByButton) {
		connect(
			kv.first, SIGNAL(clicked()),
			this, SLOT(alignmentButtonClicked())
		);
	}
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::preUpdateUI(
	PageId const& page_id, Margins const& margins_mm, Alignment const& alignment)
{
	m_pageId = page_id;
	m_marginsMM = margins_mm;
	m_alignment = alignment;
	
	typedef AlignmentByButton::value_type KeyVal;
	BOOST_FOREACH (KeyVal const& kv, m_alignmentByButton) {
		if (kv.second == m_alignment) {
			kv.first->setChecked(true);
		}
	}
	
	updateMarginsDisplay();
	
	alignWithOthersCB->blockSignals(true);
	alignWithOthersCB->setChecked(!alignment.isNull());
	alignWithOthersCB->blockSignals(false);
	
	enableDisableAlignmentButtons();
	
	m_leftRightLinked = m_leftRightLinked && (margins_mm.left() == margins_mm.right());
	m_topBottomLinked = m_topBottomLinked && (margins_mm.top() == margins_mm.bottom());
	updateLinkDisplay(topBottomLink, m_topBottomLinked);
	updateLinkDisplay(leftRightLink, m_leftRightLinked);
	
	marginsGroup->setEnabled(false);
	alignmentGroup->setEnabled(false);
}

void
OptionsWidget::postUpdateUI()
{
	marginsGroup->setEnabled(true);
	alignmentGroup->setEnabled(true);
}

void
OptionsWidget::marginsSetExternally(Margins const& margins_mm)
{
	m_marginsMM = margins_mm;
	updateMarginsDisplay();
}

void
OptionsWidget::unitsChanged(int const idx)
{
	int decimals = 0;
	double step = 0.0;
	
	if (idx == 0) { // mm
		m_mmToUnit = 1.0;
		m_unitToMM = 1.0;
		decimals = 1;
		step = 1.0;
	} else { // in
		m_mmToUnit = MM2INCH;
		m_unitToMM = INCH2MM;
		decimals = 2;
		step = 0.01;
	}
	
	topMarginSpinBox->setDecimals(decimals);
	topMarginSpinBox->setSingleStep(step);
	bottomMarginSpinBox->setDecimals(decimals);
	bottomMarginSpinBox->setSingleStep(step);
	leftMarginSpinBox->setDecimals(decimals);
	leftMarginSpinBox->setSingleStep(step);
	rightMarginSpinBox->setDecimals(decimals);
	rightMarginSpinBox->setSingleStep(step);
	
	updateMarginsDisplay();
}

void
OptionsWidget::horMarginsChanged(double const val)
{
	if (m_ignoreMarginChanges) {
		return;
	}
	
	if (m_leftRightLinked) {
		ScopedIncDec<int> const ingore_scope(m_ignoreMarginChanges);
		leftMarginSpinBox->setValue(val);
		rightMarginSpinBox->setValue(val);
	}

	m_marginsMM.setLeft(leftMarginSpinBox->value() * m_unitToMM);
	m_marginsMM.setRight(rightMarginSpinBox->value() * m_unitToMM);
	
	emit marginsSetLocally(m_marginsMM);
}

void
OptionsWidget::vertMarginsChanged(double const val)
{
	if (m_ignoreMarginChanges) {
		return;
	}
	
	if (m_topBottomLinked) {
		ScopedIncDec<int> const ingore_scope(m_ignoreMarginChanges);
		topMarginSpinBox->setValue(val);
		bottomMarginSpinBox->setValue(val);
	}
	
	m_marginsMM.setTop(topMarginSpinBox->value() * m_unitToMM);
	m_marginsMM.setBottom(bottomMarginSpinBox->value() * m_unitToMM);
	
	emit marginsSetLocally(m_marginsMM);
}

void
OptionsWidget::topBottomLinkClicked()
{
	m_topBottomLinked = !m_topBottomLinked;
	QSettings().setValue("margins/topBottomLinked", m_topBottomLinked);
	updateLinkDisplay(topBottomLink, m_topBottomLinked);
	topBottomLinkToggled(m_topBottomLinked);
}

void
OptionsWidget::leftRightLinkClicked()
{
	m_leftRightLinked = !m_leftRightLinked;
	QSettings().setValue("margins/leftRightLinked", m_leftRightLinked);
	updateLinkDisplay(leftRightLink, m_leftRightLinked);
	leftRightLinkToggled(m_leftRightLinked);
}

void
OptionsWidget::alignWithOthersToggled()
{
	m_alignment.setNull(!alignWithOthersCB->isChecked());
	enableDisableAlignmentButtons();
	emit alignmentChanged(m_alignment);
}

void
OptionsWidget::alignmentButtonClicked()
{
	QToolButton* const button = dynamic_cast<QToolButton*>(sender());
	assert(button);
	
	AlignmentByButton::iterator const it(m_alignmentByButton.find(button));
	assert(it != m_alignmentByButton.end());
	
	m_alignment = it->second;
	emit alignmentChanged(m_alignment);
}

void
OptionsWidget::showApplyMarginsDialog()
{
	ApplyDialog* dialog = new ApplyDialog(
		this, m_pageId, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowTitle(tr("Apply Margins"));
	connect(
		dialog, SIGNAL(accepted(std::set<PageId> const&)),
		this, SLOT(applyMargins(std::set<PageId> const&))
	);
	dialog->show();
}

void
OptionsWidget::showApplyAlignmentDialog()
{
	ApplyDialog* dialog = new ApplyDialog(
		this, m_pageId, m_pageSelectionAccessor
	);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	dialog->setWindowTitle(tr("Apply Alignment"));
	connect(
		dialog, SIGNAL(accepted(std::set<PageId> const&)),
		this, SLOT(applyAlignment(std::set<PageId> const&))
	);
	dialog->show();
}

void
OptionsWidget::applyMargins(std::set<PageId> const& pages)
{
	if (pages.empty()) {
		return;
	}
	
	BOOST_FOREACH(PageId const& page_id, pages) {
		m_ptrSettings->setHardMarginsMM(page_id, m_marginsMM);
	}
	
	emit aggregateHardSizeChanged();
	emit invalidateAllThumbnails();
}

void
OptionsWidget::applyAlignment(std::set<PageId> const& pages)
{
	if (pages.empty()) {
		return;
	}
	
	BOOST_FOREACH(PageId const& page_id, pages) {
		m_ptrSettings->setPageAlignment(page_id, m_alignment);
	}
	
	emit invalidateAllThumbnails();
}

void
OptionsWidget::updateMarginsDisplay()
{
	ScopedIncDec<int> const ignore_scope(m_ignoreMarginChanges);
	
	topMarginSpinBox->setValue(m_marginsMM.top() * m_mmToUnit);
	bottomMarginSpinBox->setValue(m_marginsMM.bottom() * m_mmToUnit);
	leftMarginSpinBox->setValue(m_marginsMM.left() * m_mmToUnit);
	rightMarginSpinBox->setValue(m_marginsMM.right() * m_mmToUnit);
}

void
OptionsWidget::updateLinkDisplay(QToolButton* button, bool const linked)
{
	button->setIcon(linked ? m_chainIcon : m_brokenChainIcon);
}

void
OptionsWidget::enableDisableAlignmentButtons()
{
	bool const enabled = alignWithOthersCB->isChecked();
	
	alignTopLeftBtn->setEnabled(enabled);
	alignTopBtn->setEnabled(enabled);
	alignTopRightBtn->setEnabled(enabled);
	alignLeftBtn->setEnabled(enabled);
	alignCenterBtn->setEnabled(enabled);
	alignRightBtn->setEnabled(enabled);
	alignBottomLeftBtn->setEnabled(enabled);
	alignBottomBtn->setEnabled(enabled);
	alignBottomRightBtn->setEnabled(enabled);
}

} // namespace page_layout

