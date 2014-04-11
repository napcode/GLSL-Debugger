#include "Command.qt.h"
#include "DebugCommand.h"
#include "Process.qt.h"
#include "build-config.h"
#include <cstring>
#include <stdexcept>
#include <cassert>
#include <cstdlib>

std::atomic<uint64_t> Command::_seq_number = ATOMIC_VAR_INIT(123);

Command::Command(Process &p,  proto::ClientRequest::Type t) :
    _proc(p)
{
    _message.set_type(t);
    _message.set_id(_seq_number++);
}
/*
void Command::dbgCommandExecuteToDrawCall(bool stopOnGLError)
{
    static const QString msg("DBG_EXECUTE (DBG_JUMP_TO_DRAW_CALL)");
    _message = &msg;
    _rec->operation = DBG_EXECUTE;
    _rec->items[0] = DBG_JUMP_TO_DRAW_CALL;
    _rec->items[1] = stopOnGLError ? 1 : 0;
}

void Command::dbgCommandExecuteToShaderSwitch(bool stopOnGLError)
{
    static const QString msg("DBG_EXECUTE (DBG_JUMP_TO_SHADER_SWITCH)");
    _message = &msg;
    _rec->operation = DBG_EXECUTE;
    _rec->items[0] = DBG_JUMP_TO_SHADER_SWITCH;
    _rec->items[1] = stopOnGLError ? 1 : 0;
}

void Command::dbgCommandExecuteToUserDefined(const QString& fname,
        bool stopOnGLError)
{
    static const QString msg("DBG_EXECUTE (DBG_JUMP_TO_USER_DEFINED)");
    _message = &msg;
    _rec->operation = DBG_EXECUTE;
    _rec->items[0] = DBG_JUMP_TO_USER_DEFINED;
    _rec->items[1] = stopOnGLError ? 1 : 0;
    strncpy(_rec->fname, fname.toStdString().c_str(), SHM_MAX_FUNCNAME);
}

void ProgramControl::dbgCommandDone()
{
    static const QString msg("DBG_DONE");
    _message = &msg;
    _rec->operation = DBG_DONE;
}

void ProgramControl::dbgCommandSetDbgTarget(int target,
        int alphaTestOption, int depthTestOption, int stencilTestOption,
        int blendingOption)
{
    static const QString msg("DBG_SET_DBG_TARGET");
    _message = &msg;
    _rec->operation = DBG_SET_DBG_TARGET;
    _rec->items[0] = target;
    _rec->items[1] = alphaTestOption;
    _rec->items[2] = depthTestOption;
    _rec->items[3] = stencilTestOption;
    _rec->items[4] = blendingOption;
}

void ProgramControl::dbgCommandRestoreRenderTarget(int target)
{
    static const QString msg("DBG_RESTORE_RENDER_TARGET");
    _message = &msg;
    _rec->operation = DBG_RESTORE_RENDER_TARGET;
    _rec->items[0] = target;
}

void ProgramControl::dbgCommandSaveAndInterruptQueries(void)
{
    static const QString msg("DBG_SAVE_AND_INTERRUPT_QUERIES");
    _message = &msg;
    _rec->operation = DBG_SAVE_AND_INTERRUPT_QUERIES;
}

void ProgramControl::dbgCommandRestartQueries(void)
{
    static const QString msg("DBG_RESTART_QUERIES");
    _message = &msg;
    _rec->operation = DBG_RESTART_QUERIES;
}

void ProgramControl::dbgCommandStartRecording()
{
    static const QString msg("DBG_START_RECORDING");
    _message = & msg;
    _rec->operation = DBG_START_RECORDING;
}

void ProgramControl::dbgCommandReplay(int)
{
    static const QString msg("DBG_REPLAY");
    _message = & msg;
    _rec->operation = DBG_REPLAY;
}

void ProgramControl::dbgCommandEndReplay()
{
    static const QString msg("DBG_END_REPLAY");
    _message = & msg;
    _rec->operation = DBG_END_REPLAY;
}

void Command::dbgCommandRecord()
{
    static const QString msg("DBG_RECORD_CALL");
    _message = & msg;
    _rec->operation = DBG_RECORD_CALL;
}

void Command::dbgCommandCallOrig(FunctionCallPtr call)
{
    if (call->name() != _rec->fname) {
        UT_NOTIFY(LV_ERROR, "function name does not match record.");
        return;
    }

    _rec->operation = DBG_CALL_ORIGFUNCTION;

    if(!call->isDirty()) {
        static const QString msg("DBG_CALL_ORIGFUNCTION");
        _message = & msg;
    }
    else {
        static const QString msg("DBG_CALL_ORIGFUNCTION (with modified arguments)");
        _message = & msg;
        _rec->numItems = call->numArguments();

        int i = 0;
        UT_NOTIFY(LV_INFO, "Printing arguments:");
        for(auto &arg : call->arguments()) {
            _rec->items[2 * i + 1] = arg->type();
            UT_NOTIFY_NO_PRFX("arg " << i << ") ");
            print_dbg_type(arg->type(), arg->data());
            UT_NOTIFY_NO_PRFX("\n");
            copyArgumentToProcess(arg->address(), arg->data(), arg->type());
            ++i;
        }
    }
}

void Command::dbgCommandCallDBGFunction(const QString& fname)
{
    static const QString msg("DBG_CALL_FUNCTION");
    _message = & msg;
    strncpy(_rec->fname, fname.toStdString().c_str(), SHM_MAX_FUNCNAME);
    _rec->operation = DBG_CALL_FUNCTION;
}

void Command::dbgCommandFreeMem(unsigned int numBlocks,
        void **addresses)
{
    static const QString msg("DBG_FREE_MEM");
    _message = & msg;

    if (numBlocks > SHM_MAX_ITEMS) {
        UT_NOTIFY(LV_ERROR, "Cannot free " << numBlocks <<
            " memory blocks in one call! Max. is " << (int)SHM_MAX_ITEMS);
        exit(1);
    }

    _rec->operation = DBG_FREE_MEM;
    _rec->numItems = numBlocks;
    for (unsigned int i = 0; i < numBlocks; ++i) {
        _rec->items[i] = (ALIGNED_DATA) addresses[i];
    }
}

void Command::dbgCommandAllocMem(unsigned int numBlocks,
        unsigned int *sizes, void **addresses)
{
    static const QString msg("DBG_ALLOC_MEM");
    _message = & msg;

    if (numBlocks > SHM_MAX_ITEMS) {
        UT_NOTIFY(LV_ERROR, "Cannot allocate " << numBlocks <<
            " memory blocks in one call! Max. is " << (int)SHM_MAX_ITEMS);
        exit(1);
    }

    _rec->operation = DBG_ALLOC_MEM;
    _rec->numItems = numBlocks;
    for (unsigned int i = 0; i < numBlocks; ++i) {
        _rec->items[i] = sizes[i];
    }
}

void Command::dbgCommandClearRenderBuffer(int mode, float r,
        float g, float b, float a, float f, int s)
{
    static const QString msg("DBG_CLEAR_RENDER_BUFFER");
    _message = & msg;
    rec->operation = DBG_CLEAR_RENDER_BUFFER;
    rec->items[0] = (ALIGNED_DATA) mode;
    *(float*) (void*) &rec->items[1] = r;
    *(float*) (void*) &rec->items[2] = g;
    *(float*) (void*) &rec->items[3] = b;
    *(float*) (void*) &rec->items[4] = a;
    *(float*) (void*) &rec->items[5] = f;
    rec->items[6] = (ALIGNED_DATA) s;
}

void Command::dbgCommandReadRenderBuffer(int numComponents,
        int *width, int *height, float **image)
{
    static const QString msg("DBG_READ_RENDER_BUFFER");
    _message = & msg;

    debug_record_t *rec = getThreadRecord(_debuggeePID);
    ErrorCode error;

    rec->operation = DBG_READ_RENDER_BUFFER;
    rec->items[0] = (ALIGNED_DATA) numComponents;

    error = checkError();

    return error;
}
*/
// ErrorCode ProgramControl::dbgCommandShaderStepFragment(void *shaders[3],
//      int numComponents, int format, int *width, int *height, void **image)
// {
//  debug_record_t *rec = getThreadRecord(_debuggeePID);
//  ErrorCode error;

