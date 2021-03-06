#include "ctl.h"

#include <stdio.h>
#include <string>

int main(int argc, char* argv[])
{
    int err = 0;
    KStor::Control::Ctl ctl(err);
    if (err)
    {
        printf("Ctl open err %d\n", err);
        return err;
    }

    if (argc < 2) {
        printf("Invalid number of args\n");
        return 1;
    }

    std::string cmd(argv[1]);
    if (cmd == "mount")
    {
        if (argc < 3 || argc > 4)
        {
            printf("Invalid number of args %d\n", argc);
            return 1;
        }

        std::string deviceName(argv[2]);
        bool format = false;
        if (argc == 4)
        {
            std::string param(argv[3]);
            if (param == "-f")
                format = true;
        }

        KStor::Api::Guid volumeId;
        err = ctl.Mount(deviceName.c_str(), format, volumeId);
        if (err)
        {
            printf("Ctl mount err %d\n", err);
            return err;
        }

        return 0;
    }
    else if (cmd == "umount")
    {
        if (argc != 3)
        {
            printf("Invalid number of args\n");
            return 1;
        }

        std::string deviceName(argv[2]);
        err = ctl.Unmount(deviceName.c_str());
        if (err)
        {
            printf("Ctl unmount err %d\n", err);
            return err;
        }

        return 0;
    }
    else if (cmd == "start-server")
    {
        if (argc != 4)
        {
            printf("Invalid number of args\n");
            return 1;
        }

        std::string host(argv[2]);
        int port = atoi(argv[3]);
        if (port <= 0 || port > 65535) {
            printf("Invalid port number\n");
            return 1;
        }

        err = ctl.StartServer(host.c_str(), port);
        if (err)
        {
            printf("Ctl start server err %d\n", err);
            return err;
        }

        return 0;
    }
    else if (cmd == "stop-server")
    {
        if (argc != 2)
        {
            printf("Invalid number of args\n");
            return 1;
        }

        err = ctl.StopServer();
        if (err)
        {
            printf("Ctl stop server err %d\n", err);
            return err;
        }

        return 0;
    }
    else if (cmd == "test")
    {
        if (argc != 3)
        {
            printf("Invalid number of args\n");
            return 1;
        }

        int testId = atoi(argv[2]);
        if (testId <= 0)
        {
            printf("Invalid test id\n");
            err = EINVAL;
            return err;
        }

        err = ctl.Test(testId);
        if (err)
        {
            printf("Ctl test err %d\n", err);
            return err;
        }

        return 0;   
    }
    else if (cmd == "task-stack")
    {
        if (argc != 3)
        {
            printf("Invalid number of args\n");
            return 1;
        }

        int pid = atoi(argv[2]);
        if (pid <= 0)
        {
            printf("Invalid test id\n");
            err = EINVAL;
            return err;
        }

        char stack[65536];
        err = ctl.GetTaskStack(pid, stack, sizeof(stack));
        if (err)
        {
            printf("Ctl task stack err %d\n", err);
            return err;
        }

        printf("%s\n", stack);

        return 0;
    }
    else
    {
        printf("Unknown cmd %s\n", cmd.c_str());
        return 1;
    }

    return 0;
}