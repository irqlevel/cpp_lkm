#include "thread.h"

Thread::Thread(const RunnableRef routine, int& err)
   : Routine(nullptr), Task(nullptr), Stopping(false), CompEvent(err)
{
    if (err)
    {
        return;
    }
    Start(routine, err);
}

int Thread::StartRoutine(void* context)
{
    Thread* thread = static_cast<Thread*>(context);
    return thread->ExecuteRoutine();
}

int Thread::ExecuteRoutine()
{
    int err = Routine->Execute(*this);
    CompEvent.SetAll();
    return err;
}

void Thread::Start(const RunnableRef routine, int& err)
{
    if (err)
    {
        return;
    }
    if (!routine.get())
    {
        err = E_INVAL;
        return;
    }
    if (err)
    {
        return;
    }
    Routine = routine;
    Task = get_kapi()->task_create(&Thread::StartRoutine, this, "kcpp-thread");
    if (!Task)
    {
        err = E_NO_MEM;
        return;
    }
    get_kapi()->task_get(Task);
    get_kapi()->task_wakeup(Task);
    err = E_OK;
}

void Thread::Stop()
{
    Stopping = true;
    PRINTF("Set thread %p stopping\n", this, Stopping);
}

bool Thread::IsStopping() const
{
    PRINTF("Is thread %p stopping %d\n", this, Stopping);
    return Stopping;
}

void* Thread::GetId() const
{
    return Task;
}

void Thread::Wait()
{
    CompEvent.Wait();
}

void Thread::StopAndWait()
{
    Stop();
    Wait();
}

Thread::~Thread()
{
    if (Task)
    {
        StopAndWait();
        get_kapi()->task_put(Task);
    }
}

void Thread::Sleep(int milliseconds)
{
    get_kapi()->msleep(milliseconds);
}