//  static const QString msg("DBG_SHADER_STEP");
//  _message = & msg;
//  rec->operation = DBG_SHADER_STEP;
//  rec->items[0] = (ALIGNED_DATA) shaders[0];
//  rec->items[1] = (ALIGNED_DATA) shaders[1];
//  rec->items[2] = (ALIGNED_DATA) shaders[2];
//  rec->items[3] = (ALIGNED_DATA) DBG_TARGET_FRAGMENT_SHADER;
//  rec->items[4] = (ALIGNED_DATA) numComponents;
//  rec->items[5] = (ALIGNED_DATA) format;
//  error = executeDbgCommand();
//  if (error != EC_NONE) {
//      return error;
//  }
//  error = checkError();
//  if (error == EC_NONE) {
//      if (rec->result == DBG_READBACK_RESULT_FRAGMENT_DATA) {
//          void *buffer = (void*) rec->items[0];
//          *width = (int) rec->items[1];
//          *height = (int) rec->items[2];
//          if (!buffer || *width <= 0 || *height <= 0) {
//              error = EC_DBG_INVALID_VALUE;
//          } else {
//              int formatSize;
//              switch (format) {
//              case GL_FLOAT:
//                  formatSize = sizeof(float);
//                  break;
//              case GL_INT:
//                  formatSize = sizeof(int);
//                  break;
//              case GL_UNSIGNED_INT:
//                  formatSize = sizeof(unsigned int);
//                  break;
//              default:
//                  return EC_DBG_INVALID_VALUE;
//              }

