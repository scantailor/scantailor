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

#ifndef FIX_ORIENTATION_APPLYDIALOG_H_
#define FIX_ORIENTATION_APPLYDIALOG_H_

#include "ui_OrientationApplyDialog.h"
#include <QDialog>

namespace fix_orientation
{

class Scope;

class ApplyDialog : public QDialog, private Ui::OrientationApplyDialog
{
	Q_OBJECT
public:
	ApplyDialog(QWidget* parent, int num_pages, int cur_page);
	
	virtual ~ApplyDialog();
signals:
	void accepted(Scope const& scope);
private slots:
	void thisPageSelected();
	
	void everyPageSelected();
	
	void everyOtherPageSelected();
	
	void onSubmit();
private:
	int m_numPages;
	int m_curPage;
};

} // namespace fix_orientation

#endif
