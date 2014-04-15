#ifndef FUNCTIONSMAP_H
#define FUNCTIONSMAP_H

#include <QtCore/QString>
#include <QtCore/QHash>
#include "proto/protocol.h"

typedef QHash<QString, proto::GLFunction> GLFunctionsMap;

class FunctionsMap {
public:
	static FunctionsMap& instance();
	void initialize(proto::GLFunctions& list);
	const proto::GLFunction& operator[](const QString& name);
private:
	static FunctionsMap* _instance;
	GLFunctionsMap _map;

};

#endif
