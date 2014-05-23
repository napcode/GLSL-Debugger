#include "types.h"
#include "notify.h"
#include "glenumerants.h"
#include "../GL/gl.h"

size_t sizeof_dbg_type(Proto__DebugType d)
{
    switch (d)
    {
    case PROTO__DEBUG_TYPE__CHAR:
        return sizeof(char);
    case PROTO__DEBUG_TYPE__UNSIGNED_CHAR:
        return sizeof(unsigned char);
    case PROTO__DEBUG_TYPE__SHORT:
        return sizeof(short);
    case PROTO__DEBUG_TYPE__UNSIGNED_SHORT:
        return sizeof(unsigned short);
    case PROTO__DEBUG_TYPE__INT:
        return sizeof(int);
    case PROTO__DEBUG_TYPE__UNSIGNED_INT:
        return sizeof(unsigned int);
    case PROTO__DEBUG_TYPE__LONG_INT:
        return sizeof(long);
    case PROTO__DEBUG_TYPE__UNSIGNED_LONG_INT:
        return sizeof(unsigned long);
    case PROTO__DEBUG_TYPE__LONG_LONG_INT:
        return sizeof(long long);
    case PROTO__DEBUG_TYPE__UNSIGNED_LONG_LONG_INT:
        return sizeof(unsigned long long);
    case PROTO__DEBUG_TYPE__FLOAT:
        return sizeof(float);
    case PROTO__DEBUG_TYPE__DOUBLE:
        return sizeof(double);    
    case PROTO__DEBUG_TYPE__LONGDOUBLE:
        return sizeof(long double);
    case PROTO__DEBUG_TYPE__POINTER:
        return sizeof(void *);
    case PROTO__DEBUG_TYPE__BOOL:
        return sizeof(GLboolean);
    case PROTO__DEBUG_TYPE__BITFIELD:
        return sizeof(GLbitfield);
    case PROTO__DEBUG_TYPE__ENUM:
        return sizeof(GLbitfield);
    case PROTO__DEBUG_TYPE__STRUCT:
        UT_NOTIFY(LV_ERROR, "sizeof PROTO__DEBUG_TYPE__STRUCT not implemented");
        return 0;
    default:
        UT_NOTIFY(LV_ERROR, "invalid type %d", d);
        return 0;
    }
    return 0;
}

void print_dbg_type(Proto__DebugType type, const void* buf)
{
    switch (type) {
    case PROTO__DEBUG_TYPE__CHAR:
        UT_NOTIFY_NO_PRFX("%hhi", *(const char*)buf);
        break;
    case PROTO__DEBUG_TYPE__UNSIGNED_CHAR:
        UT_NOTIFY_NO_PRFX("%hhu", *(const unsigned char*)buf);
        break;
    case PROTO__DEBUG_TYPE__SHORT:
        UT_NOTIFY_NO_PRFX("%hi", *(const short*)buf);
        break;
    case PROTO__DEBUG_TYPE__UNSIGNED_SHORT:
        UT_NOTIFY_NO_PRFX("%hu", *(const unsigned short*)buf);
        break;
    case PROTO__DEBUG_TYPE__INT:
        UT_NOTIFY_NO_PRFX("%i", *(const int*)buf);
        break;
    case PROTO__DEBUG_TYPE__UNSIGNED_INT:
        UT_NOTIFY_NO_PRFX("%u", *(const unsigned int*)buf);
        break;
    case PROTO__DEBUG_TYPE__LONG_INT:
        UT_NOTIFY_NO_PRFX("%li", *(const long*)buf);
        break;
    case PROTO__DEBUG_TYPE__UNSIGNED_LONG_INT:
        UT_NOTIFY_NO_PRFX("%lu", *(const unsigned long*)buf);
        break;
    case PROTO__DEBUG_TYPE__LONG_LONG_INT:
        UT_NOTIFY_NO_PRFX("%ll", *(const long long*)buf);
        break;
    case PROTO__DEBUG_TYPE__UNSIGNED_LONG_LONG_INT:
        UT_NOTIFY_NO_PRFX("%lu", *(const unsigned long long*)buf);
        break;
    case PROTO__DEBUG_TYPE__FLOAT:
        UT_NOTIFY_NO_PRFX("%f", *(const float*)buf);
        break;
    case PROTO__DEBUG_TYPE__DOUBLE:
        UT_NOTIFY_NO_PRFX("%lf", *(const double*)buf);
        break;
    case PROTO__DEBUG_TYPE__LONGDOUBLE:
        UT_NOTIFY_NO_PRFX("%Lf", *(const long double*)buf);
        break;
    case PROTO__DEBUG_TYPE__POINTER:
        UT_NOTIFY_NO_PRFX("%p", *(const void**)buf);
        break;
    case PROTO__DEBUG_TYPE__BOOL:
        UT_NOTIFY_NO_PRFX("%s", *(const GLboolean*)buf ? "TRUE" : "FALSE");
        break;
    case PROTO__DEBUG_TYPE__BITFIELD:
    {
        char *s = dissectBitfield(*(GLbitfield*) buf);
        UT_NOTIFY_NO_PRFX("%s", s);
        free(s);
        break;
    }
    case PROTO__DEBUG_TYPE__ENUM:
        UT_NOTIFY_NO_PRFX("%s", lookupEnum(*(GLenum*)buf));
        break;
    case PROTO__DEBUG_TYPE__STRUCT:
        UT_NOTIFY_NO_PRFX("STRUCT");
        break;
    default:
        UT_NOTIFY_NO_PRFX("UNKNOWN TYPE [%i]", type);
    }
}
