#pragma once

#include <util/TypeTraits.hpp>
#include <Object.hpp>


class Nothing{};

template <typename T, typename = IsValue<T>>
class Maybe : public Object
{
  public:
    Maybe() = default;
    Maybe(Nothing) : Maybe() {}

    template <typename U = T, typename = IsCopyConstructible<U>>
    Maybe(T const &v) : _value(v), _set(true) {}

    template <typename U = T, typename = IsMoveConstructible<U>>
    Maybe(T &&v) : _value(std::move(v)), _set(true) {}

    /** Returns whether this contains a value. */
    constexpr operator bool() const noexcept { return _set; }

    /** Returns whether this contains a value. */
    constexpr bool isPresent() const noexcept { return _set; }

    /**
     * Accesses the contained value.
     * @return A const pointer to the value if set, nullptr otherwise.
     */
    constexpr T const * operator->() const { return _set ? &_value : nullptr; }

    /**
     * Accesses the contained value.
     * @return A pointer to the value if set, nullptr otherwise.
     */
    constexpr T * operator->() { return _set ? &_value : nullptr; }

    /**
     * Accesses the contained value.
     * @return A const ref to the value. Return value is undefined if not set.
     */
    constexpr T const & operator*() const { return _value; }

    /**
     * Accesses the contained value.
     * @return A reference to the value. Return value is undefined if not set.
     */
    constexpr T & operator*() { return _value; }

    /**
     * Returns the contained value.
     * @return A const ref to the value. Return value is undefined if not set.
     */
    constexpr T const & get() const { return _value; }

    /**
     * Returns the contained value.
     * @return A reference to the value. Return value is undefined if not set.
     */
    constexpr T & get() { return _value; }

    /**
     * Returns a copy of the contained value, or a default value if not set.
     * @param other The value to default to.
     * @return A copy of the value, or the default value if this is not set.
     */
    constexpr T getOr(T &&other) const &
    {
        return _set ? _value : std::forward<T>(other);
    }

    /**
     * Moves out the contained value or returns a default value if not set.
     * @param other The value to default to.
     * @return A copy of the value, or the default value if this is not set.
     */
    constexpr T getOr(T &&other) &&
    {
        return _set ? std::move(_value) : std::forward<T>(other);
    }

  private:
    T _value{};
    bool _set{false};
};

/**
 * Constructs and "promotes" an object to a Maybe<T>.
 * @tparam T The type of Maybe.
 * @tparam Args The types for one of T's constructors. (deducible)
 * @param args The arguments for one of T's constructors.
 * @return The Maybe.
 */
template <typename T, typename... Args>
static Maybe<T> AsMaybe(Args && ...args)
{
    return Maybe<T>(T(std::forward<Args>(args)...));
}
