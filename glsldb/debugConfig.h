#ifndef DEBUGCONFIG_H
#define DEBUGCONFIG_H

#include <QtCore/QStringList>

struct DebugConfig 
{
	QStringList programArgs;
	QString workDir;
    bool traceFork = false;
    bool traceExec = false;
    bool traceClone = false;
};

#endif
