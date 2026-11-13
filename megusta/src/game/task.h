// SPDX-License-Identifier: GPL-2.0

#pragma once
#include <cstdint>
namespace megusta {
class Ball;
struct TaskRuntime;
using GameTaskCallback = void (*)(std::uintptr_t argument);
// One normalized task definition shared with the ball contact-event queue.
class Task {
public:
    GameTaskCallback callback;
    std::uintptr_t callbackArgument;
    int scheduleMode;
    std::uint32_t stateFlags;
    std::uint32_t mask;
    void* contextObject;
    std::uintptr_t contextTag;
    void initialize(GameTaskCallback function, std::uintptr_t argument, int mode);
    void setMask(std::uint32_t value);
    void start(TaskRuntime& runtime);
    void stop();
    void invokeCallback(TaskRuntime& runtime);
    void runScheduled(TaskRuntime& runtime);
    void runIdle(TaskRuntime& runtime);
};
class TasksManager {
public:
    Task slots[81];
    int taskCount;
    std::uint32_t tickCount;
    Task* currentTask;
    std::uint32_t frameMask;
    TasksManager* initializeHead();
    void clear(TaskRuntime& runtime);
    void tickScheduled(TaskRuntime& runtime);
    void runIdleCallbacks(TaskRuntime& runtime);
    void runDrawCallbacks(TaskRuntime& runtime);
    Task* appendScheduled(GameTaskCallback callback, std::uintptr_t argument, int mode);
    Task* appendIdle(GameTaskCallback callback, std::uintptr_t argument, TaskRuntime& runtime);
    Task* appendDraw(GameTaskCallback callback, std::uintptr_t argument, TaskRuntime& runtime);
};
struct TaskRuntime {
    void* context;
    TasksManager& scheduledTasks;
    Ball* (*currentBall)(void*);
    std::uintptr_t (*lastTouch)(void*, Ball*);
    void (*prepareDraw)(void*);
    GameTaskCallback pythonCallbackBridge;
    void (*releasePythonCallback)(std::uintptr_t);
};
extern TasksManager gameTasks;
extern int gameTaskCallbackDepth;
void invokePythonTaskCallback(std::uintptr_t callable);
void releasePythonTaskCallback(std::uintptr_t callable);
}
