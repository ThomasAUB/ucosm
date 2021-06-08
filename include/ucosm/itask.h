#pragma once

struct ITask {
    virtual bool schedule() = 0;
};