//              *image = malloc(
//                      numComponents * (*width) * (*height) * formatSize);

//              cpyFromProcess(_debuggeePID, *image, buffer,
//                      numComponents * (*width) * (*height) * formatSize);
//              error = dbgCommandFreeMem(1, &buffer);
//          }
//      } else {
//          error = EC_DBG_INVALID_VALUE;
//      }
//  }
//  return error;
// }

// ErrorCode ProgramControl::dbgCommandShaderStepVertex(void *shaders[3],
//      int target, int primitiveMode, int forcePointPrimitiveMode,
//      int numFloatsPerVertex, int *numPrimitives, int *numVertices,
//      float **vertexData)
// {
//  debug_record_t *rec = getThreadRecord(_debuggeePID);
//  ErrorCode error;

//  static const QString msg("DBG_SHADER_STEP");
//  _message = & msg;
//  rec->operation = DBG_SHADER_STEP;
//  rec->items[0] = (ALIGNED_DATA) shaders[0];
//  rec->items[1] = (ALIGNED_DATA) shaders[1];
//  rec->items[2] = (ALIGNED_DATA) shaders[2];
//  rec->items[3] = (ALIGNED_DATA) target;
//  rec->items[4] = (ALIGNED_DATA) primitiveMode;
//  rec->items[5] = (ALIGNED_DATA) forcePointPrimitiveMode;
//  rec->items[6] = (ALIGNED_DATA) numFloatsPerVertex;
//  error = executeDbgCommand();
//  if (error != EC_NONE) {
//      return error;
//  }
//  error = checkError();
//  if (error == EC_NONE) {
//      if (rec->result == DBG_READBACK_RESULT_VERTEX_DATA) {
//          void *buffer = (void*) rec->items[0];
//          *numVertices = (int) rec->items[1];
//          *numPrimitives = (int) rec->items[2];
//          *vertexData = (float*) malloc(
//                  *numVertices * numFloatsPerVertex * sizeof(float));
//          cpyFromProcess(_debuggeePID, *vertexData, buffer,
//                  *numVertices * numFloatsPerVertex * sizeof(float));
//          error = dbgCommandFreeMem(1, &buffer);
//      } else {
//          error = EC_DBG_INVALID_VALUE;
//      }
//  }
//  return error;
// }


// ErrorCode ProgramControl::getShaderCode(char *shaders[3],
//      TBuiltInResource *resource, char **serializedUniforms, int *numUniforms)
// {
//  debug_record_t *rec = getThreadRecord(_debuggeePID);
//  int i;
//  ErrorCode error;
//  void *addr[5];

// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  static const QString msg("DBG_GET_SHADER_CODE");
//  _message = & msg;
//  rec->operation = DBG_GET_SHADER_CODE;
//  error = executeDbgCommand();
//  if (error != EC_NONE) {
//      return error;
//  }
//  error = checkError();
//  if (error != EC_NONE) {
//      return error;
//  }

