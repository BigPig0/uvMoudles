/*
 * Copyright (c) 2014 Moritz Bitsch <moritzbitsch@gmail.com>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef CPTR_H
#define CPTR_H

#include <stddef.h>

#if ( defined _WIN32 )
#ifndef _WINDLL_FUNC
#ifdef THIRD_UTIL_EXPORT
#define _WINDLL_FUNC		_declspec(dllexport)
#else
#define _WINDLL_FUNC		extern
#endif
#endif
#elif ( defined __unix ) || ( defined __linux__ )
#ifndef _WINDLL_FUNC
#define _WINDLL_FUNC
#endif
#endif

#ifdef __cplusplus
extern "C"
{
#endif

_WINDLL_FUNC void* cptr_alloc(size_t size);
_WINDLL_FUNC void* cptr_retain(void* ptr, void* parent);
_WINDLL_FUNC void cptr_release(void* ptr);

#ifdef __cplusplus
}
#endif

#endif
