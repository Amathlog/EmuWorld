#pragma once

#ifndef COEFF_SIZE_DOUBLE
#define COEFF_SIZE_DOUBLE 1
#endif

#if WIN32
#pragma warning(push)
#pragma warning(disable : 4244)
#include <filter_includes.h>
#pragma warning(pop)
#else
#include <filter_includes.h>
#endif