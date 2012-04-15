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

#ifndef UTILS_H_
#define UTILS_H_

#include <QString>
#include "ThumbnailPixmapCache.h"

class Utils
{
public:
	template<typename M, typename K, typename V>
	static typename M::iterator mapSetValue(
		M& map, K const& key, V const& val);
	
	/**
	 * \brief If \p output_dir exists, creates a "cache" subdirectory under it.
	 *
	 * The idea is to prevent creating a bogus directory structure when loading
	 * a project created on another machine.
	 */
	static void maybeCreateCacheDir(QString const& output_dir);

	static QString outputDirToThumbDir(QString const& output_dir);

	static IntrusivePtr<ThumbnailPixmapCache> createThumbnailCache(QString const& output_dir);

	/**
	 * Unlike QFile::rename(), this one overwrites existing files.
	 */
	static bool overwritingRename(QString const& from, QString const& to);
	
	/**
	 * \brief A high precision, locale independent number to string conversion.
	 *
	 * This function is intended to be used instead of
	 * QDomElement::setAttribute(double), which is locale dependent.
	 */
	static QString doubleToString(double val) {
		return QString::number(val, 'g', 16);
	}
	
	/**
	 * \brief Generate rich text, complete with headers and stuff,
	 *        for a clickable link.
	 *
	 * \param label The text to show as a link.
	 * \param target A URL or something else.  If the link will
	 *        be used in a QLable, this string will be passed
	 *        to QLabel::linkActivated(QString const&).
	 * \return The resulting reach text.
	 */
	static QString richTextForLink(
		QString const& label,
		QString const& target = QString(QChar('#')));
};

template<typename M, typename K, typename V>
typename M::iterator
Utils::mapSetValue(M& map, K const& key, V const& val)
{
	typename M::iterator const it(map.lower_bound(key));
	if (it == map.end() || map.key_comp()(key, it->first)) {
		return map.insert(it, typename M::value_type(key, val));
	} else {
		it->second = val;
		return it;
	}
}

#endif
