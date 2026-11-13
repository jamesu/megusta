/*
 * Compatibility subset for the legacy Microsoft largeint.h used by
 * CPython 1.5.2's Win32 timemodule. The Win32 SDK supplies LARGE_INTEGER and
 * the performance-counter APIs; this file supplies only the three arithmetic
 * helpers consumed by that source file.
 */
#ifndef MEGUSTA_PYTHON_LARGEINT_H
#define MEGUSTA_PYTHON_LARGEINT_H

#include <windows.h>

#define LargeIntegerEqualToZero(value) ((value).QuadPart == 0)
#define LargeIntegerSubtract(left, right) \
	megusta_large_integer_subtract((left), (right))
#define LargeIntegerDivide(dividend, divisor, remainder) \
	megusta_large_integer_divide((dividend), (divisor), (remainder))

static __inline LARGE_INTEGER
megusta_large_integer_subtract(LARGE_INTEGER left, LARGE_INTEGER right)
{
	LARGE_INTEGER result;
	result.QuadPart = left.QuadPart - right.QuadPart;
	return result;
}

static __inline LARGE_INTEGER
megusta_large_integer_divide(LARGE_INTEGER dividend, LARGE_INTEGER divisor,
			     LARGE_INTEGER *remainder)
{
	LARGE_INTEGER quotient;
	quotient.QuadPart = dividend.QuadPart / divisor.QuadPart;
	remainder->QuadPart = dividend.QuadPart % divisor.QuadPart;
	return quotient;
}

#endif /* MEGUSTA_PYTHON_LARGEINT_H */
