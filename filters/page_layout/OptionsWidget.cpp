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
#include "Settings.h"
#include "imageproc/Constants.h"

using namespace imageproc::constants;

namespace page_layout
{

OptionsWidget::OptionsWidget(IntrusivePtr<Settings> const& settings)
:	m_ptrSettings(settings),
	m_mmToUnit(1.0),
	m_unitToMM(1.0),
	m_leftRightLinked(true),
	m_topBottomLinked(true)
{
	setupUi(this);
	connect(
		unitsComboBox, SIGNAL(currentIndexChanged(int)),
		this, SLOT(unitsChanged(int))
	);
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::preUpdateUI(PageId const& page_id)
{
	m_marginsMM = m_ptrSettings->getPageMarginsMM(page_id);
	updateMarginsDisplay();
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
	if (idx == 0) { // mm
		m_mmToUnit = 1.0;
		m_unitToMM = 1.0;
	} else { // in
		m_mmToUnit = MM2INCH;
		m_unitToMM = INCH2MM;
	}
	updateMarginsDisplay();
}

void
OptionsWidget::updateMarginsDisplay()
{
	topMarginSpinBox->setValue(m_marginsMM.top() * m_mmToUnit);
	bottomMarginSpinBox->setValue(m_marginsMM.bottom() * m_mmToUnit);
	leftMarginSpinBox->setValue(m_marginsMM.left() * m_mmToUnit);
	rightMarginSpinBox->setValue(m_marginsMM.right() * m_mmToUnit);
}

} // namespace page_layout

