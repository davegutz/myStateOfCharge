#ifndef __PRINT64_H
#define __PRINT64_H

// Library for creating strings for 64-bit integers in binary, octal, decimal, or hex. 
// Unsigned (uint64_t) can be converted to any case. 
// Signed (int64_t) can be converted to decimal only.
//
// Github: https://github.com/rickkas7/Print64
// License: MIT (can be used in #ifndef __PRINT64_H
#define __PRINT64_H

// Library for creating strings for 64-bit integers in binary, octal, decimal, or hex. 
// Unsigned (uint64_t) can be converted to any case. 
// Signed (int64_t) can be converted to decimal only.
//
// Github: https://github.com/rickkas7/Print64
// License: MIT (can be used in closed-source commercial products)

#include "Particle.h"

/**
 * @brief Convert an unsigned 64-bit integer to a string
 * 
 * @param value The value to convert
 * 
 * @param base The number base. Acceptable values are 2, 8, 10, and 16. Default is 10 (decimal).
 * 
 * @return A String object containing an ASCII representation of the value.
 */
String toString(uint64_t value, unsigned char base = 10);

/**
 * @brief Convert an signed 64-bit integer to a string (ASCII decimal signed integer)
 * 
 * @param value The value to convert
 * 
 * @return A String object containing an ASCII representation of the value (decimal)
 */
String toString(int64_t value);


#endif /* __PRINT64_H */
