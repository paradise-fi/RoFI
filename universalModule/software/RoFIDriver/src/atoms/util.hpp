#pragma once

#include <type_traits>

template < std::size_t... >
struct sum: std::integral_constant< std::size_t, 0 > {};

template <std::size_t X, std::size_t... Xs >
struct sum< X, Xs... > :
  std::integral_constant< std::size_t, X + sum< Xs... >::value > {};
