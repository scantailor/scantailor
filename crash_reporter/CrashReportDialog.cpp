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

#include "CrashReportDialog.h"
#include "CrashReportDialog.h.moc"
#include "MultipartFormData.h"
#include "version.h"
#include <QDir>
#include <QUrl>
#include <QTextDocument> // for Qt::escape()
#include <QToolTip>
#include <QEvent>
#include <QApplication>
#include <QLocale>
#include <QPushButton>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QNetworkProxyFactory>
#include <QDebug>
#include <Qt>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNodeList>

class CrashReportDialog::SystemProxyFactory : public QNetworkProxyFactory
{
public:
	virtual QList<QNetworkProxy> queryProxy(
			QNetworkProxyQuery const& query = QNetworkProxyQuery()) {
		return systemProxyForQuery(query);
	}
};

CrashReportDialog::CrashReportDialog(
	QString const& dir, QString const& id, QWidget* parent)
:	QDialog(parent),
	m_dumpFileInfo(QDir(dir), id+".dmp"),
	m_normalPalette(QApplication::palette()),
	m_errorPalette(m_normalPalette),
	m_successPalette(m_normalPalette),
	m_pDispatcher(new QNetworkAccessManager(this)),
	m_pSubmitter(new QNetworkAccessManager(this)),
	m_disregardAdditionalInfo(true),
	m_disregardEmail(true)
{
	m_pDispatcher->setProxyFactory(new SystemProxyFactory);
	m_pSubmitter->setProxyFactory(new SystemProxyFactory);

	ui.setupUi(this);
	ui.statusLabel->setText(" ");
	
	m_errorPalette.setColor(QPalette::WindowText, Qt::red);
	m_successPalette.setColor(QPalette::WindowText, Qt::green);

	ui.dumpFile->setToolTip(tr(
		"This file contains the internal state of the program when it crashed"
	));
	ui.dumpFile->setText(
		QString("<a href=\"%1\">%2</a> (%3)").arg(
			Qt::escape(QUrl::fromLocalFile(m_dumpFileInfo.absoluteFilePath()).toString()),
			Qt::escape(tr("Dump file")),
			formatFileSize(m_dumpFileInfo.size())
		)
	);
	ui.dumpFile->setOpenExternalLinks(true);
	
	QPalette graytext_palette(m_normalPalette);
	graytext_palette.setColor(QPalette::Text, QColor(118, 118, 118));
	ui.additionalInfo->setPalette(graytext_palette);
	ui.email->setPalette(graytext_palette);
	
	ui.buttonBox->button(QDialogButtonBox::Ok)->setFocus();

	// To catch FocusIn events and clear text / restore palette.
	ui.additionalInfo->installEventFilter(this);
	ui.email->installEventFilter(this);

	connect(ui.buttonBox, SIGNAL(accepted()), SLOT(onSubmit()));
}

CrashReportDialog::~CrashReportDialog()
{
	QFile::remove(m_dumpFileInfo.absoluteFilePath());
}

void
CrashReportDialog::onSubmit()
{
	ui.statusLabel->setText(tr("Sending ..."));
	ui.statusLabel->setPalette(m_normalPalette);
	ui.infoGroup->setEnabled(false);
	ui.buttonBox->setEnabled(false);

	QUrl url("http://scantailor.sourceforge.net/crash_dispatcher/");
	url.addQueryItem("version", VERSION);
	url.addQueryItem("locale", QLocale::system().name());
	
	m_pDispatcher->get(QNetworkRequest(url));
	connect(
		m_pDispatcher, SIGNAL(finished(QNetworkReply*)),
		SLOT(dispatchDone(QNetworkReply*))
	);
}

