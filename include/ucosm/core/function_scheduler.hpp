#pragma once


namespace ucosm {

    template<typename task_t>
    struct FreeFunctionTask final : task_t {

        using func_t = void(*)(task_t&);

        FreeFunctionTask() : mFunc(nullptr) {}

        FreeFunctionTask(func_t inFunc) : mFunc(inFunc) {}

        void setTask(func_t inFunc) {
            mFunc = inFunc;
        }

        void run() override {
            if (mFunc) {
                mFunc(*this);
            }
        }

    private:
        func_t mFunc;
    };

    template<typename obj_t, typename task_t>
    struct MemberFunctionTask final : task_t {

        using func_t = void(obj_t::*)(task_t&);

        MemberFunctionTask(obj_t& o) :
            mInst(o),
            mFunc(nullptr) {}

        MemberFunctionTask(obj_t& o, func_t inFunc) :
            mInst(o),
            mFunc(inFunc) {}

        void setTask(func_t inFunc) {
            mFunc = inFunc;
        }

        void run() override {
            if (mFunc) {
                (mInst.*mFunc)(*this);
            }
        }

    private:
        obj_t& mInst;
        func_t mFunc;
    };

}