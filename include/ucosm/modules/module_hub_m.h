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
class ModuleHub_M {

    using modules_t = std::tuple<ModuleCollection...>;

    static constexpr size_t kModuleCount = std::tuple_size<modules_t>::value;

    modules_t mModules;

public:

    template<typename module_t>
    module_t& getModule() {
        return std::get<module_t>(mModules);
    }

    template<size_t I>
    typename std::tuple_element<I, modules_t>::type& getModule() {
        return std::get<I>(mModules);
    }

    //////////////////// init

    template<size_t I = 0>
    typename std::enable_if<I < kModuleCount, void>::type init() {
        std::get<I>(mModules).init();
        init<I + 1>();
    }

    //////////////////// exe readdy

    template<size_t I = 0>
    typename std::enable_if<I < kModuleCount, bool>::type isExeReady() {
        if (std::get<I>(mModules).isExeReady()) {
            return isExeReady<I + 1>();
        }
        return false;
    }

    //////////////////// del ready

    template<size_t I = 0>
    typename std::enable_if<I < kModuleCount, bool>::type isDelReady() {
        if (std::get<I>(mModules).isDelReady()) {
            return isDelReady<I + 1>();
        }
        return false;
    }

    //////////////////// pre del

    template<size_t I = 0>
    typename std::enable_if<I < kModuleCount, void>::type makePreDel() {
        std::get<I>(mModules).makePreDel();
        makePreDel<I + 1>();
    }

    //////////////////// pre exe

    template<size_t I = 0>
    typename std::enable_if<I < kModuleCount, void>::type makePreExe() {
        std::get<I>(mModules).makePreExe();
        makePreExe<I + 1>();
    }

    //////////////////// post exe

    template<size_t I = 0>
    typename std::enable_if<I < kModuleCount, void>::type makePostExe() {
        std::get<I>(mModules).makePostExe();
        makePostExe<I + 1>();
    }

    ////////////////////

private:

    template<size_t I = 0>
    typename std::enable_if<I == kModuleCount, void>::type init() {
    }

    template<size_t I = 0>
    typename std::enable_if<I == kModuleCount, bool>::type isExeReady() {
        return true;
    }

    template<size_t I = 0>
    typename std::enable_if<I == kModuleCount, bool>::type isDelReady() {
        return true;
    }

    template<size_t I = 0>
    typename std::enable_if<I == kModuleCount, void>::type makePreDel() {
    }

    template<size_t I = 0>
    typename std::enable_if<I == kModuleCount, void>::type makePreExe() {
    }

    template<size_t I = 0>
    typename std::enable_if<I == kModuleCount, void>::type makePostExe() {
    }

};

