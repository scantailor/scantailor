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

#ifndef CRASH_REPORTER_DIALOG_H_
#define CRASH_REPORTER_DIALOG_H_

#include "ui_CrashReportDialog.h"
#include <QDialog>
#include <QtGlobal>
#include <QString>
#include <QByteArray>
#include <QFileInfo>
#include <QPalette>

class QNetworkAccessManager;
class QNetworkReply;

class CrashReportDialog : public QDialog
{
	Q_OBJECT
public:
	CrashReportDialog(QString const& dir, QString const& id, QWidget* parent = 0);

	~CrashReportDialog();
protected:
	virtual bool eventFilter(QObject* watched, QEvent* event);
private slots:
	void onSubmit();

	void dispatchDone(QNetworkReply* reply);

	void submissionDone(QNetworkReply* reply);
private:
	class SystemProxyFactory;

	static QString formatFileSize(qint64 size);

	QString prepareDetails() const;

	Ui::CrashReportDialog ui;
	QFileInfo m_dumpFileInfo;
	QPalette m_normalPalette;
	QPalette m_errorPalette;
	QPalette m_successPalette;
	QNetworkAccessManager* m_pDispatcher;
	QNetworkAccessManager* m_pSubmitter;
	bool m_disregardAdditionalInfo;
	bool m_disregardEmail;
};

#endif
