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

#include "Filter.h"
#include "FilterUiInterface.h"
#include "OptionsWidget.h"
#include "Task.h"
#include "PageId.h"
#include "Settings.h"
#include "Params.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "CacheDrivenTask.h"
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#include <QString>
#include <QObject>
#include <QCoreApplication>
#include <QDomDocument>
#include <QDomElement>

namespace output
{

Filter::Filter()
:	m_ptrSettings(new Settings)
{
	m_ptrOptionsWidget.reset(new OptionsWidget(m_ptrSettings));
}

Filter::~Filter()
{
}

QString
Filter::getName() const
{
	return QCoreApplication::translate("output::Filter", "Output");
}

PageSequence::View
Filter::getView() const
{
	return PageSequence::PAGE_VIEW;
}

void
Filter::preUpdateUI(FilterUiInterface* ui, PageId const& page_id)
{
	m_ptrOptionsWidget->preUpdateUI(page_id);
	ui->setOptionsWidget(m_ptrOptionsWidget.get(), ui->KEEP_OWNERSHIP);
}

QDomElement
Filter::saveSettings(
	ProjectWriter const& writer, QDomDocument& doc) const
{
	
	using namespace boost::lambda;
	
	QDomElement filter_el(doc.createElement("output"));
	writer.enumPages(
		bind(
			&Filter::writePageSettings,
			this, boost::ref(doc), var(filter_el), _1, _2
		)
	);
	
	return filter_el;
}

void
Filter::writePageSettings(
	QDomDocument& doc, QDomElement& filter_el,
	PageId const& page_id, int numeric_id) const
{
	Params const params(m_ptrSettings->getParams(page_id));
	
	QDomElement page_el(doc.createElement("page"));
	page_el.setAttribute("id", numeric_id);
	page_el.appendChild(params.toXml(doc, "params"));
	
	filter_el.appendChild(page_el);
}

void
Filter::loadSettings(ProjectReader const& reader, QDomElement const& filters_el)
{
	m_ptrSettings->clear();
	
	QDomElement const filter_el(
		filters_el.namedItem("output").toElement()
	);
	
	QString const page_tag_name("page");
	QDomNode node(filter_el.firstChild());
	for (; !node.isNull(); node = node.nextSibling()) {
		if (!node.isElement()) {
			continue;
		}
		if (node.nodeName() != page_tag_name) {
			continue;
		}
		QDomElement const el(node.toElement());
		
		bool ok = true;
		int const id = el.attribute("id").toInt(&ok);
		if (!ok) {
			continue;
		}
		
		PageId const page_id(reader.pageId(id));
		if (page_id.isNull()) {
			continue;
		}
		
		QDomElement const params_el(el.namedItem("params").toElement());
		if (params_el.isNull()) {
			continue;
		}
		
		Params const params(params_el);
		m_ptrSettings->setParams(page_id, params);
	}
}

IntrusivePtr<Task>
Filter::createTask(
	PageId const& page_id, int const page_num,
	QString const& out_dir, ThumbnailPixmapCache& thumbnail_cache,
	bool const batch, bool const debug)
{
	return IntrusivePtr<Task>(
		new Task(
			IntrusivePtr<Filter>(this), m_ptrSettings,
			thumbnail_cache, page_id, page_num, out_dir,
			batch, debug
		)
	);
}

IntrusivePtr<CacheDrivenTask>
Filter::createCacheDrivenTask(QString const& out_dir)
{
	return IntrusivePtr<CacheDrivenTask>(
		new CacheDrivenTask(m_ptrSettings, out_dir)
	);
}

} // namespace output
