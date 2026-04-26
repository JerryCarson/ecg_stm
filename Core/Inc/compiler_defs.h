#ifndef COMPILER_DEFS_H
#define COMPILER_DEFS_H

#ifdef __DOXYGEN__
    /* Для Doxygen заменяем специфичные атрибуты на стандартный inline */
    #define FORCE_INLINE static inline
#else
    /* Для компилятора (GCC/Clang) */
    #define FORCE_INLINE static inline __attribute__((always_inline))
#endif

#endif