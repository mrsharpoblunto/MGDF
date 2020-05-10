#include "StdAfx.h"

#include "win32Exception.hpp"

#include <eh.h>

#if defined(_DEBUG)
#define new new (_NORMAL_BLOCK, __FILE__, __LINE__)
#pragma warning(disable : 4291)
#endif

namespace MGDF {
namespace core {

std::string Win32Exception::TranslateError(unsigned code) {
  switch (code) {
    case EXCEPTION_ACCESS_VIOLATION:
      return "Access violation";
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
      return "The thread tried to access an array element that is out of "
             "bounds and the underlying hardware supports bounds checking.";
    case EXCEPTION_BREAKPOINT:
      return "A breakpoINT32 was encountered.";
    case EXCEPTION_DATATYPE_MISALIGNMENT:
      return "The thread tried to read or write data that is misaligned on "
             "hardware that does not provide alignment.\r\nFor example, 16-bit "
             "values must be aligned on 2-byte boundaries; 32-bit values on "
             "4-byte boundaries, and so on.";
    case EXCEPTION_FLT_DENORMAL_OPERAND:
      return "One of the operands in a floating-poINT32 operation is "
             "denormal.\r\nA denormal value is one that is too small to "
             "represent as a standard floating-poINT32 value.";
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
      return "The thread tried to divide a floating-poINT32 value by a "
             "floating-poINT32 divisor of zero.";
    case EXCEPTION_FLT_INEXACT_RESULT:
      return "The result of a floating-poINT32 operation cannot be represented "
             "exactly as a decimal fraction.";
    case EXCEPTION_FLT_INVALID_OPERATION:
      return "This exception represents any floating-poINT32 exception not "
             "included in this list.";
    case EXCEPTION_FLT_OVERFLOW:
      return "The exponent of a floating-poINT32 operation is greater than the "
             "magnitude allowed by the corresponding type.";
    case EXCEPTION_FLT_STACK_CHECK:
      return "The stack overflowed or underflowed as the result of a "
             "floating-poINT32 operation.";
    case EXCEPTION_FLT_UNDERFLOW:
      return "The exponent of a floating-poINT32 operation is less than the "
             "magnitude allowed by the corresponding type.";
    case EXCEPTION_ILLEGAL_INSTRUCTION:
      return "The thread tried to execute an invalid instruction.";
    case EXCEPTION_IN_PAGE_ERROR:
      return "The thread tried to access a page that was not present, and the  "
             "was unable to load the page.\r\nFor example, this exception "
             "might occur if a network connection is lost while running a "
             "program over the network.";
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
      return "The thread tried to divide an integer value by an integer "
             "divisor of zero.";
    case EXCEPTION_INT_OVERFLOW:
      return "The result of an integer operation caused a carry out of the "
             "most significant bit of the result.";
    case EXCEPTION_INVALID_DISPOSITION:
      return "An exception handler returned an invalid disposition to the "
             "exception dispatcher.\r\nProgrammers using a high-level language "
             "such as C should never encounter this exception.";
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
      return "The thread tried to continue execution after a noncontinuable "
             "exception occurred.";
    case EXCEPTION_PRIV_INSTRUCTION:
      return "The thread tried to execute an instruction whose operation is "
             "not allowed in the current machine mode.";
    case EXCEPTION_SINGLE_STEP:
      return "A trace trap or other single-instruction mechanism signaled that "
             "one instruction has been executed.";
    case EXCEPTION_STACK_OVERFLOW:
      return "The thread used up its stack.";
  }
  return "Unknown exception";
}

}  // namespace core
}  // namespace MGDF