//  for (i = 0; i < 3; i++) {
//      shaders[i] = NULL;
//  }

//  if (rec->result != DBG_SHADER_CODE) {
//      return checkError();
//  }

//  if (rec->numItems > 0) {

//      addr[0] = (void*) rec->items[0];
//      addr[1] = (void*) rec->items[2];
//      addr[2] = (void*) rec->items[4];
//      addr[3] = (void*) rec->items[6];
//      addr[4] = (void*) rec->items[9];

//      /* copy shader sources */
//      for (i = 0; i < 3; i++) {
//          if (rec->items[2 * i] == 0) {
//              continue;
//          }
//          if (!(shaders[i] = new char[rec->items[2 * i + 1] + 1])) {
//              dbgPrint(DBGLVL_ERROR, "not enough memory\n");
//              for (--i; i >= 0; i--) {
//                  delete[] shaders[i];
//                  shaders[i] = NULL;
//              }
//              /* TODO: what about memory on client side? */
//              error = dbgCommandFreeMem(5, addr);
//              return EC_MEMORY_ALLOCATION_FAILED;
//          }
//          cpyFromProcess(_debuggeePID, shaders[i], addr[i],
//                  rec->items[2 * i + 1]);
//          shaders[i][rec->items[2 * i + 1]] = '\0';
//      }

//      /* copy shader resource info */
//      cpyFromProcess(_debuggeePID, resource, addr[3],
//              sizeof(TBuiltInResource));

//      if (rec->items[7] > 0) {
//          *numUniforms = rec->items[7];
//          if (!(*serializedUniforms = new char[rec->items[8]])) {
//              dbgPrint(DBGLVL_ERROR, "not enough memory\n");
//              for (i = 0; i < 3; ++i) {
//                  delete[] shaders[i];
//                  shaders[i] = NULL;
//                  *serializedUniforms = NULL;
//                  *numUniforms = 0;
//              }
//              /* TODO: what about memory on client side? */
//              error = dbgCommandFreeMem(5, addr);
//              return EC_MEMORY_ALLOCATION_FAILED;
//          }
//          cpyFromProcess(_debuggeePID, *serializedUniforms, addr[4],
//                  rec->items[8]);
//      } else {
//          *serializedUniforms = NULL;
//          *numUniforms = 0;
//      }

//      /* free memory on client side */
//      dbgPrint(DBGLVL_INFO,
//              "getShaderCode: free memory on client side [%p, %p, %p, %p, %p]\n", addr[0], addr[1], addr[2], addr[3], addr[4]);
//      error = dbgCommandFreeMem(5, addr);
//      if (error != EC_NONE) {
//          dbgPrint(DBGLVL_WARNING,
//                  "getShaderCode: free memory on client side error: %i\n", error);
//          for (i = 0; i < 3; i++) {
//              delete[] shaders[i];
//              shaders[i] = NULL;
//          }
//          return error;
//      }
//  }
//  dbgPrint(DBGLVL_INFO, ">>>>>>>>> Orig. Vertex Shader <<<<<<<<<<<\n%s\n"
//  ">>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<\n", shaders[0]);
//  dbgPrint(DBGLVL_INFO, ">>>>>>>> Orig. Geometry Shader <<<<<<<<<<\n%s\n"
//  ">>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<\n", shaders[1]);
//  dbgPrint(DBGLVL_INFO, ">>>>>>>> Orig. Fragment Shader <<<<<<<<<<\n%s\n"
//  ">>>>>>>>>>>>>>>>>>>>>><<<<<<<<<<<<<<<<<<<\n", shaders[2]);
//  return EC_NONE;
// }

// ErrorCode ProgramControl::saveActiveShader(void)
// {
//  debug_record_t *rec = getThreadRecord(_debuggeePID);
//  ErrorCode error;

// #ifdef _WIN32
//  ::SwitchToThread();
// #else  _WIN32
//  sched_yield();
// #endif /* _WIN32 */

//  static const QString msg("DBG_STORE_ACTIVE_SHADER");
//  _message = & msg;
//  rec->operation = DBG_STORE_ACTIVE_SHADER;
//  error = executeDbgCommand();
//  if (error != EC_NONE) {
//      return error;
//  }
//  return checkError();
// }

