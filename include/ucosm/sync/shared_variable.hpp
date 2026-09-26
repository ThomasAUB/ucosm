/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * MIT License                                                                     *
 *                                                                                 *
 * Copyright (c) 2026 Thomas AUBERT                                                *
 *                                                                                 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy    *
 * of this software and associated documentation files (the "Software"), to deal   *
 * in the Software without restriction, including without limitation the rights    *
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is           *
 * furnished to do so, subject to the following conditions:                        *
 *                                                                                 *
 * The above copyright notice and this permission notice shall be included in all  *
 * copies or substantial portions of the Software.                                 *
 *                                                                                 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR      *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,        *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE     *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER          *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,   *
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE   *
 * SOFTWARE.                                                                       *
 *                                                                                 *
 * github : https://github.com/ThomasAUB/ucosm                                     *
 *                                                                                 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */


#pragma once

#include <stdint.h>
#include <type_traits>
#include "uatom.hpp"
#include "memory_order.hpp"

namespace ucosm {

    /**
     * @brief Lock-free value shared between contexts, versioned on each store.
     * Single writer, any number of readers.
     *
     * @tparam T Value type (trivially copyable and lock-free on the platform)
     */
    template<typename T>
    class SharedVariable {
        static_assert(std::is_trivially_copyable<T>::value,
            "Type must be trivially copyable");
        static_assert(uatom::Atomic<T>::is_always_lock_free,
            "Type must be lock-free on this platform (use smaller types on MCU)");

    public:

        using version_t = uint32_t;

        /**
         * @brief Construct with initial value.
         * @param initialValue Initial value
         */
        explicit SharedVariable(const T& initialValue = T{}) noexcept :
            mValue(initialValue) {}

        /**
         * @brief Store a new value and bump the version. Writer side.
         * @param newValue New value to store
         */
        void store(const T& newValue) noexcept {
            const version_t version = mVersion.load(std::memory_order_relaxed);
            detail::storeRelease(mValue, newValue);
            detail::storeRelease(mVersion, static_cast<version_t>(version + 1));
        }

        /**
         * @brief Read the current value.
         * @return Current value
         */
        T load() const noexcept {
            return detail::loadAcquire(mValue);
        }

        /**
         * @brief Get current version number.
         * @return Version number (increments on each store)
         */
        version_t getVersion() const noexcept {
            return detail::loadAcquire(mVersion);
        }

        /**
         * @brief Check if value has been stored since a given version.
         * @param lastVersion Version previously returned by getVersion()
         * @return true if value has been updated
         */
        bool hasChanged(version_t lastVersion) const noexcept {
            return detail::loadAcquire(mVersion) != lastVersion;
        }

    private:
        uatom::Atomic<version_t> mVersion { 0 };
        uatom::Atomic<T> mValue;
    };

}
