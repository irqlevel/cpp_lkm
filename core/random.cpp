#include "random.h"
#include "astring.h"
#include "memory.h"
#include "trace.h"

Random::Random(Error& err, bool pseudoRandom)
{
    if (err != Error::Success)
    {
        return;
    }

    AString devName((pseudoRandom) ? "/dev/urandom" : "/dev/random", Memory::PoolType::Kernel, err);
    if (err != Error::Success)
    {
        trace(0, "Can't allocate string");
        return;
    }

    err = DevRandomFile.Open(devName, true, false);
    if (err != Error::Success)
    {
        trace(0, "Can't open dev random file %s, err %d", devName.GetBuf(), err.GetCode());
        return;
    }

    trace(1, "Random 0x%p contructed, dev %s", this, devName.GetBuf());
}

Error Random::GetBytes(void* buf, int len)
{
    Error err = DevRandomFile.Read(0, buf, len);
    if (err != Error::Success)
    {
        trace(0, "Can't read dev random file, err %d", err.GetCode());
    }
    return err;
}

unsigned long Random::GetUlong()
{
    unsigned long result;

    Error err = GetBytes(&result, sizeof(result));
    if (err != Error::Success)
    {
        return (unsigned long)-1;
    }

    return result;
}
Random::~Random()
{
    trace(1, "Random 0x%p desctructor", this);
}