void
CrashReportDialog::dispatchDone(QNetworkReply* reply)
{
	reply->deleteLater();
	ui.infoGroup->setEnabled(true);
	ui.buttonBox->setEnabled(true);

	if (reply->error() != QNetworkReply::NoError) {
		ui.statusLabel->setText(reply->errorString());
		ui.statusLabel->setPalette(m_errorPalette);
		return;
	}
	
	int const status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (status != 200) {
		ui.statusLabel->setText(tr("Unexpected response (code %1) from dispatcher").arg(status));
		ui.statusLabel->setPalette(m_errorPalette);
		return;
	}

	QDomDocument doc;

	QString errmsg;
	if (!doc.setContent(reply->readAll(), true, &errmsg)) {
		ui.statusLabel->setText(errmsg);
		ui.statusLabel->setPalette(m_errorPalette);
		return;
	}
	
	QDomElement doc_el(doc.documentElement());

	QDomElement reject_el(doc_el.namedItem("reject").toElement());
	if (!reject_el.isNull()) {
		ui.statusLabel->setText(reject_el.text());
		ui.statusLabel->setPalette(m_errorPalette);
		return;
	}

	QDomElement forward_el(doc_el.namedItem("forward").toElement());
	if (forward_el.isNull()) {
		ui.statusLabel->setText(tr("Unexpected response from dispatcher"));
		ui.statusLabel->setPalette(m_errorPalette);
		return;
	}

	MultipartFormData form_data;

	QUrl const url(forward_el.attribute("url"));
	QDomNodeList const fields(forward_el.childNodes());
	for (int i = 0; i < fields.count(); ++i) {
		QDomElement field(fields.item(i).toElement());
		if (field.isNull()) {
			continue;
		}
		if (field.localName() == "fixed") {
			form_data.addParameter(field.attribute("name"), field.text());
		} else if (field.localName() == "mapped") {
			QString const name(field.attribute("name"));
			QString const var(field.text());
			if (var == "details") {
				form_data.addParameter(name, prepareDetails());
			} else if (var == "file") {
				QFile file(m_dumpFileInfo.absoluteFilePath());
				file.open(QIODevice::ReadOnly);
				form_data.addFile(name, m_dumpFileInfo.fileName(), file.readAll());
			}
		}
	}

	QNetworkRequest request(url);
	request.setHeader(QNetworkRequest::ContentTypeHeader, form_data.contentType());
	request.setRawHeader("User-Agent", "Scan Tailor crash reporter");
	request.setRawHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8");

	m_pSubmitter->post(request, form_data.finalize());
	connect(
		m_pSubmitter, SIGNAL(finished(QNetworkReply*)),
		SLOT(submissionDone(QNetworkReply*))
	);

	ui.infoGroup->setEnabled(false);
	ui.buttonBox->setEnabled(false);
}

void
CrashReportDialog::submissionDone(QNetworkReply* reply)
{
	reply->deleteLater();
	ui.infoGroup->setEnabled(true);
	ui.buttonBox->setEnabled(true);
	
	if (reply->error() != QNetworkReply::NoError) {
		ui.statusLabel->setText(reply->errorString());
		ui.statusLabel->setPalette(m_errorPalette);
		return;
	}
	
	int const status = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
	if (status != 200) {
		ui.statusLabel->setText(
			tr("Unexpected response (code %1) from the receiving side").arg(status)
		);
		ui.statusLabel->setPalette(m_errorPalette);
		return;
	}
	
	ui.statusLabel->setText(tr("Successfully sent"));
	ui.statusLabel->setPalette(m_successPalette);
	ui.infoGroup->setEnabled(false);
	ui.buttonBox->setStandardButtons(QDialogButtonBox::Close);
}

bool
CrashReportDialog::eventFilter(QObject* watched, QEvent* event)
{
	if (event->type() == QEvent::FocusIn) {
		if (watched == ui.additionalInfo) {
			if (m_disregardAdditionalInfo) {
				ui.additionalInfo->setPalette(m_normalPalette);
				ui.additionalInfo->clear();
				m_disregardAdditionalInfo = false;
			}
		} else if (watched == ui.email) {
			if (m_disregardEmail) {
				ui.email->setPalette(m_normalPalette);
				ui.email->clear();
				m_disregardEmail = false;
			}
		}
	}
	return false; // Don't filter out the event.
}

QString
CrashReportDialog::formatFileSize(qint64 size)
{
	qint64 const kb = (size + 1023) / 1024;
	if (kb < 1000) {
		return QString("%1 KB").arg((size + 1023) / 1024);
	} else {
		return QString("%1 MB").arg(size / (1024.0*1024), 0, 'g', 1);
	}
}

QString
CrashReportDialog::prepareDetails() const
{
	QString text;
	text += "This crash report was sent using Scan Tailor's built-in crash reporter.\n\n";
	text += "Scan Tailor version: " VERSION "\n\n";

	QString const additional_info(ui.additionalInfo->toPlainText().trimmed());
	if (!m_disregardAdditionalInfo && !additional_info.isEmpty()) {
		text += "Additional information provided by the user:";
		text += "\n--------------------------------------------------------------\n";
		text += additional_info;
		text += "\n--------------------------------------------------------------\n";
	}

	QString const email(ui.email->text().trimmed());
	if (!m_disregardEmail && !email.isEmpty()) {
		text += "Contact email: "+email;
	}

	return text;
}
