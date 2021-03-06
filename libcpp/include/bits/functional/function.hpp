//
// Created by Martin Miralles-Cordal on 12/24/2019.
//

#pragma once

#include <type_traits>
#include <utility>

namespace std {

inline namespace thor_os {

//=======================================================================
// Copyright Baptiste Wicht 2013-2018.
// Distributed under the terms of the MIT License.
// (See accompanying file LICENSE or copy at
//  http://www.opensource.org/licenses/MIT)
//=======================================================================

template<typename> struct function;

template<typename R, typename... Args>
struct function<R(Args...)> {
  public:
    template<typename T>
    function(T&& t) {
        using impl = model<typename decay<T>::type>;

        static_assert(sizeof(impl) <= sizeof(storage), "Limited size in function");
        static_assert(is_trivially_destructible<T>::value, "Limited support of function");

        auto* s = static_cast<void*>(&storage[0]);
        new (s) impl(forward<T>(t));
    }

    function(const function& rhs) = delete;
    function& operator=(const function& rhs) = delete;

    function(function&& rhs) = delete;
    function& operator=(function&& rhs) = delete;

    R operator()(Args... args) const {
        auto* s = static_cast<const void*>(&storage[0]);
        auto* c = static_cast<const concept_t*>(s);
        return (*c)(forward<Args>(args)...);
    }

  private:
    size_t storage[2];

    struct concept_t {
        virtual R operator()(Args...) const = 0;
    };

    template<typename T>
    struct model : concept_t {
        template<typename U> model(U&& u) : t(forward<U>(u)) {}

        R operator()(Args... args) const override {
            return t(forward<Args>(args)...);
        }

        T t;
    };
};

} // end namespace thor_os
} // namespace std