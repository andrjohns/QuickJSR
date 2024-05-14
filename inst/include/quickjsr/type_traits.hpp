#ifndef QUICKJSR_TYPE_TRAITS_HPP
#define QUICKJSR_TYPE_TRAITS_HPP

#include <type_traits>
#include <vector>

namespace quickjsr {
  template <typename T>
  struct is_std_vector : std::false_type {};
  template <typename T>
  struct is_std_vector<std::vector<T>> : std::true_type {};

  template <typename T, typename = void>
  struct value_type {
    using type = typename std::decay_t<T>;
  };

  template <typename T>
  struct value_type<T, std::enable_if_t<is_std_vector<T>::value>> {
    using type = typename std::decay_t<T>::value_type;
  };

  template <typename T>
  using value_type_t = typename value_type<T>::type;

  template <typename GoalT, typename InT>
  using enable_if_type_t = std::enable_if_t<std::is_same<GoalT, InT>::value>;
} // namespace quickjsr
#endif
