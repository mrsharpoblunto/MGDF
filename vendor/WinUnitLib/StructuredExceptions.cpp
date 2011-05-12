/* -------------------------------------------------------------------------- *
   WinUnit - Maria Blees (maria.blees@microsoft.com)
 * -------------------------------------------------------------------------- */
///  @file StructuredExceptions.cpp

#include "StructuredExceptions.h"

using namespace WinUnitLib;

/// This function returns a string associated with a particular structured
/// exception code.
const wchar_t* StructuredExceptions::GetDescription(
    DWORD exceptionCode,   ///< A structured exception code (see 
                           ///< GetExceptionCode() documentation for more information)
    DWORD type /* 0 */)    ///< Extra type information (only applicable to access violations)
{
    switch(exceptionCode)
    {
        case EXCEPTION_ACCESS_VIOLATION:
            // The thread tried to read from or write to a virtual address for 
            // which it does not have the appropriate access.
            switch(type)
            {
            case 0: // read
                return L"Access violation (read).";
            case 1: // write
                return L"Access violation (write).";
            default:
                return L"Access violation.";
            }
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
            // The thread tried to access an array element that is out of bounds and 
            // the underlying hardware supports bounds checking.
            return L"Array bounds exceeded.";
        case EXCEPTION_BREAKPOINT:
            // A breakpoint was encountered.
            return L"A breakpoint was encountered.";
        case EXCEPTION_DATATYPE_MISALIGNMENT:
            // The thread tried to read or write data that is misaligned on 
            // hardware that does not provide alignment. For example, 16-bit values 
            // must be aligned on 2-byte boundaries; 32-bit values on 4-byte 
            // boundaries, and so on.
            return L"Data type misalignment.";
        case EXCEPTION_FLT_DENORMAL_OPERAND:
            // One of the operands in a floating-point operation is denormal. A 
            // denormal value is one that is too small to represent as a standard 
            // floating-point value.
            return L"Floating-point exception: denormal operand.";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:
            // The thread tried to divide a floating-point value by a floating-point 
            // divisor of zero.
            return L"Floating-point exception: divide-by-zero.";
        case EXCEPTION_FLT_INEXACT_RESULT:
            // The result of a floating-point operation cannot be represented 
            // exactly as a decimal fraction.
            return L"Floating-point exception: inexact result.";
        case EXCEPTION_FLT_INVALID_OPERATION:
            // This exception represents any floating-point exception not included 
            // in this list.
            return L"Floating-point exception.";
        case EXCEPTION_FLT_OVERFLOW:
            // The exponent of a floating-point operation is greater than the 
            // magnitude allowed by the corresponding type.
            return L"Floating-point exception: arithmetic overflow.";
        case EXCEPTION_FLT_STACK_CHECK:
            // The stack overflowed or underflowed as the result of a 
            // floating-point operation.
            return L"Floating-point exception: stack check.";
        case EXCEPTION_FLT_UNDERFLOW:
            // The exponent of a floating-point operation is less than the 
            // magnitude allowed by the corresponding type.
            return L"Floating-point exception: arithmetic underflow.";
        case EXCEPTION_ILLEGAL_INSTRUCTION:
            // The thread tried to execute an invalid instruction.
            return L"Illegal instruction.";
        case EXCEPTION_IN_PAGE_ERROR:
            // The thread tried to access a page that was not present, and the 
            // system was unable to load the page. For example, this exception 
            // might occur if a network connection is lost while running a program 
            // over the network.
            return L"Could not load page.";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:
            // The thread tried to divide an integer value by an integer divisor 
            // of zero.
            return L"Integer divide-by-zero.";
        case EXCEPTION_INT_OVERFLOW:
            // The result of an integer operation caused a carry out of the most 
            // significant bit of the result.
            return L"Integer overflow.";
        case EXCEPTION_INVALID_DISPOSITION:
            // An exception handler returned an invalid disposition to the 
            // exception dispatcher.  Programmers using a high-level language
            // such as C should never encounter this exception.
            return L"An exception handler returned an invalid disposition to the"
                   L" exception dispatcher.";
        case EXCEPTION_NONCONTINUABLE_EXCEPTION:
            // The thread tried to continue execution after a noncontinuable 
            // exception occurred.
            return L"The thread tried to continue execution after a "
                   L"noncontinuable exception occurred.";
        case EXCEPTION_PRIV_INSTRUCTION:
            // The thread tried to execute an instruction whose operation is not 
            // allowed in the current machine mode.
            return L"Attempt to execute privileged instruction.";
        case EXCEPTION_SINGLE_STEP:
            // A trace trap or other single-instruction mechanism signaled that
            // one instruction has been executed.
            return L"Single step exception.";
        case EXCEPTION_STACK_OVERFLOW:
            // The thread used up its stack.
            return L"Stack overflow.";
        default:
            return L"Unexpected exception.";
    }
}


