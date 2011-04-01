/*
    Scan Tailor - Interactive post-processing tool for scanned pages.

    ConsoleBatch - Batch processing scanned pages from command line.
    Copyright (C) 2011 Petr Kovar <pejuko@gmail.com>

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

#ifndef CONSOLEBATCH_H_
#define CONSOLEBATCH_H_

#include <QString>
#include <vector>

#include "IntrusivePtr.h"
#include "BackgroundTask.h"
#include "FilterResult.h"
#include "OutputFileNameGenerator.h"
#include "PageId.h"
#include "PageInfo.h"
#include "PageView.h"
#include "ProjectPages.h"
#include "ImageFileInfo.h"
#include "ThumbnailPixmapCache.h"
#include "OutputFileNameGenerator.h"
#include "StageSequence.h"
#include "PageSelectionAccessor.h"
#include "ProjectReader.h"


class ConsoleBatch
{
	// Member-wise copying is OK.
public:
	ConsoleBatch(
			std::vector<ImageFileInfo> const& images,
			QString                    const& output_directory,
			Qt::LayoutDirection        const  layout);
	ConsoleBatch(QString const project_file);

	void process();
	void saveProject(QString const project_file);

private:
	bool batch;
	bool debug;
	IntrusivePtr<FileNameDisambiguator> m_ptrDisambiguator;
	IntrusivePtr<ProjectPages> m_ptrPages;
	IntrusivePtr<StageSequence> m_ptrStages;
	OutputFileNameGenerator m_outFileNameGen;
	IntrusivePtr<ThumbnailPixmapCache> m_ptrThumbnailCache;
	std::auto_ptr<ProjectReader> m_ptrReader;

	void setupFilter(int idx, std::set<PageId> allPages);
	void setupFixOrientation(std::set<PageId> allPages);
	void setupPageSplit(std::set<PageId> allPages);
	void setupDeskew(std::set<PageId> allPages);
	void setupSelectContent(std::set<PageId> allPages);
	void setupPageLayout(std::set<PageId> allPages);
	void setupOutput(std::set<PageId> allPages);

	BackgroundTaskPtr createCompositeTask(
		PageInfo const& page,
		int const last_filter_idx
	);
};

#endif
