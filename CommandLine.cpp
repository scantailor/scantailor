#include "CommandLine.h"
#include <QMap>
#include <QRegExp>
#include <QStringList>

QMap<QString, QString> CommandLine::options;

void CommandLine::parse_cli(int argc, char **argv)
{
	QStringList args;

	for (int i=0; i<argc; i++) {
		args << argv[i];
	}

	CommandLine::parse_cli(args);
}

void CommandLine::parse_cli(QStringList const& argv)
{
	QRegExp rx("^--([^=]+)=(.*)$");
	QRegExp rx_switch("^--([^=]+)$");

	for (int i=0; i<argv.size(); i++) {
		if (rx.exactMatch(argv[i])) {
			CommandLine::options[rx.cap(1)] = rx.cap(2);
		} else if (rx_switch.exactMatch(argv[i])) {
			CommandLine::options[rx_switch.cap(1)] = "true";
		}
	}
}
