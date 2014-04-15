#include "FunctionsMap.h"
#include "utils/notify.h"

FunctionsMap* FunctionsMap::_instance = 0;

FunctionsMap& FunctionsMap::instance()
{
	if (!_instance) {
		_instance = new FunctionsMap;
	}
	return *_instance;
}

void FunctionsMap::initialize(proto::GLFunctions& list)
{
	int32_t i = 0;
	_map.clear();
	while (list.function_size() > i) {
		_map[QString::fromStdString(list.function(i).name())] = list.function(i);
		++i;
	}
	UT_NOTIFY(LV_INFO, _map.size() << " registered GL-Functions");
}
const proto::GLFunction& FunctionsMap::operator[](const QString& name)
{
	GLFunctionsMap::const_iterator it = _map.find(name);
	if (it != _map.end())
		throw std::logic_error("no such GL-function");
	return it.value();
}
