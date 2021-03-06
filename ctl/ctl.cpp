#include "ctl.h"
#include "memory.h"

#include <include/ctl.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

namespace KStor
{

namespace Control
{

Ctl::Ctl(int& err)
    : DevFd(-1)
{
    if (err)
    {
        return;
    }

    int fd = open("/dev/" KSTOR_CONTROL_DEVICE, 0);
    if (fd == -1)
    {
        err = errno;
        return;
    }
    DevFd = fd;
    return;
}


int Ctl::GetTime(unsigned long long& time)
{
    Cmd cmd;

    time = 0;
    memset(&cmd, 0, sizeof(cmd));
    int err = ioctl(DevFd, IOCTL_KSTOR_GET_TIME, &cmd);
    if (!err)
    {
        time = cmd.Union.GetTime.Time;
    }

    return err;
}

int Ctl::GetRandomUlong(unsigned long& value)
{
    Cmd cmd;

    value = -1;
    memset(&cmd, 0, sizeof(cmd));
    int err = ioctl(DevFd, IOCTL_KSTOR_GET_RANDOM_ULONG, &cmd);
    if (!err)
    {
        value = cmd.Union.GetRandomUlong.Value;
    }

    return err;
}

int Ctl::Mount(const char* deviceName, bool format, KStor::Api::Guid& volumeId)
{
    Cmd cmd;

    memset(&cmd, 0, sizeof(cmd));

    auto& params = cmd.Union.Mount;
    snprintf(params.DeviceName, ArraySize(params.DeviceName), "%s", deviceName);
    params.Format = format;
    int err = ioctl(DevFd, IOCTL_KSTOR_MOUNT, &cmd);
    if (!err)
    {
        volumeId = cmd.Union.Mount.VolumeId;
    }

    return err;
}

int Ctl::Unmount(const KStor::Api::Guid& volumeId)
{
    Cmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    auto& params = cmd.Union.Unmount;
    params.VolumeId = volumeId;

    return ioctl(DevFd, IOCTL_KSTOR_UNMOUNT, &cmd);
}

int Ctl::Unmount(const char* deviceName)
{
    Cmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    auto& params = cmd.Union.UnmountByName;
    snprintf(params.DeviceName, ArraySize(params.DeviceName),
        "%s", deviceName);

    return ioctl(DevFd, IOCTL_KSTOR_UNMOUNT_BY_NAME, &cmd);
}

int Ctl::StartServer(const char *host, unsigned short port)
{
    Cmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    auto& params = cmd.Union.StartServer;
    snprintf(params.Host, ArraySize(params.Host),
        "%s", host);
    params.Port = port;
    return ioctl(DevFd, IOCTL_KSTOR_START_SERVER, &cmd);
}

int Ctl::StopServer()
{
    Cmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    return ioctl(DevFd, IOCTL_KSTOR_STOP_SERVER, &cmd);
}

int Ctl::Test(unsigned int testId)
{
    Cmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    auto& params = cmd.Union.Test;
    params.TestId = testId;
    return ioctl(DevFd, IOCTL_KSTOR_TEST, &cmd);
}

int Ctl::GetTaskStack(int pid, char *buf, unsigned long len)
{
    Cmd cmd;

    memset(&cmd, 0, sizeof(cmd));
    auto& params = cmd.Union.GetTaskStack;
    params.Pid = pid;
    auto r = ioctl(DevFd, IOCTL_KSTOR_GET_TASK_STACK, &cmd);
    if (r)
        return r;

    snprintf(buf, len, "%s", params.Stack);
    return 0;
}

Ctl::~Ctl()
{
    if (DevFd >= 0)
    {
        close(DevFd);
        DevFd = -1;
    }
}

}

}