// ErrorCode ProgramControl::restoreActiveShader(void)
// {
//  debug_record_t *rec = getThreadRecord(_debuggeePID);
//  ErrorCode error;

// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  static const QString msg("DBG_RESTORE_ACTIVE_SHADER");
//  _message = & msg;
//  rec->operation = DBG_RESTORE_ACTIVE_SHADER;
//  error = executeDbgCommand();
//  if (error != EC_NONE) {
//      return error;
//  }
//  return checkError();
// }

// ErrorCode ProgramControl::setDbgShaderCode(char *shaders[3], int target)
// {
//  debug_record_t *rec = getThreadRecord(_debuggeePID);
//  int i;
//  ErrorCode error;
//  void *addr[3];

// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  dbgPrint(DBGLVL_INFO,
//          "setting SH code: %p, %p, %p, %i\n", shaders[0], shaders[1], shaders[2], target);

//  /* allocate client side memory and copy shader src */
//  for (i = 0; i < 3; i++) {
//      if (shaders[i]) {
//          unsigned int size = strlen(shaders[i]) + 1;
//          dbgPrint(DBGLVL_ERROR,
//                  "allocating memory for shader[%i]: %ibyte\n", i, size);
//          error = dbgCommandAllocMem(1, &size, &addr[i]);
//          if (error != EC_NONE) {
//              dbgCommandFreeMem(i, addr);
//              return error;
//          }
//          cpyToProcess(_debuggeePID, addr[i], shaders[i], size);
//      } else {
//          addr[i] = NULL;
//      }
//  }
//  static const QString msg("DBG_SET_DBG_SHADER");
//  _message = & msg;
//  for (i = 0; i < 3; i++) {
//      rec->items[i] = (ALIGNED_DATA) addr[i];
//  }
//  rec->operation = DBG_SET_DBG_SHADER;
//  rec->items[3] = target;
//  error = executeDbgCommand();
//  if (error != EC_NONE) {
//      dbgCommandFreeMem(3, addr);
//      return error;
//  }
//  error = checkError();
//  if (error != EC_NONE) {
//      dbgCommandFreeMem(3, addr);
//      return error;
//  }

//  /* free memory on client side */
//  dbgPrint(DBGLVL_INFO,
//          "setShaderCode: free memory on client side [%p, %p, %p]\n", addr[0], addr[1], addr[2]);
//  error = dbgCommandFreeMem(3, addr);
//  if (error != EC_NONE) {
//      dbgPrint(DBGLVL_ERROR,
//              "getShaderCode: free memory on client side error: %i\n", error);
//      return error;
//  }
//  return EC_NONE;
// }

// ErrorCode ProgramControl::initializeRenderBuffer(bool copyRGB, bool copyAlpha,
//      bool copyDepth, bool copyStencil, float red, float green, float blue,
//      float alpha, float depth, int stencil)
// {
//  int mode = DBG_CLEAR_NONE;

// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  if (!copyRGB) {
//      mode |= DBG_CLEAR_RGB;
//  }
//  if (!copyAlpha) {
//      mode |= DBG_CLEAR_ALPHA;
//  }
//  if (!copyDepth) {
//      mode |= DBG_CLEAR_DEPTH;
//  }
//  if (!copyStencil) {
//      mode |= DBG_CLEAR_STENCIL;
//  }
//  return dbgCommandClearRenderBuffer(mode, red, green, blue, alpha, depth,
//          stencil);
// }

// ErrorCode ProgramControl::readBackActiveRenderBuffer(int numComponents,
//      int *width, int *heigh, float **image)
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  return dbgCommandReadRenderBuffer(numComponents, width, heigh, image);
// }

// ErrorCode ProgramControl::insertGlEnd(void)
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  return dbgCommandCallDBGFunction("glEnd");
// }

// ErrorCode ProgramControl::callOrigFunc(FunctionCallPtr call)
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  return dbgCommandCallOrig(call);
// }

// ErrorCode ProgramControl::restoreRenderTarget(int target)
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  return dbgCommandRestoreRenderTarget(target);
// }

