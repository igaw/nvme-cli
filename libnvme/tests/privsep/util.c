// SPDX-License-Identifier: LGPL-2.1-or-later

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "util.h"

void fail(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	vfprintf(stderr, fmt, args);
	va_end(args);
	fputc('\n', stderr);
	abort();
}
