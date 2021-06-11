/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * MIT License                                                                     *
 *                                                                                 *
 * Copyright (c) 2020 Thomas AUBERT                                                *
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
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#pragma once

#include <tuple>
// provides a module container

template<class ...ModuleCollection>
class ModuleMix_M: public ModuleCollection... {

    using modules_t = std::tuple<ModuleCollection...>;

    template<size_t I>
    using getTypeAt = typename std::tuple_element<I, modules_t>::type;

    static constexpr size_t kModuleCount = std::tuple_size<modules_t>::value;

public:

    void init() {
        uint8_t d[] = { (uint8_t) 0, (ModuleCollection::init(), (uint8_t)0)... };
        static_cast<void>(d); // avoids warning dor unused variables
    }

    template<size_t I = 0>
    typename std::enable_if<I < kModuleCount, bool>::type isExeReady() {
        if (static_cast<getTypeAt<I>*>(this)->isExeReady()) {
            return isExeReady<I + 1>();
        }
        return false;
    }

    template<size_t I = 0>
    typename std::enable_if<I < kModuleCount, bool>::type isDelReady() {
        if (static_cast<getTypeAt<I>*>(this)->isDelReady()) {
            return isDelReady<I + 1>();
        }
        return false;
    }

    void makePreDel() {
        uint8_t d[] = { (uint8_t) 0,
                (ModuleCollection::makePreDel(), (uint8_t)0)... };
        static_cast<void>(d); // avoids warning dor unused variables
    }

    void makePreExe() {
        uint8_t d[] = { (uint8_t) 0,
                (ModuleCollection::makePreExe(), (uint8_t)0)... };
        static_cast<void>(d); // avoids warning dor unused variables
    }

    void makePostExe() {
        uint8_t d[] = { (uint8_t) 0,
                (ModuleCollection::makePostExe(), (uint8_t)0)... };
        static_cast<void>(d); // avoids warning dor unused variables
    }

private:

    template<size_t I = 0>
    typename std::enable_if<I == kModuleCount, bool>::type isExeReady() {
        return true;
    }

    template<size_t I = 0>
    typename std::enable_if<I == kModuleCount, bool>::type isDelReady() {
        return true;
    }

};

