#include "task.h"
#include "task_constants.h"
#include "../engine/resource_runtime.h"
#include <cstdlib>
#include <cstring>
#include <type_traits>
namespace megusta {
TasksManager gameTasks{};
int gameTaskCallbackDepth=0;
static_assert(std::is_trivially_copyable<Task>::value, "task slots are zeroed as a block");
void Task::initialize(GameTaskCallback function,std::uintptr_t argument,int mode) {
    callback=function; callbackArgument=argument; scheduleMode=mode; mask=0xffffffffu;
    if (mode>0) {
        for (unsigned i=0;i<sizeof(task_constants::modes)/sizeof(task_constants::modes[0]);++i) {
            if (mode==task_constants::modes[i]) { mask=task_constants::masks[i]; return; }
        }
        sharedIo->standardError->printf(task_constants::warningFormat,mode);
    }
}
void Task::setMask(std::uint32_t value) { scheduleMode=1; mask=value; }
void Task::start(TaskRuntime& runtime) {
    if (!(stateFlags&1u)) {
        stateFlags=1;
        Ball* ball=runtime.currentBall(runtime.context);
        if (runtime.scheduledTasks.currentTask) {
            contextObject=runtime.scheduledTasks.currentTask->contextObject;
            contextTag=runtime.scheduledTasks.currentTask->contextTag;
        } else {
            contextObject=ball;
            contextTag=ball ? runtime.lastTouch(runtime.context,ball) : 0;
        }
        runScheduled(runtime);
    }
}
void Task::stop() { stateFlags &= ~1u; }
void Task::invokeCallback(TaskRuntime& runtime) {
    ++gameTaskCallbackDepth;
    if (gameTaskCallbackDepth>20) {
        sharedIo->standardError->printf(task_constants::fatalFormat,"C:\\dev\\minigolf\\src\\game\\task.cxx",0x6d,"Invalid state");
        std::exit(1);
    }
    Task* previous=runtime.scheduledTasks.currentTask;
    runtime.scheduledTasks.currentTask=this;
    callback(callbackArgument);
    runtime.scheduledTasks.currentTask=previous;
    --gameTaskCallbackDepth;
}
void Task::runScheduled(TaskRuntime& runtime) {
    if (scheduleMode>=0 && (stateFlags&1u) && !(stateFlags&2u) &&
        (mask & runtime.scheduledTasks.frameMask)==runtime.scheduledTasks.frameMask) {
        invokeCallback(runtime);
        if (scheduleMode==0) stop();
    }
}
void Task::runIdle(TaskRuntime& runtime) {
    if (scheduleMode<0 && (stateFlags&1u) && !(stateFlags&2u)) invokeCallback(runtime);
}
TasksManager* TasksManager::initializeHead() { slots[0].callback=nullptr; return this; }
void TasksManager::clear(TaskRuntime& runtime) {
    for (int i=1;i<=taskCount;++i)
        if (slots[i].callback==runtime.pythonCallbackBridge && slots[i].callbackArgument)
            runtime.releasePythonCallback(slots[i].callbackArgument);
    std::memset(slots,0,sizeof(slots));
    taskCount=0; currentTask=nullptr; tickCount=0; frameMask=1;
}
void TasksManager::tickScheduled(TaskRuntime& runtime) {
    ++tickCount;
    for (int i=1;i<=taskCount;++i) slots[i].runScheduled(runtime);
    frameMask <<= 1;
    if (!(frameMask & 0x3fffffffu)) frameMask=1;
}
void TasksManager::runIdleCallbacks(TaskRuntime& runtime) {
    for (int i=1;i<=taskCount;++i) slots[i].runIdle(runtime);
}
void TasksManager::runDrawCallbacks(TaskRuntime& runtime) {
    runtime.prepareDraw(runtime.context);
    for (int i=1;i<=taskCount;++i) slots[i].runIdle(runtime);
}
Task* TasksManager::appendScheduled(GameTaskCallback callback,std::uintptr_t argument,int mode) {
    if (taskCount>79) std::exit(1);
    Task* task=&slots[++taskCount]; task->initialize(callback,argument,mode); return task;
}
Task* TasksManager::appendIdle(GameTaskCallback callback,std::uintptr_t argument,TaskRuntime& runtime) {
    Task* task=appendScheduled(callback,argument,-1); task->start(runtime); return task;
}
Task* TasksManager::appendDraw(GameTaskCallback callback,std::uintptr_t argument,TaskRuntime& runtime) {
    Task* task=appendScheduled(callback,argument,-1); task->start(runtime); return task;
}
}
