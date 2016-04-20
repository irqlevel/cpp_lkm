#include "main.h"
#include "shared_ptr.h"
#include "list_entry.h"
#include "list.h"
#include "auto_lock.h"
#include "spinlock.h"
#include "worker.h"
#include "trace.h"
#include "vector.h"
#include "astring.h"
#include "kpatch.h"

#include "public.h"

struct kernel_api g_kapi;

struct kernel_api *get_kapi(void)
{
    return &g_kapi;
}

class TJob : public Runnable
{
public:
    TJob(int& err)
        : Runnable(err)
    {
        trace(1,"job %p ctor", this);
    }
    virtual ~TJob()
    {
        trace(1,"job %p dtor", this);
    }
    int Run(const Threadable& thread)
    {
        trace(1,"Hello from job %p", this);
        return E_OK;
    }
};

void test_worker()
{
    trace(1,"Test worker!!!");
    Worker w;

    int err = E_OK;
    WorkerRef worker(new (MemType::Atomic) Worker(err));
    if (!worker.get() || err)
        return;

    err = E_OK;
    RunnableRef job(new TJob(err));
    if (!job.get() || err)
        return;

    if (!worker->ExecuteAndWait(job, err))
        return;

    trace(1,"Waited job err %d", err);
}

void test_vector()
{
    Vector<char> v(MemType::Atomic);

    v.PushBack('a');
    v.PushBack('b');

    trace(1, "v[0]=%c v[1]=%c", v[0], v[1]);
}

void test_astring()
{
    int err = E_OK;

    AString s("blabla", MemType::Atomic, err);
    if (err)
        return;

    trace(1, "s init err %d", err);
    trace(1, "s content=%s len=%lu", s.GetBuf(), s.GetLen());
}

int test_kpatch()
{
    int err = E_OK;
    AString symbol("_do_fork", MemType::Kernel, err);
    if (err)
        return err;

    KPatch kp(err);
    if (err)
        return err;

    Vector<unsigned long> callers(MemType::Kernel);

    err = kp.GetCallers(symbol, callers);
    if (err)
        return err;

    for (size_t i = 0; i < callers.GetSize(); i++)
    {
        trace(1, "caller 0x%lx", callers[i]);
    }

    return E_OK;
}

int cpp_init(struct kernel_api *kapi)
{
    g_kapi = *kapi;
    trace(1,"cpp_init");

    test_worker();
    test_vector();
    test_astring();
    test_kpatch();

    trace(1, "cpp_init completed");
    return 0;
}

void cpp_deinit(void)
{
    trace(1,"cpp_deinit");
}
