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
#include "Filter.h"
#include "ApplyDialog.h"
#include "Scope.h"
#include "Settings.h"
#include "PageSequence.h"
#include "ImageId.h"
#include "PageId.h"
#include <boost/foreach.hpp>
#include <vector>
#include <assert.h>

namespace fix_orientation
{

OptionsWidget::OptionsWidget(
	IntrusivePtr<Settings> const& settings,
	IntrusivePtr<PageSequence> const& page_sequence)
:	m_ptrSettings(settings),
	m_ptrPageSequence(page_sequence),
	m_numPages(0),
	m_curPage(0)
{
	setupUi(this);
	
	connect(rotateLeftBtn, SIGNAL(clicked()), this, SLOT(rotateLeft()));
	connect(rotateRightBtn, SIGNAL(clicked()), this, SLOT(rotateRight()));
	connect(resetBtn, SIGNAL(clicked()), this, SLOT(resetRotation()));
	connect(applyToBtn, SIGNAL(clicked()), this, SLOT(showApplyToDialog()));
}

OptionsWidget::~OptionsWidget()
{
}

void
OptionsWidget::preUpdateUI(
	OrthogonalRotation const rotation, int const num_pages, int const cur_page)
{
	m_numPages = num_pages;
	m_curPage = cur_page;
	m_rotation = rotation;
	setRotationPixmap();
}

void
OptionsWidget::postUpdateUI(OrthogonalRotation const rotation)
{
	setRotation(rotation);
}

void
OptionsWidget::rotateLeft()
{
	OrthogonalRotation rotation(m_rotation);
	rotation.prevClockwiseDirection();
	setRotation(rotation);
}

void
OptionsWidget::rotateRight()
{
	OrthogonalRotation rotation(m_rotation);
	rotation.nextClockwiseDirection();
	setRotation(rotation);
}

void
OptionsWidget::resetRotation()
{
	setRotation(OrthogonalRotation());
}

void
OptionsWidget::showApplyToDialog()
{
	ApplyDialog* dialog = new ApplyDialog(this, m_numPages, m_curPage);
	dialog->setAttribute(Qt::WA_DeleteOnClose);
	connect(
		dialog, SIGNAL(accepted(Scope const&)),
		this, SLOT(scopeSet(Scope const&))
	);
	dialog->show();
}

void
OptionsWidget::scopeSet(Scope const& scope)
{
	std::vector<ImageId> const image_ids(
		m_ptrSettings->applyRule(scope, m_rotation)
	);
	
	BOOST_FOREACH(ImageId const& image_id, image_ids) {
		emit invalidateThumbnail(PageId(image_id));
	}
}

void
OptionsWidget::setRotation(OrthogonalRotation const rotation)
{
	if (rotation == m_rotation) {
		return;
	}
	
	m_rotation = rotation;
	setRotationPixmap();
	
	ImageId const image_id(m_ptrPageSequence->curImage());
	m_ptrSettings->applyRule(image_id, rotation);
	
	emit rotated(rotation);
	emit invalidateThumbnail(PageId(image_id));
}

void
OptionsWidget::setRotationPixmap()
{
	char const* path = 0;
	
	switch (m_rotation.toDegrees()) {
		case 0:
			path = ":/icons/big-up-arrow.png";
			break;
		case 90:
			path = ":/icons/big-right-arrow.png";
			break;
		case 180:
			path = ":/icons/big-down-arrow.png";
			break;
		case 270:
			path = ":/icons/big-left-arrow.png";
			break;
		default:
			assert(!"Unreachable");
	}
	
	rotationIndicator->setPixmap(QPixmap(path));
}


} // namespace fix_orientation
