#include "block_device.h"
#include "trace.h"

BlockDevice::BlockDevice(const AString& deviceName, Error& err)
    : BDevPtr(nullptr)
    , Mode(KAPI_BDEV_MODE_READ|KAPI_BDEV_MODE_WRITE|KAPI_BDEV_MODE_EXCLUSIVE)
{
    if (err != Error::Success)
    {
        return;
    }

    int rc = get_kapi()->bdev_get_by_path(deviceName.GetBuf(),
        Mode, this, &BDevPtr);
    if (rc != 0)
    {
        trace(0, "Can't get bdev %s, err %d", deviceName.GetBuf(), rc);
        err = Error(rc);
        return;
    }

    trace(1, "Bdev 0x%p bdev 0x%p constructed", this, BDevPtr);
}

void* BlockDevice::GetBdev()
{
    return BDevPtr;
}

BlockDevice::~BlockDevice()
{
    trace(1, "Bdev 0x%p bdev 0x%p destructor", this, BDevPtr);

    if (BDevPtr != nullptr)
    {
        get_kapi()->bdev_put(BDevPtr, Mode);
        BDevPtr = nullptr;
    }
}