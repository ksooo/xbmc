/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/log.h"

#include <cassert>
#include <type_traits>
#include <utility>

namespace KODI::UTILS
{
/* Base implementation taken from https://github.com/microsoft/GSL/blob/main/include/gsl/util */

// narrow_cast(): a searchable way to do narrowing casts of values
template<class T, class U>
constexpr T Narrow_cast(U&& u) noexcept
{
  return static_cast<T>(std::forward<U>(u));
}

/* Base implementation taken from https://github.com/microsoft/GSL/blob/main/include/gsl/narrow */

// narrow() : a checked version of narrow_cast() that throws if the cast changed the value
template<class T, class U, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
constexpr T Narrow(U u)
{
  constexpr const bool is_different_signedness =
      (std::is_signed<T>::value != std::is_signed<U>::value);

  const T t = Narrow_cast<T>(u);

#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
  if (Narrow_cast<U>(t) != u || (is_different_signedness && ((t < T{}) != (u < U{}))))
  {
    CLog::Log(LOGFATAL, "Type conversion error: Mismatch while narrowing data {} -> {}!", u, t);
    assert(false);
  }
#if defined(__clang__) || defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

  return t;
}
} // namespace KODI::UTILS
