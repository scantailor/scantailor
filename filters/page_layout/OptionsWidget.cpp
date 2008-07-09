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
#include <QPixmap>
#include <QString>

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
	m_chainIcon.addPixmap(
		QPixmap(QString::fromAscii(":/icons/stock-vchain-24.png"))
	);
	m_brokenChainIcon.addPixmap(
		QPixmap(QString::fromAscii(":/icons/stock-vchain-broken-24.png"))
	);
	
	setupUi(this);
	updateLinkDisplay(topBottomLink, m_topBottomLinked);
	updateLinkDisplay(leftRightLink, m_leftRightLinked);
	
	connect(
		unitsComboBox, SIGNAL(currentIndexChanged(int)),
		this, SLOT(unitsChanged(int))
	);
	connect(topBottomLink, SIGNAL(clicked()), this, SLOT(topBottomLinkClicked()));
	connect(leftRightLink, SIGNAL(clicked()), this, SLOT(leftRightLinkClicked()));
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
	int decimals = 0;
	if (idx == 0) { // mm
		m_mmToUnit = 1.0;
		m_unitToMM = 1.0;
		decimals = 1;
	} else { // in
		m_mmToUnit = MM2INCH;
		m_unitToMM = INCH2MM;
		decimals = 2;
	}
	
	topMarginSpinBox->setDecimals(decimals);
	bottomMarginSpinBox->setDecimals(decimals);
	leftMarginSpinBox->setDecimals(decimals);
	rightMarginSpinBox->setDecimals(decimals);
	
	updateMarginsDisplay();
}

void
OptionsWidget::topBottomLinkClicked()
{
	m_topBottomLinked = !m_topBottomLinked;
	updateLinkDisplay(topBottomLink, m_topBottomLinked);
	topBottomLinkToggled(m_topBottomLinked);
}

void
OptionsWidget::leftRightLinkClicked()
{
	m_leftRightLinked = !m_leftRightLinked;
	updateLinkDisplay(leftRightLink, m_leftRightLinked);
	leftRightLinkToggled(m_leftRightLinked);
}

void
OptionsWidget::updateMarginsDisplay()
{
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

} // namespace page_layout

