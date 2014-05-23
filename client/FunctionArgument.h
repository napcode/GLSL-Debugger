#ifndef FUNCTIONARGUMENT_H
#define FUNCTIONARGUMENT_H

#include "utils/types.h"
#include <QtCore/QVector>
#include <QtCore/QString>
#include <QtCore/QSharedPointer>

class FunctionArgument 
{
public:
	FunctionArgument(proto::DebugType type = proto::DebugType::CHAR);
	FunctionArgument(proto::DebugType type, void *data, void* address);
	FunctionArgument(const FunctionArgument& rhs);
	FunctionArgument(const proto::FunctionArgument& rhs);
	~FunctionArgument();

	void* data() const { return static_cast<void*>(_data); }
	/* this assumes proto::DebugType is correctly set */
	void data(const void *data);

	proto::DebugType type() const { return _type; }

	void const * address() const { return static_cast<void*>(_address); }
	void* address() { return static_cast<void*>(_address); }
	void address(void *address) { _address = static_cast<char*>(address); }

	bool operator==(const FunctionArgument& rhs) const;
	bool operator!=(const FunctionArgument& rhs) const;

	/* checks wether the argument has been modified */
	bool isDirty() const { return _dirty; }

	QString asString() const;
private:
	proto::DebugType _type;
	char *_data;
	char *_address;
	bool _dirty;
};
typedef QSharedPointer<FunctionArgument> FunctionArgumentPtr;
typedef QVector<FunctionArgumentPtr> ArgumentVector;

#endif