/// This function decides based on the exception information passed in if we
/// want to handle the exception or just let it bubble up and cause the app to
/// "crash".
/// @return Value indicating whether we should handle it or not.
int StructuredExceptions::Filter(LPEXCEPTION_POINTERS pExceptionInformation,
                                           EXCEPTION_RECORD* pExceptionRecordStorage)
{
    if (pExceptionInformation == NULL) { return EXCEPTION_CONTINUE_SEARCH; }

    PEXCEPTION_RECORD pExceptionRecord = pExceptionInformation->ExceptionRecord;

    if (pExceptionRecord == NULL) 
    { 
        return EXCEPTION_CONTINUE_SEARCH; 
    }

    // Copy out the exception record so it can be used in the __except block
    if (pExceptionRecordStorage)
    {
        *pExceptionRecordStorage = *pExceptionRecord;
    }

    if (pExceptionRecord->ExceptionFlags & EXCEPTION_NONCONTINUABLE)
    {
        return EXCEPTION_CONTINUE_SEARCH;
    }

    bool wantToHandleException = false;

    switch(pExceptionRecord->ExceptionCode)
    {
    case EXCEPTION_ACCESS_VIOLATION:
        // The thread tried to read from or write to a virtual address for 
        // which it does not have the appropriate access.
        if (pExceptionRecord->ExceptionInformation[0] == 0) // attempted read
        {
            wantToHandleException = true;
        }
        // Otherwise, it was an attempted write.
        break;
    case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
        // The thread tried to access an array element that is out of bounds and 
        // the underlying hardware supports bounds checking.
        break;
    case EXCEPTION_BREAKPOINT:
        // A breakpoint was encountered.
        break;
    case EXCEPTION_DATATYPE_MISALIGNMENT:
        // The thread tried to read or write data that is misaligned on 
        // hardware that does not provide alignment. For example, 16-bit values 
        // must be aligned on 2-byte boundaries; 32-bit values on 4-byte 
        // boundaries, and so on.
        break;
    case EXCEPTION_FLT_DENORMAL_OPERAND:
        // One of the operands in a floating-point operation is denormal. A 
        // denormal value is one that is too small to represent as a standard 
        // floating-point value.
        wantToHandleException = true;
        break;
    case EXCEPTION_FLT_DIVIDE_BY_ZERO:
        // The thread tried to divide a floating-point value by a floating-point 
        // divisor of zero.
        wantToHandleException = true;
        break;
    case EXCEPTION_FLT_INEXACT_RESULT:
        // The result of a floating-point operation cannot be represented 
        // exactly as a decimal fraction.
        wantToHandleException = true;
        break;
    case EXCEPTION_FLT_INVALID_OPERATION:
        // This exception represents any floating-point exception not included 
        // in this list.
        wantToHandleException = true;
        break;
    case EXCEPTION_FLT_OVERFLOW:
        // The exponent of a floating-point operation is greater than the 
        // magnitude allowed by the corresponding type.
        wantToHandleException = true;
        break;
    case EXCEPTION_FLT_STACK_CHECK:
        // The stack overflowed or underflowed as the result of a 
        // floating-point operation.
        break;
    case EXCEPTION_FLT_UNDERFLOW:
        // The exponent of a floating-point operation is less than the 
        // magnitude allowed by the corresponding type.
        wantToHandleException = true;
        break;
    case EXCEPTION_ILLEGAL_INSTRUCTION:
        // The thread tried to execute an invalid instruction.
        break;
    case EXCEPTION_IN_PAGE_ERROR:
        // The thread tried to access a page that was not present, and the 
        // system was unable to load the page. For example, this exception 
        // might occur if a network connection is lost while running a program 
        // over the network.
        break;
    case EXCEPTION_INT_DIVIDE_BY_ZERO:
        // The thread tried to divide an integer value by an integer divisor 
        // of zero.
        wantToHandleException = true;
        break;
    case EXCEPTION_INT_OVERFLOW:
        // The result of an integer operation caused a carry out of the most 
        // significant bit of the result.
        wantToHandleException = true;
        break;
    case EXCEPTION_INVALID_DISPOSITION:
        // An exception handler returned an invalid disposition to the 
        // exception dispatcher.  Programmers using a high-level language
        // such as C should never encounter this exception.
        break;
    case EXCEPTION_NONCONTINUABLE_EXCEPTION:
        // The thread tried to continue execution after a noncontinuable 
        // exception occurred.
        break;
    case EXCEPTION_PRIV_INSTRUCTION:
        // The thread tried to execute an instruction whose operation is not 
        // allowed in the current machine mode.
        break;
    case EXCEPTION_SINGLE_STEP:
        // A trace trap or other single-instruction mechanism signaled that
        // one instruction has been executed.
        break;
    case EXCEPTION_STACK_OVERFLOW:
        // The thread used up its stack.
        break;
    }
    return (wantToHandleException ? EXCEPTION_EXECUTE_HANDLER : 
                                    EXCEPTION_CONTINUE_SEARCH);
}


