#include "misc_device.h"
#include "memory.h"
#include "kapi.h"
#include "trace.h"

MiscDevice::MiscDevice()
{
}

MiscDevice::MiscDevice(const AString& devName, Error& err)
{
    if (err != Error::Success)
    {
        return;
    }

    err = Create(devName);
}

MiscDevice::MiscDevice(const char* devName, Error& err)
{
    if (err != Error::Success)
    {
        return;
    }

    err = Create(devName);
}

Error MiscDevice::Create(const char* devName)
{
    Error err;

    AString devName_(devName, Memory::PoolType::Kernel, err);
    if (err != Error::Success)
    {
        return err;
    }
    return Create(devName_);
}

Error MiscDevice::Create(const AString& devName)
{
    Error err;
    err = get_kapi()->misc_dev_register(devName.GetBuf(), this, &MiscDevice::Ioctl, &MiscDevPtr);
    if (err != Error::Success)
    {
        trace(0, "Device %s register failed, err %d", devName.GetBuf(), err.GetCode());
        return err;
    }

    trace(1, "Device 0x%p dev 0x%p name %s", this, MiscDevPtr, devName.GetBuf());
    return err;
}

MiscDevice::~MiscDevice()
{
    trace(1, "Device 0x%p dev 0x%p destructor", this, MiscDevPtr);

    if (MiscDevPtr != nullptr)
    {
        get_kapi()->misc_dev_unregister(MiscDevPtr);
        MiscDevPtr = nullptr;
    }
}

long MiscDevice::Ioctl(void* context, unsigned int code, unsigned long arg)
{
    MiscDevice* device = static_cast<MiscDevice*>(context);

    Error err = device->Ioctl(code, arg);
    return err.GetCode();
}

Error MiscDevice::Ioctl(unsigned int code, unsigned long arg)
{
    return Error::NotImplemented;
}