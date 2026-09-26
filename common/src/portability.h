#ifndef C751D214_288B_4768_8F18_4FA61E631139
#define C751D214_288B_4768_8F18_4FA61E631139

/**
 * Defines portable macros for compiler attributes
 */

//-------------------------------
// SECTION
//-------------------------------

#if defined(__GNUC__) || defined(__clang__)
#define SECTION(name) __attribute__((section(name)))
#define SECTION_SUPPORTED 1

#elif defined(_MSC_VER)
#define SECTION(name) __declspec(allocate(name))
#define SECTION_SUPPORTED 1

#else
#define SECTION_SUPPORTED 0
#endif

#if !SECTION_SUPPORTED
typedef char section_attribute_not_supported[-1];
#endif

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

//-------------------------------
// PACKED
//-------------------------------

#if defined(_MSC_VER)
#define PACKED_STRUCT_BEGIN __pragma(pack(push, 1))
#define PACKED_STRUCT_END __pragma(pack(pop))
#define PACKED
#elif defined(__GNUC__) || defined(__clang__)
#define PACKED_STRUCT_BEGIN
#define PACKED_STRUCT_END
#define PACKED __attribute__((packed))
#else
#error "Packed structs are not supported on this compiler"
#endif

//-------------------------------
// ARRAY SIZE
//-------------------------------

/* Compiler feature detection for static assertions */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
/* C11 standard static assert */
#define STATIC_ASSERT_EXPR(cond) \
  (sizeof(struct {               \
    _Static_assert(cond, "");    \
    int dummy;                   \
  }))
#elif defined(__GNUC__) || defined(__clang__)
/* GCC/Clang extension for expression-level static check */
#define STATIC_ASSERT_EXPR(cond) (sizeof(char[1 - 2 * !(cond)]))
#elif defined(_MSC_VER)
/* MSVC C99/C11 static check */
#define STATIC_ASSERT_EXPR(cond) (sizeof(char[1 - 2 * !(cond)]))
#else
/* Fallback: no static check */
#define STATIC_ASSERT_EXPR(cond) 0
#endif

/* Check if x is an array, not a pointer */
#if defined(__GNUC__) || defined(__clang__)
/*
 * Uses GCC/Clang __builtin_types_compatible_p:
 * 1. Checks that x is not a pointer.
 * 2. Checks that &x and x do NOT have the same type
 *    (for arrays, &arr is typeof(type(*)[N]), while arr decays to
 * typeof(type*)).
 */
#define IS_ARRAY(x)                                                     \
  (!__builtin_types_compatible_p(__typeof__(x), __typeof__(&(x)[0])) && \
   !__builtin_types_compatible_p(__typeof__(x), __typeof__(*(x))))
#else
/*
 * Fallback pointer check for other compilers:
 * Compare pointer types of &x and x. For pointers, typeof(&ptr) ==
 * typeof(ptr*), whereas for arrays, &arr is an array pointer
 * (int(*)[N]), not int**.
 */
#define IS_ARRAY(x) \
  (!__builtin_types_compatible_p(__typeof__(x), __typeof__(&(x)[0])))
#endif

/*
 * Main Macro:
 * Evaluates the static assert at compile time alongside sizeof
 * calculation.
 */
#define ARRAY_SIZE(arr) \
  ((void)STATIC_ASSERT_EXPR(IS_ARRAY(arr)), sizeof(arr) / sizeof((arr)[0]))

#endif /* C751D214_288B_4768_8F18_4FA61E631139 */