// ErrorCode ProgramControl::setDbgTarget(int target, int alphaTestOption,
//      int depthTestOption, int stencilTestOption, int blendingOption)
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  return dbgCommandSetDbgTarget(target, alphaTestOption, depthTestOption,
//          stencilTestOption, blendingOption);
// }

// ErrorCode ProgramControl::saveAndInterruptQueries(void)
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  return dbgCommandSaveAndInterruptQueries();
// }

// ErrorCode ProgramControl::restartQueries(void)
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  return dbgCommandRestartQueries();
// }

// ErrorCode ProgramControl::initRecording()
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  return dbgCommandStartRecording();
// }

// ErrorCode ProgramControl::recordCall()
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  return dbgCommandRecord();
// }

// ErrorCode ProgramControl::replay(int target)
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  return dbgCommandReplay(target);
// }

// ErrorCode ProgramControl::endReplay()
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  return dbgCommandEndReplay();
// }

// ErrorCode ProgramControl::shaderStepFragment(char *shaders[3],
//      int numComponents, int format, int *width, int *heigh, void **image)
// {
//  ErrorCode error;
//  void *addr[3];
//  int i;

// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  /* allocate client side memory and copy shader src */
//  for (i = 0; i < 3; i++) {
//      if (shaders[i]) {
//          unsigned int size = strlen(shaders[i]) + 1;
//          error = dbgCommandAllocMem(1, &size, &addr[i]);
//          if (error != EC_NONE) {
//              dbgCommandFreeMem(i, addr);
//              return error;
//          }
//          cpyToProcess(_debuggeePID, addr[i], shaders[i], size);
//      } else {
//          addr[i] = NULL;
//      }
//  }

//  error = dbgCommandShaderStepFragment(addr, numComponents, format, width,
//          heigh, image);
//  if (error) {
//      dbgCommandFreeMem(3, addr);
//      return error;
//  }

//  /* free memory on client side */
//  error = dbgCommandFreeMem(3, addr);
//  if (error) {
//      dbgPrint(DBGLVL_ERROR,
//              "getShaderCode: free memory on client side error: %i\n", error);
//  }
//  return error;
// }

// ErrorCode ProgramControl::shaderStepVertex(char *shaders[3], int target,
//      int primitiveMode, int forcePointPrimitiveMode, int numFloatsPerVertex,
//      int *numPrimitives, int *numVertices, float **vertexData)
// {
//  ErrorCode error;
//  void *addr[3];
//  int i, basePrimitiveMode;

// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  /* allocate client side memory and copy shader src */
//  for (i = 0; i < 3; i++) {
//      if (shaders[i]) {
//          unsigned int size = strlen(shaders[i]) + 1;
//          error = dbgCommandAllocMem(1, &size, &addr[i]);
//          if (error != EC_NONE) {
//              dbgCommandFreeMem(i, addr);
//              return error;
//          }
//          cpyToProcess(_debuggeePID, addr[i], shaders[i], size);
//      } else {
//          addr[i] = NULL;
//      }
//  }

//  switch (primitiveMode) {
//  case GL_POINTS:
//      basePrimitiveMode = GL_POINTS;
//      break;
//  case GL_LINES:
//  case GL_LINE_STRIP:
//  case GL_LINE_LOOP:
//  case GL_LINES_ADJACENCY_EXT:
//  case GL_LINE_STRIP_ADJACENCY_EXT:
//      basePrimitiveMode = GL_LINES;
//      break;
//  case GL_TRIANGLES:
//  case GL_QUADS:
//  case GL_QUAD_STRIP:
//  case GL_TRIANGLE_STRIP:
//  case GL_TRIANGLE_FAN:
//  case GL_POLYGON:
//  case GL_TRIANGLES_ADJACENCY_EXT:
//  case GL_TRIANGLE_STRIP_ADJACENCY_EXT:
//  case GL_QUAD_MESH_SUN:
//  case GL_TRIANGLE_MESH_SUN:
//      basePrimitiveMode = GL_TRIANGLES;
//      break;
//  default:
//      dbgPrint(DBGLVL_WARNING, "Unknown primitive mode\n");
//      return EC_DBG_INVALID_VALUE;
//  }

//  error = dbgCommandShaderStepVertex(addr, target, basePrimitiveMode,
//          forcePointPrimitiveMode, numFloatsPerVertex, numPrimitives,
//          numVertices, vertexData);
//  if (error) {
//      dbgCommandFreeMem(3, addr);
//      return error;
//  }

//  /* free memory on client side */
//  error = dbgCommandFreeMem(3, addr);
//  if (error) {
//      dbgPrint(DBGLVL_WARNING,
//              "getShaderCode: free memory on client side error: %i\n", error);
//  }
//  return error;
// }

// ErrorCode ProgramControl::callDone(void)
// {
//  ErrorCode error;

// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */

//  error = dbgCommandDone();
// #ifdef DEBUG
//  printCall();
// #endif
//  return error;
// }

// ErrorCode ProgramControl::execute(bool stopOnGLError)
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */
//  return dbgCommandExecute(stopOnGLError);
// }

// ErrorCode ProgramControl::executeToShaderSwitch(bool stopOnGLError)
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */
//  return dbgCommandExecuteToShaderSwitch(stopOnGLError);
// }

// ErrorCode ProgramControl::executeToDrawCall(bool stopOnGLError)
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */
//  return dbgCommandExecuteToDrawCall(stopOnGLError);
// }

// ErrorCode ProgramControl::executeToUserDefined(const char *fname,
//      bool stopOnGLError)
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */
//  return dbgCommandExecuteToUserDefined(fname, stopOnGLError);

// }

// ErrorCode ProgramControl::stop(void)
// {
// #ifdef _WIN32
//  ::SwitchToThread();
// #else /* _WIN32 */
//  sched_yield();
// #endif /* _WIN32 */
//  return dbgCommandStopExecution();
// }

// ErrorCode ProgramControl::checkExecuteState(int *state)
// {
//  debug_record_t *rec = getThreadRecord(_debuggeePID);
//  dbgPrint(DBGLVL_INFO, "execute state: %i\n", (int)rec->result);
//  switch (rec->result) {
//  case DBG_EXECUTE_IN_PROGRESS:
//      *state = 0;
//      return EC_NONE;
//  case DBG_FUNCTION_CALL:
//      *state = 1;
//      return EC_NONE;
//  case DBG_ERROR_CODE:
//      *state = 1;
//      return checkError();
//  default:
//      return EC_UNKNOWN_ERROR;
//  }
// }

// ErrorCode ProgramControl::executeContinueOnError(void)
// {
// #ifdef _WIN32
//  ::SwitchToThread();
//  ::SetEvent(_hEvtDebuggee);
// #else /* _WIN32 */
//  sched_yield();
//  ptrace(PTRACE_CONT, _debuggeePID, 0, 0);
// #endif /* _WIN32 */
//  return EC_NONE;
// }



// #ifdef _WIN32
// bool ProgramControl::attachToProgram(const PID_T pid) {
//  char dllPath[_MAX_PATH];// Path to debugger preload DLL.
//  char smName[_MAX_PATH];// Name of shared memory.

//  HMODULE hThis = GetModuleHandleW(NULL);
//  GetModuleFileNameA(hThis, dllPath, _MAX_PATH);
//  char *insPos = strrchr(dllPath, '\\');
//  if (insPos != NULL) {
//      strcpy(insPos + 1, DEBUGLIB);
//  } else {
//      strcpy(dllPath, DEBUGLIB);
//  }

//  this->createEvents(pid);
//  ::SetEvent(_hEvtDebuggee);

//  this->setDebugEnvVars();    // TODO dirty hack.
//  ::GetEnvironmentVariableA("GLSL_DEBUGGER_SHMID", smName, _MAX_PATH);
//  if (!::AttachToProcess(_ai, pid, PROCESS_ALL_ACCESS, dllPath, smName,
//                  _path_dbgfuncs)) {
//         error(EC_UNKNOWN_ERROR);
//         return false;
//  }

//  _hDebuggedProgram = _ai.hProcess;
//  _debuggeePID = pid;  // TODO: Do we need this information?

//  waitForChildStatus();
//     if(state() < ST_ALIVE)
//         return false;

//  return true;
// }
// #else /* _WIN32 */
// bool ProgramControl::attachToProgram(const PID_T pid)
// {
//  /* FIXME
//   * check if we're already attached to something?
//   * anyway: attaching is tricky. we need to replace the currently linked libGL
//   */
//     UT_NOTIFY(LV_INFO, "not yet implemented");
//     if(ptrace((__ptrace_request)PTRACE_ATTACH, pid, 0, 0) == -1) {
//      UT_NOTIFY(LV_ERROR, "attempt to attach to process(" << pid << ") failed");
//      return false;
//     }
//     waitForChildStatus();
//  if(state() != ST_STOPPED) {
//      UT_NOTIFY(LV_ERROR, "child should have stopped...");
//      _debuggeePID = 0;
//      state(ST_INVALID);
//      error(EC_UNKNOWN_ERROR);
//      return false;
//  }
//  return true;
// }
// #endif /* _WIN32 */

// bool ProgramControl::detachFromProgram(void)
// {
//  ErrorCode retval = EC_UNKNOWN_ERROR;

// #ifdef _WIN32
//  if (_ai.hProcess != NULL) {
//      // TODO: This won't work ... at least not properly.
//      this->execute(false);

//      if (::DetachFromProcess(_ai)) {
//          _hDebuggedProgram = NULL;
//          _debuggeePID = 0;
//      } else {
//          error(EC_EXIT);
//             return false;
//      }

//      //::SetEvent(_hEvtDebuggee);
//      //this->closeEvents();
//      this->stop();// Ensure consistent state.
//      waitForChildStatus();
//  }
//  /* Nothing to detach from. */
//  return true;
// #else /* _WIN32 */
//  // TODO
//     UT_NOTIFY(LV_INFO, "not yet implemented");
//  return false;
// #endif /* _WIN32 */
// }

// void Command::overwriteFuncArguments(FunctionCallPtr call)
// {
//  if (call->name() != _rec->fname) {
//      UT_NOTIFY(LV_ERROR, "function name does not match record");
//      exit(1);
//  }

//  int i = 0;
//  UT_NOTIFY(LV_INFO, "Printing arguments:");
//  for(auto &arg : call->arguments()) {
//      UT_NOTIFY_NO_PRFX("arg " << i << ") ");
//      print_dbg_type(arg->type(), arg->data());
//      UT_NOTIFY_NO_PRFX("\n");
//      copyArgumentToProcess(arg->address(), arg->data(), arg->type());
//      ++i;
//  }
// }

// void Command::queryResult()
// {
//  switch(_rec->result) {
//      case DBG_RET_ALLOCATED:
//      {
//      /* FIXME "numBlocks" is the parameter from the alloc command? */
//      for (unsigned int i = 0; i < numBlocks; ++i) {
//          addresses[i] = (void*) _rec->items[i];
//          dbgPrint(DBGLVL_INFO,
//                  "%i bytes of memory allocated at %p\n", sizes[i], addresses[i]);
//      }
//      break;
//      case DBG_RET_READBACK_RENDERBUFFER: {
//          void *buffer = (void*) rec->items[0];
//          *width = (int) rec->items[1];
//          *height = (int) rec->items[2];
//          /* TODO: check error */
//          *image = (float*) malloc(
//                  numComponents * (*width) * (*height) * sizeof(float));
//          cpyFromProcess(_debuggeePID, *image, buffer,
//                  numComponents * (*width) * (*height) * sizeof(float));
//          error = dbgCommandFreeMem(1, &buffer)
//      }
//      case DBG_ERROR_CODE:
//          handleError();
//      break;
//  }
// }
// Process::ErrorCode Command::handleError()
// {
//  ErrorCode r = EC_NONE;
//  switch (_rec->result == DBG_ERROR_CODE) {
//      switch ((unsigned int) _rec->items[0]) {
//      case GL_STACK_OVERFLOW:
//          r = EC_GL_STACK_OVERFLOW; break;
//      case GL_STACK_UNDERFLOW:
//          r = EC_GL_STACK_UNDERFLOW; break;
//      case GL_OUT_OF_MEMORY:
//          r = EC_GL_OUT_OF_MEMORY; break;
//      case GL_TABLE_TOO_LARGE:
//          r = EC_GL_TABLE_TOO_LARGE; break;
//      case GL_INVALID_FRAMEBUFFER_OPERATION_EXT:
//          r = EC_GL_INVALID_FRAMEBUFFER_OPERATION_EXT; break;
//      default:
//          UT_NOTIFY(LV_WARN, "unkown error code " << (unsigned int)_rec->items[0]);
//          r = EC_UNKNOWN_ERROR;
//      }
//      error(r);
//  }
//  return r;
// }
