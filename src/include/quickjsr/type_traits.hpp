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
    using type = typename std::decay<T>::type;
  };

  template <typename T>
  struct value_type<T, typename std::enable_if<is_std_vector<T>::value>::type> {
    using type = typename std::decay<T>::type::value_type;
  };

  template <typename T>
  using value_type_t = typename value_type<T>::type;

  template <typename GoalT, typename InT>
  using enable_if_type_t = typename std::enable_if<std::is_same<GoalT, InT>::value>::type;
} // namespace quickjsr
#endif
