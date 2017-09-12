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
#include "OrderByWidthProvider.h"
#include "OrderByHeightProvider.h"
#ifndef Q_MOC_RUN
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#endif
#include <QString>
#include <QObject>
#include <QDomDocument>
#include <QDomElement>
#include <assert.h>
#include "CommandLine.h"

namespace select_content
{

Filter::Filter(
	PageSelectionAccessor const& page_selection_accessor)
:	m_ptrSettings(new Settings),
	m_selectedPageOrder(0)
{
	if (CommandLine::get().isGui()) {
		m_ptrOptionsWidget.reset(
			new OptionsWidget(m_ptrSettings, page_selection_accessor)
		);
	}

	typedef PageOrderOption::ProviderPtr ProviderPtr;

	ProviderPtr const default_order;
	ProviderPtr const order_by_width(new OrderByWidthProvider(m_ptrSettings));
	ProviderPtr const order_by_height(new OrderByHeightProvider(m_ptrSettings));
	m_pageOrderOptions.push_back(PageOrderOption(tr("Natural order"), default_order));
	m_pageOrderOptions.push_back(PageOrderOption(tr("Order by increasing width"), order_by_width));
	m_pageOrderOptions.push_back(PageOrderOption(tr("Order by increasing height"), order_by_height));
}

Filter::~Filter()
{
}

QString
Filter::getName() const
{
	return tr("Select Content");
}

PageView
Filter::getView() const
{
	return PAGE_VIEW;
}

int
Filter::selectedPageOrder() const
{
	return m_selectedPageOrder;
}

void
Filter::selectPageOrder(int option)
{
	assert((unsigned)option < m_pageOrderOptions.size());
	m_selectedPageOrder = option;
}

std::vector<PageOrderOption>
Filter::pageOrderOptions() const
{
	return m_pageOrderOptions;
}

void
Filter::performRelinking(AbstractRelinker const& relinker)
{
	m_ptrSettings->performRelinking(relinker);
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
	
	QDomElement filter_el(doc.createElement("select-content"));

	filter_el.setAttribute("average", m_ptrSettings->avg());
	filter_el.setAttribute("sigma", m_ptrSettings->std());
	filter_el.setAttribute("maxDeviation", m_ptrSettings->maxDeviation());
	filter_el.setAttribute("pageDetectionBoxWidth", m_ptrSettings->pageDetectionBox().width());
	filter_el.setAttribute("pageDetectionBoxHeight", m_ptrSettings->pageDetectionBox().height());
	filter_el.setAttribute("pageDetectionTolerance", m_ptrSettings->pageDetectionTolerance());

	writer.enumPages(
		boost::lambda::bind(
			&Filter::writePageSettings,
			this, boost::ref(doc), var(filter_el), boost::lambda::_1, boost::lambda::_2
		)
	);
	
	return filter_el;
}

void
Filter::writePageSettings(
	QDomDocument& doc, QDomElement& filter_el,
	PageId const& page_id, int numeric_id) const
{
	std::auto_ptr<Params> const params(m_ptrSettings->getPageParams(page_id));
	if (!params.get()) {
		return;
	}
	
	QDomElement page_el(doc.createElement("page"));
	page_el.setAttribute("id", numeric_id);
	page_el.appendChild(params->toXml(doc, "params"));
	
	filter_el.appendChild(page_el);
}

void
Filter::loadSettings(ProjectReader const& reader, QDomElement const& filters_el)
{
	m_ptrSettings->clear();

    CommandLine cli = CommandLine::get();

	QDomElement const filter_el(
		filters_el.namedItem("select-content").toElement()
	);

	m_ptrSettings->setAvg(filter_el.attribute("average").toDouble());
	m_ptrSettings->setStd(filter_el.attribute("sigma").toDouble());
    
    if (cli.hasContentDeviation()) {
        m_ptrSettings->setMaxDeviation(cli.getContentDeviation());
    } else {
	    m_ptrSettings->setMaxDeviation(filter_el.attribute("maxDeviation", QString::number(cli.getContentDeviation())).toDouble());
    }
	
	QSizeF box(0.0, 0.0);
	box.setWidth(filter_el.attribute("pageDetectionBoxWidth", "0.0").toDouble());
	box.setHeight(filter_el.attribute("pageDetectionBoxHeight", "0.0").toDouble());
	m_ptrSettings->setPageDetectionBox(box);
	
	m_ptrSettings->setPageDetectionTolerance(filter_el.attribute("pageDetectionTolerance", "0.1").toDouble());

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
		m_ptrSettings->setPageParams(page_id, params);
	}
}

IntrusivePtr<Task>
Filter::createTask(
	PageId const& page_id,
	IntrusivePtr<page_layout::Task> const& next_task,
	bool batch, bool debug)
{
	return IntrusivePtr<Task>(
		new Task(
			IntrusivePtr<Filter>(this), next_task,
			m_ptrSettings, page_id, batch, debug
		)
	);
}

IntrusivePtr<CacheDrivenTask>
Filter::createCacheDrivenTask(
	IntrusivePtr<page_layout::CacheDrivenTask> const& next_task)
{
	return IntrusivePtr<CacheDrivenTask>(
		new CacheDrivenTask(m_ptrSettings, next_task)
	);
}

} // namespace select_content
