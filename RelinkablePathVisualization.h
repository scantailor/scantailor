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

#ifndef RELINKABLE_PATH_VISUALIZATION_H_
#define RELINKABLE_PATH_VISUALIZATION_H_

#include <QWidget>
#include <QString>

class RelinkablePath;
class QHBoxLayout;
class QAbstractButton;

class RelinkablePathVisualization : public QWidget
{
	Q_OBJECT
public:
	RelinkablePathVisualization(QWidget* parent = 0);

	void clear();

	void setPath(RelinkablePath const& path, bool clickable);
signals:
	/** \p type is either RelinkablePath::File or RelinkablePath::Dir */
	void clicked(QString const& prefix_path, QString const& suffix_path, int type);
protected:
	virtual void paintEvent(QPaintEvent* evt);
private:
	struct PathComponent;
	class ComponentButton;

	void onClicked(int component_idx, QString const& prefix_path, QString const& suffix_path, int type);

	void stylePathComponentButton(QAbstractButton* btn, bool exists);

	static void checkForExistence(std::vector<PathComponent>& components);

	QHBoxLayout* m_pLayout;
};

#endif
