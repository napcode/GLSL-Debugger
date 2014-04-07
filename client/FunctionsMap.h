#ifndef FUNCTIONSMAP_H
#define FUNCTIONSMAP_H

#include <QtCore/QString>
#include <QtCore/QHash>
#include "DebugLib/debuglib.h"

typedef QHash<QString, gl_func_t*> GLFunctionsMap;

class FunctionsMap {
public:
	static FunctionsMap& instance();
	void initialize();
	gl_func_t* operator[](const QString& name);
private:
	static FunctionsMap* _instance;
	GLFunctionsMap _map;

};

#endif
