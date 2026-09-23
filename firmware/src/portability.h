#ifndef C751D214_288B_4768_8F18_4FA61E631139
#define C751D214_288B_4768_8F18_4FA61E631139

/**
 * Defines portable macros for compiler attributes
 */

//-------------------------------
// NORETURN
//-------------------------------

#ifndef NORETURN
/* C23 standard */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 202311L
#define NORETURN [[noreturn]]
/* C11 standard */
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define NORETURN _Noreturn
/* GCC and Clang */
#elif defined(__GNUC__) || defined(__clang__)
#define NORETURN __attribute__((__noreturn__))
/* Microsoft Visual Studio */
#elif defined(_MSC_VER)
#define NORETURN __declspec(noreturn)
/* Fallback for strict C99 compilers without extensions */
#else
#define NORETURN
#endif
#endif

#endif /* C751D214_288B_4768_8F18_4FA61E631139 */
