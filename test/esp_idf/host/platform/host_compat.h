#pragma once
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdarg.h>
#ifndef __containerof
#define __containerof(ptr, type, member) ((type *)((char *)(ptr) - offsetof(type, member)))
#endif
