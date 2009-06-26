/*
    Scan Tailor - Interactive post-processing tool for scanned pages.
    Copyright (C) 2007-2009  Joseph Artsimovich <joseph_a@mail.ru>

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

#ifndef PROJECTOPENINGCONTEXT_H_
#define PROJECTOPENINGCONTEXT_H_

#include "NonCopyable.h"
#include "ProjectReader.h"
#include "ImageFileInfo.h"
#include <QObject>
#include <QPointer>
#include <QString>
#include <Qt>
#include <vector>

class FixDpiDialog;
class QWidget;
class QDomDocument;

class ProjectOpeningContext : public QObject
{
	Q_OBJECT
	DECLARE_NON_COPYABLE(ProjectOpeningContext)
public:
	ProjectOpeningContext(
		QWidget* parent, QString const& project_file, QDomDocument const& doc);
	
	virtual ~ProjectOpeningContext();
	
	void proceed();
	
	QString const& projectFile() const { return m_projectFile; }
	
	ProjectReader* projectReader() { return &m_reader; }
signals:
	void done(ProjectOpeningContext* context);
private slots:
	void fixedDpiSubmitted();
	
	void fixDpiDialogDestroyed();
private:
	void showFixDpiDialog();
	
	QString m_projectFile;
	ProjectReader m_reader;
	QPointer<FixDpiDialog> m_ptrFixDpiDialog;
	QWidget* m_pParent;
};

#endif
