/*Copyright (c) 2005-2006 Dodji Seketeli
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies
 * or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS",
 * WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE
 * WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE
 " AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "config.h"
#ifndef MPX_API_MACROS_HH
#define MPX_API_MACROS_HH

// FIXME temporary
#define HAS_GCC_VISIBILITY_SUPPORT 1

# ifdef HAS_GCC_VISIBILITY_SUPPORT
#  ifndef DSO_EXPORTED
#   define DSO_EXPORTED __attribute__ ((visibility("default")))
#  endif
#  ifndef DSO_NOT_EXPORTED
#   define DSO_NOT_EXPORTED __attribute__ ((visibility("hidden")))
#  endif
#  ifndef MPX_EXCEPTION_API
#   define MPX_EXCEPTION_API DSO_EXPORTED
#  endif
#  ifndef MPX_PURE_IFACE
#   define MPX_PURE_IFACE DSO_EXPORTED
#  endif
#  ifndef MPX_API
#   define MPX_API DSO_EXPORTED
#  endif //MPX_API DSO_EXPORTED
# else
#  define DSO_EXPORTED
#  define DSO_NOT_EXPORTED
#  define MPX_EXCEPTION_API
#  define MPX_PURE_IFACE
#  define MPX_API
# endif //HAS_GCC_VISIBILITY_SUPPORT
#endif

