#include "types.h"
#include "notify.h"
#include "glenumerants.h"
#include "../GL/gl.h"

size_t sizeof_dbg_type(enum DBG_TYPE d)
{
    switch (d)
    {
    case DBG_TYPE_CHAR:
        return sizeof(char);
    case DBG_TYPE_UNSIGNED_CHAR:
        return sizeof(unsigned char);
    case DBG_TYPE_SHORT_INT:
        return sizeof(short);
    case DBG_TYPE_UNSIGNED_SHORT_INT:
        return sizeof(unsigned short);
    case DBG_TYPE_INT:
        return sizeof(int);
    case DBG_TYPE_UNSIGNED_INT:
        return sizeof(unsigned int);
    case DBG_TYPE_LONG_INT:
        return sizeof(long);
    case DBG_TYPE_UNSIGNED_LONG_INT:
        return sizeof(unsigned long);
    case DBG_TYPE_LONG_LONG_INT:
        return sizeof(long long);
    case DBG_TYPE_UNSIGNED_LONG_LONG_INT:
        return sizeof(unsigned long long);
    case DBG_TYPE_FLOAT:
        return sizeof(float);
    case DBG_TYPE_DOUBLE:
        return sizeof(double);
    case DBG_TYPE_POINTER:
        return sizeof(void *);
    case DBG_TYPE_BOOLEAN:
        return sizeof(GLboolean);
    case DBG_TYPE_BITFIELD:
        return sizeof(GLbitfield);
    case DBG_TYPE_ENUM:
        return sizeof(GLbitfield);
    case DBG_TYPE_STRUCT:
        UT_NOTIFY(LV_ERROR, "sizeof DBG_TYPE_STRUCT not implemented");
        return 0;
    default:
        UT_NOTIFY(LV_ERROR, "invalid type");
        return 0;
    }
    return 0;
}

void print_dbg_type(enum DBG_TYPE type, const void* buf)
{
    switch (type) {
    case DBG_TYPE_CHAR:
        UT_NOTIFY_NO_PRFX("%hhi", *(const char*)buf);
        break;
    case DBG_TYPE_UNSIGNED_CHAR:
        UT_NOTIFY_NO_PRFX("%hhu", *(const unsigned char*)buf);
        break;
    case DBG_TYPE_SHORT_INT:
        UT_NOTIFY_NO_PRFX("%hi", *(const short*)buf);
        break;
    case DBG_TYPE_UNSIGNED_SHORT_INT:
        UT_NOTIFY_NO_PRFX("%hu", *(const unsigned short*)buf);
        break;
    case DBG_TYPE_INT:
        UT_NOTIFY_NO_PRFX("%i", *(const int*)buf);
        break;
    case DBG_TYPE_UNSIGNED_INT:
        UT_NOTIFY_NO_PRFX("%u", *(const unsigned int*)buf);
        break;
    case DBG_TYPE_LONG_INT:
        UT_NOTIFY_NO_PRFX("%li", *(const long*)buf);
        break;
    case DBG_TYPE_UNSIGNED_LONG_INT:
        UT_NOTIFY_NO_PRFX("%lu", *(const unsigned long*)buf);
        break;
    case DBG_TYPE_LONG_LONG_INT:
        UT_NOTIFY_NO_PRFX("%ll", *(const long long*)buf);
        break;
    case DBG_TYPE_UNSIGNED_LONG_LONG_INT:
        UT_NOTIFY_NO_PRFX("%lu", *(const unsigned long long*)buf);
        break;
    case DBG_TYPE_FLOAT:
        UT_NOTIFY_NO_PRFX("%f", *(const float*)buf);
        break;
    case DBG_TYPE_DOUBLE:
        UT_NOTIFY_NO_PRFX("%lf", *(const double*)buf);
        break;
    case DBG_TYPE_POINTER:
        UT_NOTIFY_NO_PRFX("%p", *(const void**)buf);
        break;
    case DBG_TYPE_BOOLEAN:
        UT_NOTIFY_NO_PRFX("%s", *(const GLboolean*)buf ? "TRUE" : "FALSE");
        break;
    case DBG_TYPE_BITFIELD:
    {
        char *s = dissectBitfield(*(GLbitfield*) buf);
        UT_NOTIFY_NO_PRFX("%s", s);
        free(s);
        break;
    }
    case DBG_TYPE_ENUM:
        UT_NOTIFY_NO_PRFX("%s", lookupEnum(*(GLenum*)buf));
        break;
    case DBG_TYPE_STRUCT:
        UT_NOTIFY_NO_PRFX("STRUCT");
        break;
    default:
        UT_NOTIFY_NO_PRFX("UNKNOWN TYPE [%i]", type);
    }
}