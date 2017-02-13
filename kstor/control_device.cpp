#include "control_device.h"
#include "volume.h"

#include <core/trace.h>
#include <core/copy_user.h>
#include <core/time.h>
#include <core/unique_ptr.h>
#include <core/trace.h>
#include <core/auto_lock.h>
#include <core/shared_auto_lock.h>

#include <include/ctl.h>

namespace KStor 
{

ControlDevice* ControlDevice::Device = nullptr;

ControlDevice::ControlDevice(Core::Error& err)
    : MiscDevice(KSTOR_CONTROL_DEVICE, err)
    , Rng(err)
{
}

Core::Error ControlDevice::Mount(const Core::AString& deviceName, bool format, uint64_t blockSize, Guid& volumeId)
{
    Core::AutoLock lock(VolumeLock);
    if (VolumeRef.Get() != nullptr)
    {
        return Core::Error::AlreadyExists;
    }

    Core::Error err;
    VolumeRef = Core::MakeShared<Volume, Core::Memory::PoolType::Kernel>(deviceName, err);
    if (VolumeRef.Get() == nullptr)
    {
        trace(0, "CtrlDev 0x%p can't allocate device", this);
        err.SetNoMemory();
        return err;
    }

    if (!err.Ok())
    {
        trace(0, "CtrlDev 0x%p device init err %d", this, err.GetCode());
        return err;
    }

    if (format)
    {
        err = VolumeRef->Format(blockSize);
        if (!err.Ok())
        {
            trace(0, "CtrlDev 0x%p device format err %d", this, err.GetCode());
            return err;
        }
    }

    err = VolumeRef->Load();
    if (!err.Ok())
    {
        trace(0, "CtrlDev 0x%p device load err %d", this, err.GetCode());
        return err;
    }

    volumeId = VolumeRef->GetVolumeId();
    return Core::Error::Success;
}

Core::Error ControlDevice::Unmount(const Guid& volumeId)
{
    Core::AutoLock lock(VolumeLock);
    if (VolumeRef.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    if (VolumeRef->GetVolumeId() == volumeId)
    {
        VolumeRef.Reset();
        return Core::Error::Success;
    }

    return Core::Error::NotFound;
}

Core::Error ControlDevice::Unmount(const Core::AString& deviceName)
{
    Core::AutoLock lock(VolumeLock);
    if (VolumeRef.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    if (VolumeRef->GetDeviceName().Compare(deviceName) == 0)
    {
        VolumeRef.Reset();
        return Core::Error::Success;
    }

    return Core::Error::NotFound;
}

Core::Error ControlDevice::StartServer(const Core::AString& host, unsigned short port)
{
    return Srv.Start(host, port);
}

Core::Error ControlDevice::StopServer()
{
    Srv.Stop();
    return Core::Error::Success;
}

Core::Error ControlDevice::Ioctl(unsigned int code, unsigned long arg)
{
    trace(3, "Ioctl 0x%x arg 0x%lx", code, arg);

    Core::Error err;
    Core::UniquePtr<Control::Cmd> cmd(new (Core::Memory::PoolType::Kernel) Control::Cmd);
    if (cmd.Get() == nullptr)
    {
        trace(0, "Can't allocate memory");
        goto cleanup;
    }

    err = Core::CopyFromUser(cmd.Get(), reinterpret_cast<Control::Cmd*>(arg));
    if (!err.Ok())
    {
        trace(0, "Can't copy cmd from user");
        goto cleanup;
    }

    switch (code)
    {
    case IOCTL_KSTOR_GET_TIME:
        cmd->Union.GetTime.Time = Core::Time::GetTime();
        break;
    case IOCTL_KSTOR_GET_RANDOM_ULONG:
        cmd->Union.GetRandomUlong.Value = Rng.GetUlong();
        break;
    case IOCTL_KSTOR_MOUNT:
    {
        auto& params = cmd->Union.Mount;
        if (params.DeviceName[Core::Memory::ArraySize(params.DeviceName) - 1] != '\0')
        {
            err.SetInvalidValue();
            break;
        }

        Core::AString deviceName(params.DeviceName, Core::Memory::ArraySize(params.DeviceName) - 1, err);
        if (!err.Ok())
        {
            break;
        }

        Guid volumeId;
        err = Mount(deviceName, params.Format, params.BlockSize, volumeId);
        if (err.Ok()) {
            params.VolumeId = volumeId.GetContent();
        }
        break;
    }
    case IOCTL_KSTOR_UNMOUNT:
        err = Unmount(cmd->Union.Unmount.VolumeId);
        break;
    case IOCTL_KSTOR_UNMOUNT_BY_NAME:
    {
        auto& params = cmd->Union.UnmountByName;
        if (params.DeviceName[Core::Memory::ArraySize(params.DeviceName) - 1] != '\0')
        {
            err.SetInvalidValue();
            break;
        }

        Core::AString deviceName(params.DeviceName, Core::Memory::ArraySize(params.DeviceName) - 1, err);
        if (!err.Ok())
        {
            break;
        }

        err = Unmount(deviceName);
        break;
    }
    case IOCTL_KSTOR_START_SERVER:
    {
        auto& params = cmd->Union.StartServer;
        if (params.Host[Core::Memory::ArraySize(params.Host) - 1] != '\0')
        {
            err.SetInvalidValue();
            break;
        }

        Core::AString host(params.Host, Core::Memory::ArraySize(params.Host) - 1, err);
        if (!err.Ok())
        {
            break;
        }

        err = StartServer(host, params.Port);
        break;
    }
    case IOCTL_KSTOR_STOP_SERVER:
        err = StopServer();
        break;
    default:
        trace(0, "Unknown ioctl 0x%x", code);
        err = Core::Error::UnknownCode;
        break;
    }

    if (!err.Ok())
    {
        goto cleanup;
    }

    err = Core::CopyToUser(reinterpret_cast<Control::Cmd*>(arg), cmd.Get());
    if (!err.Ok())
    {
        trace(0, "Can't copy cmd to user");
        goto cleanup;
    }

cleanup:

    trace(1, "Ioctl 0x%x result %d", code, err.GetCode());
    return err;
}

ControlDevice::~ControlDevice()
{
}

Core::Error ControlDevice::ChunkCreate(const Guid& chunkId)
{
    Core::SharedAutoLock lock(VolumeLock);
    if (VolumeRef.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    return VolumeRef->ChunkCreate(chunkId);
}

Core::Error ControlDevice::ChunkWrite(const Guid& chunkId, unsigned char data[Api::ChunkSize])
{
    Core::SharedAutoLock lock(VolumeLock);
    if (VolumeRef.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    return VolumeRef->ChunkWrite(chunkId, data);
}

Core::Error ControlDevice::ChunkRead(const Guid& chunkId, unsigned char data[Api::ChunkSize])
{
    Core::SharedAutoLock lock(VolumeLock);
    if (VolumeRef.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    return VolumeRef->ChunkRead(chunkId, data);
}

Core::Error ControlDevice::ChunkDelete(const Guid& chunkId)
{
    Core::SharedAutoLock lock(VolumeLock);
    if (VolumeRef.Get() == nullptr)
    {
        return Core::Error::NotFound;
    }

    return VolumeRef->ChunkDelete(chunkId);
}

ControlDevice* ControlDevice::Get()
{
    return Device;
}

Core::Error ControlDevice::Create()
{
    Core::Error err;

    if (Device != nullptr)
        return Core::Error::InvalidState;

    Device = new (Core::Memory::PoolType::Kernel) ControlDevice(err);
    if (Device == nullptr)
    {
        err.SetNoMemory();
        return err;
    }

    if (!err.Ok())
    {
        delete Device;
        Device = nullptr;
        return err;
    }

    return err;
}

void ControlDevice::Delete()
{
    if (Device != nullptr)
    {
        delete Device;
        Device = nullptr;
    }
}

}