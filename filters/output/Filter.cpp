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
#include "OutputParams.h"
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
#include <memory>

#include "CommandLine.h"
#include "ImageViewTab.h"

namespace output
{

Filter::Filter(
	PageSelectionAccessor const& page_selection_accessor)
:	m_ptrSettings(new Settings)
{
	if (CommandLine::get().isGui()) {
		m_ptrOptionsWidget.reset(
			new OptionsWidget(m_ptrSettings, page_selection_accessor)
		);
	}
}

Filter::~Filter()
{
}

QString
Filter::getName() const
{
	return QCoreApplication::translate("output::Filter", "Output");
}

PageView
Filter::getView() const
{
	return PAGE_VIEW;
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
	
	QDomElement filter_el(doc.createElement("output"));
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
	Params const params(m_ptrSettings->getParams(page_id));
	
	QDomElement page_el(doc.createElement("page"));
	page_el.setAttribute("id", numeric_id);

	page_el.appendChild(m_ptrSettings->pictureZonesForPage(page_id).toXml(doc, "zones"));
	page_el.appendChild(m_ptrSettings->fillZonesForPage(page_id).toXml(doc, "fill-zones"));
	page_el.appendChild(params.toXml(doc, "params"));
	
	std::auto_ptr<OutputParams> output_params(m_ptrSettings->getOutputParams(page_id));
	if (output_params.get()) {
		page_el.appendChild(output_params->toXml(doc, "output-params"));
	}
	
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
		
		ZoneSet const picture_zones(el.namedItem("zones").toElement(), m_pictureZonePropFactory);
		if (!picture_zones.empty()) {
			m_ptrSettings->setPictureZones(page_id, picture_zones);
		}

		ZoneSet const fill_zones(el.namedItem("fill-zones").toElement(), m_fillZonePropFactory);
		if (!fill_zones.empty()) {
			m_ptrSettings->setFillZones(page_id, fill_zones);
		}

		QDomElement const params_el(el.namedItem("params").toElement());
		if (!params_el.isNull()) {
			Params const params(params_el);
			m_ptrSettings->setParams(page_id, params);
		}
		
		QDomElement const output_params_el(el.namedItem("output-params").toElement());
		if (!output_params_el.isNull()) {
			OutputParams const output_params(output_params_el);
			m_ptrSettings->setOutputParams(page_id, output_params);
		}
	}
}

IntrusivePtr<Task>
Filter::createTask(
	PageId const& page_id,
	IntrusivePtr<ThumbnailPixmapCache> const& thumbnail_cache,
	OutputFileNameGenerator const& out_file_name_gen,
	bool const batch, bool const debug)
{
	ImageViewTab lastTab(TAB_OUTPUT);
	if (m_ptrOptionsWidget.get() != 0)
		lastTab = m_ptrOptionsWidget->lastTab();
	return IntrusivePtr<Task>(
		new Task(
			IntrusivePtr<Filter>(this), m_ptrSettings,
			thumbnail_cache, page_id, out_file_name_gen,
			lastTab, batch, debug
		)
	);
}

IntrusivePtr<CacheDrivenTask>
Filter::createCacheDrivenTask(OutputFileNameGenerator const& out_file_name_gen)
{
	return IntrusivePtr<CacheDrivenTask>(
		new CacheDrivenTask(m_ptrSettings, out_file_name_gen)
	);
}

} // namespace output
