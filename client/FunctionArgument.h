#ifndef FUNCTIONARGUMENT_H
#define FUNCTIONARGUMENT_H

extern "C" {
#include "utils/types.h"
}
#include <QtCore/QVector>
#include <QtCore/QString>
#include <QtCore/QSharedPointer>

class FunctionArgument 
{
public:
	FunctionArgument(DBG_TYPE type = DBG_TYPE_CHAR);
	FunctionArgument(DBG_TYPE type, void *data, void* address);
	FunctionArgument(const FunctionArgument& rhs);
	~FunctionArgument();

	void* data() const { return static_cast<void*>(_data); }
	/* this assumes DBG_TYPE is correctly set */
	void data(void *data);

	DBG_TYPE type() const { return _type; }

	void const * address() const { return static_cast<void*>(_address); }
	void* address() { return static_cast<void*>(_address); }
	void address(void *address) { _address = static_cast<char*>(address); }

	bool operator==(const FunctionArgument& rhs) const;
	bool operator!=(const FunctionArgument& rhs) const;

	/* checks wether the argument has been modified */
	bool isDirty() const { return _dirty; }

	QString asString() const;
private:
	DBG_TYPE _type;
	char *_data;
	char *_address;
	bool _dirty;
};
typedef QSharedPointer<FunctionArgument> FunctionArgumentPtr;
typedef QVector<FunctionArgumentPtr> ArgumentVector;

#endif
