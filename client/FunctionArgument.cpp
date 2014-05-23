#include "FunctionArgument.h"
#include <cstring>
#include <cassert>
#include "GL/gl.h"
extern "C" {
#include "utils/glenumerants.h"
}

FunctionArgument::FunctionArgument(proto::DebugType type)
    :   _type(type),
        _data(new char[sizeof_dbg_type(type)]),
        _address(nullptr), 
        _dirty(false)
{

}

FunctionArgument::FunctionArgument(proto::DebugType type, void *data, void *address)
    :   FunctionArgument(type) 
{
    /* don't call data() since it will set the dirty flag */
    std::memmove(_data, data, sizeof_dbg_type(_type));
    FunctionArgument::address(address);
}

FunctionArgument::FunctionArgument(const FunctionArgument& rhs)
    :   FunctionArgument(rhs._type, rhs._data, rhs._address)
{
}

FunctionArgument::FunctionArgument(const proto::FunctionArgument& arg)
    :   FunctionArgument(arg.type()) 
{
    FunctionArgument::address((char*)arg.address());
    std::memmove(_data, arg.data().c_str(), sizeof_dbg_type(_type));
}

FunctionArgument::~FunctionArgument()
{
    delete[] _data;
}

bool FunctionArgument::operator==(const FunctionArgument &rhs) const
{
    if (_type != rhs._type)
        return false;

    switch (_type)
    {
    case proto::DebugType::CHAR:
        if (*(char *) _data == *(char *) rhs._data)
            return true;
        break;
    case proto::DebugType::UNSIGNED_CHAR:
        if (*(unsigned char *) _data == *(unsigned char *) rhs._data)
            return true;
        break;
    case proto::DebugType::SHORT:
        if (*(short *) _data == *(short *) rhs._data)
            return true;
        break;
    case proto::DebugType::UNSIGNED_SHORT:
        if (*(unsigned short *) _data
                == *(unsigned short *) rhs._data)
            return true;
        break;
    case proto::DebugType::INT:
        if (*(int *) _data == *(int *) rhs._data)
            return true;
        break;
    case proto::DebugType::UNSIGNED_INT:
        if (*(unsigned int *) _data
                == *(unsigned int *) rhs._data)
            return true;
        break;
    case proto::DebugType::LONG_INT:
        if (*(long *) _data == *(long *) rhs._data)
            return true;
        break;
    case proto::DebugType::UNSIGNED_LONG_INT:
        if (*(unsigned long *) _data
                == *(unsigned long *) rhs._data)
            return true;
        break;
    case proto::DebugType::LONG_LONG_INT:
        if (*(long long *) _data
                == *(long long *) rhs._data)
            return true;
        break;
    case proto::DebugType::UNSIGNED_LONG_LONG_INT:
        if (*(unsigned long long *) _data == *(unsigned long long *) rhs._data)
            return true;
        break;
    case proto::DebugType::FLOAT:
        if (*(float *) _data == *(float *) rhs._data)
            return true;
        break;
    case proto::DebugType::DOUBLE:
        if (*(double *) _data == *(double *) rhs._data)
            return true;
        break;
    case proto::DebugType::POINTER:
        if (*(void **) _data == *(void **) rhs._data)
            return true;
        break;
    case proto::DebugType::BOOL:
        if (*(GLboolean *) _data == *(GLboolean *) rhs._data)
            return true;
        break;
    case proto::DebugType::BITFIELD:
        if (*(GLbitfield *) _data == *(GLbitfield *) rhs._data)
            return true;
        break;
    case proto::DebugType::ENUM:
        if (*(GLenum *) _data == *(GLenum *) rhs._data)
            return true;
        break;
    case proto::DebugType::STRUCT:
        return false; /* FIXME */
        break;
    default:
        return false;
    }
    return false;
}
bool FunctionArgument::operator!=(const FunctionArgument &rhs) const
{
    return !operator==(rhs);
}

void FunctionArgument::data(const void *data)
{
    assert(data);
	std::memmove(_data, data, sizeof_dbg_type(_type));
    _dirty = true;
}

QString FunctionArgument::asString() const
{
    switch (_type)
    {
    case proto::DebugType::CHAR:
    	return QString::fromLatin1((char *)_data, 1);
    case proto::DebugType::UNSIGNED_CHAR:
        //return QString::fromLatin1((const unsigned char *)_data, 1);
        {
            QString s;
            s.sprintf("%i", *(unsigned char*)_data);
            return s;
        }
    case proto::DebugType::SHORT:
        return QString::number(*(short *)_data);        
    case proto::DebugType::UNSIGNED_SHORT:
        return QString::number(*(unsigned short *)_data);        
    case proto::DebugType::INT:
        return QString::number(*(int *)_data);        
    case proto::DebugType::UNSIGNED_INT:
        return QString::number(*(unsigned int *)_data);        
    case proto::DebugType::LONG_INT:
        return QString::number(*(long *)_data);        
    case proto::DebugType::UNSIGNED_LONG_INT:
        return QString::number(*(unsigned long *)_data);        
    case proto::DebugType::LONG_LONG_INT:
        return QString::number(*(long long *)_data);        
    case proto::DebugType::UNSIGNED_LONG_LONG_INT:
        return QString::number(*(unsigned long long *)_data);        
    case proto::DebugType::FLOAT:
        return QString::number(*(float *)_data);        
    case proto::DebugType::DOUBLE:
        return QString::number(*(double *)_data);        
    case proto::DebugType::POINTER:
        {
            QString s;
            s.sprintf("%p", *(void**)_data);
            return s;
        }
    case proto::DebugType::BOOL:
        return (*(GLboolean*)_data) ? QString("true") : QString("false");
    case proto::DebugType::BITFIELD:
        {
            char *s = dissectBitfield(*(GLbitfield *) _data);
            QString t(s);
            free(s);
            return t;
        }
    case proto::DebugType::ENUM:
        return QString(lookupEnum(*(GLenum *) _data));
    case proto::DebugType::STRUCT:
        return QString("STRUCT");
    default:
        return QString("UNKNOWN TYPE ID");
    }
    return QString("DUMMY");
}
