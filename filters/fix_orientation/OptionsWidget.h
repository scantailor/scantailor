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

#ifndef FIX_ORIENTATION_OPTIONSWIDGET_H_
#define FIX_ORIENTATION_OPTIONSWIDGET_H_

#include "ui_OrientationOptionsWidget.h"
#include "FilterOptionsWidget.h"
#include "OrthogonalRotation.h"
#include "IntrusivePtr.h"

class PageSequence;

namespace fix_orientation
{

class Scope;
class Settings;

class OptionsWidget :
	public FilterOptionsWidget, private Ui::OrientationOptionsWidget
{
	Q_OBJECT
public:
	OptionsWidget(IntrusivePtr<Settings> const& settings,
		IntrusivePtr<PageSequence> const& page_sequence);
	
	virtual ~OptionsWidget();
	
	void preUpdateUI(OrthogonalRotation rotation, int num_pages, int cur_page);
	
	void postUpdateUI(OrthogonalRotation rotation);
signals:
	void rotated(OrthogonalRotation rotation);
private slots:
	void rotateLeft();
	
	void rotateRight();
	
	void resetRotation();
	
	void showApplyToDialog();
	
	void scopeSet(Scope const& scope);
private:
	void setRotation(OrthogonalRotation rotation);
	
	void setRotationPixmap();
	
	IntrusivePtr<Settings> m_ptrSettings;
	IntrusivePtr<PageSequence> m_ptrPageSequence;
	OrthogonalRotation m_rotation;
	int m_numPages;
	int m_curPage;
};

} // namespace fix_orientation

#endif
