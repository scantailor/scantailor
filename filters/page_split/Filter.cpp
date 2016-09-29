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
#include "Settings.h"
#include "ProjectPages.h"
#include "ProjectReader.h"
#include "ProjectWriter.h"
#include "PageId.h"
#include "ImageId.h"
#include "PageLayout.h"
#include "LayoutType.h"
#include "Params.h"
#include "CacheDrivenTask.h"
#include "OrthogonalRotation.h"
#ifndef Q_MOC_RUN
#include <boost/lambda/lambda.hpp>
#include <boost/lambda/bind.hpp>
#endif
#include <QString>
#include <QObject>
#include <QCoreApplication>
#include <QDomElement>
#include <stddef.h>
#include "CommandLine.h"
#include "OrderBySplitTypeProvider.h"

namespace page_split
{

Filter::Filter(IntrusivePtr<ProjectPages> const& page_sequence,
	PageSelectionAccessor const& page_selection_accessor)
:	m_ptrPages(page_sequence),
	m_ptrSettings(new Settings),
	m_selectedPageOrder(0)
{
	if (CommandLine::get().isGui()) {
		m_ptrOptionsWidget.reset(
			new OptionsWidget(
				m_ptrSettings, m_ptrPages, page_selection_accessor
			)
		);
	}
	
	typedef PageOrderOption::ProviderPtr ProviderPtr;

	ProviderPtr const default_order;
	ProviderPtr const order_by_splitline(new OrderBySplitTypeProvider(m_ptrSettings));
	m_pageOrderOptions.push_back(PageOrderOption(tr("Natural order"), default_order));
	m_pageOrderOptions.push_back(PageOrderOption(tr("Order by split type"), order_by_splitline));
}

Filter::~Filter()
{
}

QString
Filter::getName() const
{
	return QCoreApplication::translate("page_split::Filter", "Split Pages");
}

PageView
Filter::getView() const
{
	return IMAGE_VIEW;
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
	
	QDomElement filter_el(doc.createElement("page-split"));
	filter_el.setAttribute(
		"defaultLayoutType",
		layoutTypeToString(m_ptrSettings->defaultLayoutType())
	);
	
	writer.enumImages(
		boost::lambda::bind(
			&Filter::writeImageSettings,
			this, boost::ref(doc), var(filter_el), boost::lambda::_1, boost::lambda::_2
		)
	);
	
	return filter_el;
}

void
Filter::loadSettings(
	ProjectReader const& reader, QDomElement const& filters_el)
{
	m_ptrSettings->clear();
	
	QDomElement const filter_el(filters_el.namedItem("page-split").toElement());
	QString const default_layout_type(
		filter_el.attribute("defaultLayoutType")
	);
	m_ptrSettings->setLayoutTypeForAllPages(
		layoutTypeFromString(default_layout_type)
	);
	
	QString const image_tag_name("image");
	QDomNode node(filter_el.firstChild());
	for (; !node.isNull(); node = node.nextSibling()) {
		if (!node.isElement()) {
			continue;
		}
		if (node.nodeName() != image_tag_name) {
			continue;
		}
		QDomElement el(node.toElement());
		
		bool ok = true;
		int const id = el.attribute("id").toInt(&ok);
		if (!ok) {
			continue;
		}
		
		ImageId const image_id(reader.imageId(id));
		if (image_id.isNull()) {
			continue;
		}
		
		Settings::UpdateAction update;
		
		QString const layout_type(el.attribute("layoutType"));
		if (!layout_type.isEmpty()) {
			update.setLayoutType(layoutTypeFromString(layout_type));
		}
		
		QDomElement params_el(el.namedItem("params").toElement());
		if (!params_el.isNull()) {
			update.setParams(Params(params_el));
		}
		
		m_ptrSettings->updatePage(image_id, update);
	}
}

void
Filter::pageOrientationUpdate(
	ImageId const& image_id, OrthogonalRotation const& orientation)
{
	Settings::Record const record(m_ptrSettings->getPageRecord(image_id));

	if (record.layoutType() && *record.layoutType() != AUTO_LAYOUT_TYPE) {
		// The layout type was set manually, so we don't care about orientation.
		return;
	}

	if (record.params() && record.params()->dependencies().orientation() == orientation) {
		// We've already estimated the number of pages for this orientation.
		return;
	}

	// Use orientation to update the number of logical pages in an image.
	m_ptrPages->autoSetLayoutTypeFor(image_id, orientation);
}

void
Filter::writeImageSettings(
	QDomDocument& doc, QDomElement& filter_el,
	ImageId const& image_id, int const numeric_id) const
{
	Settings::Record const record(m_ptrSettings->getPageRecord(image_id));
	
	QDomElement image_el(doc.createElement("image"));
	image_el.setAttribute("id", numeric_id);
	if (LayoutType const* layout_type = record.layoutType()) {
		image_el.setAttribute(
			"layoutType", layoutTypeToString(*layout_type)
		);
	}
	
	if (Params const* params = record.params()) {
		image_el.appendChild(params->toXml(doc, "params"));
		filter_el.appendChild(image_el);
	}
}

IntrusivePtr<Task>
Filter::createTask(
	PageInfo const& page_info,
	IntrusivePtr<deskew::Task> const& next_task,
	bool const batch_processing, bool const debug)
{
	return IntrusivePtr<Task>(
		new Task(
			IntrusivePtr<Filter>(this), m_ptrSettings, m_ptrPages,
			next_task, page_info, batch_processing, debug
		)
	);
}

IntrusivePtr<CacheDrivenTask>
Filter::createCacheDrivenTask(
	IntrusivePtr<deskew::CacheDrivenTask> const& next_task)
{
	return IntrusivePtr<CacheDrivenTask>(
		new CacheDrivenTask(m_ptrSettings, next_task)
	);
}

std::vector<PageOrderOption>
Filter::pageOrderOptions() const
{
	return m_pageOrderOptions;
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

} // page_split
