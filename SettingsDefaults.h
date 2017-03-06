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

#ifndef SETTINGS_DEFAULTS_H_
#define SETTINGS_DEFAULTS_H_

#include <QSettings>
#include <QVector>

/*
 * These are helper classes that let easily register a callback function
 * in any .cpp file that will be called at application start.
 * The function is expected to write default settings hints to QSettings
 * as QSettings strips out commentaries.
 *
 * Callback function prototype receives bool argument that indicates cases
 * of GUI and console applications. It returns true if everything is fine
 *
 * To register a callback one shall include this header file
 * and create static instance of RegisterSettingsDefaults class passing
 * pointer to a callback in its constructor.
 *
 */

// Callback function prototype
typedef bool (*funcSettingsDefaults)(bool);

// Static class that collects pointers to all callback functions

class SettingsDefaults
{
public:
    static bool registerSettingsFunc(funcSettingsDefaults funcCallback, bool isGUI = true)
    {
        // local static variable behaves like global
        static QVector<funcSettingsDefaults> knownFuncs;

        // funcCallback is always null if function called from main
        // and must be non null if called from other cpp modules
        // isGUI value is meaningful only in first case.

        if (funcCallback != NULL) {
            knownFuncs.append(funcCallback);
        } else {
            foreach (funcSettingsDefaults func, knownFuncs) {
                if (!func(isGUI)) return false;
            }
        }
        return true;
    }

    // wrapper function that interfaces application main()
    static bool prepareDefaults(bool isGUI)
    {
        return registerSettingsFunc(nullptr, isGUI);
    }

};

// helper class that allows to call a static function at cpp module load time

class RegisterSettingsDefaults
{
public:
    RegisterSettingsDefaults(funcSettingsDefaults funcCallback)  { SettingsDefaults::registerSettingsFunc(funcCallback); }
};


#endif
