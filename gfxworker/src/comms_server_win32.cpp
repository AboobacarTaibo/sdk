#include "mega/logging.h"
#include "mega/utils.h"
#include "gfxworker/comms_server_win32.h"
#include "gfxworker/server.h"
#include <system_error>
#include <windows.h>

namespace mega {
namespace gfx {

std::error_code WinGfxCommunicationsServer::OK;

Win32NamedPipeEndpointServer::~Win32NamedPipeEndpointServer()
{
    if (isValid())
    {
        LOG_verbose << mName << "Endpoint server flush";
        FlushFileBuffers(mPipeHandle);
        LOG_verbose << mName << "Endpoint server disconnect";
        DisconnectNamedPipe(mPipeHandle);
    }
}

bool WinGfxCommunicationsServer::initialize()
{
    serverListeningLoop();
    return false;
}

void WinGfxCommunicationsServer::shutdown()
{
    return;
}

std::error_code WinGfxCommunicationsServer::waitForClient(HANDLE hPipe, OVERLAPPED* overlap)
{
    assert(hPipe != INVALID_HANDLE_VALUE);
    assert(overlap);

    // Wait for the client to connect asynchronous; if it succeeds,
    // the function returns a nonzero value.
    // If the function returns zero,
    //         GetLastError returns ERROR_IO_PENDING, the IO is connected
    //         GetLastError returns ERROR_PIPE_CONNECTED, the IO is pending
    bool success = ConnectNamedPipe(hPipe, overlap);
    if (success)
    {
        LOG_verbose << "Client connected";
        return OK;
    }

    if (!success && GetLastError() == ERROR_PIPE_CONNECTED)
    {
        LOG_verbose << "Client connected";
        return OK;
    }

    if (!success && GetLastError() != ERROR_IO_PENDING)
    {
        LOG_verbose << "Client couldn't connect, error=" << GetLastError() << " " << mega::winErrorMessage(GetLastError());
        return std::make_error_code(std::errc::not_connected);
    }

    // IO_PENDING
    DWORD numberOfBytesTransferred = 0;
    if (GetOverlappedResultEx(
        hPipe,
        overlap,
        &numberOfBytesTransferred,
        mWaitMs, // INFINITE wait
        false))
    {
        LOG_verbose << "Client connected";
        return OK;
    }

    if (GetLastError() == WAIT_TIMEOUT)
    {
        LOG_verbose << "Wait client connecting Timeout";
        return std::make_error_code(std::errc::timed_out);
    }
    else
    {
        LOG_verbose << "Client couldn't connect, error=" << GetLastError() << " " << mega::winErrorMessage(GetLastError());
        return std::make_error_code(std::errc::not_connected);
    }
}

void WinGfxCommunicationsServer::serverListeningLoop()
{
    WinOverlap overlap;
    if (!overlap.isValid())
    {
        return;
    }

    // first instance to prevent two processes create the same pipe
    DWORD first_instance = FILE_FLAG_FIRST_PIPE_INSTANCE;
    const DWORD BUFSIZE = 512;
    for (;;)
    {
        LOG_verbose << "server awaiting client connection";

        std::string pipename = "\\\\.\\pipe\\" + mPipename;
        std::wstring wpipename = std::wstring(pipename.begin(), pipename.end());
        auto hPipe = CreateNamedPipe(
            wpipename.c_str(),             // pipe name
            PIPE_ACCESS_DUPLEX |      // read/write access
            FILE_FLAG_OVERLAPPED |    // overlapped
            first_instance,           // first instance or not
            PIPE_TYPE_MESSAGE |       // message type pipe
            PIPE_READMODE_BYTE |      // message-read mode
            PIPE_WAIT,                // blocking mode
            PIPE_UNLIMITED_INSTANCES, // max. instances
            BUFSIZE,                  // output buffer size
            BUFSIZE,                  // input buffer size
            0,                        // client time-out
            NULL);                    // default security attribute

        if (hPipe == INVALID_HANDLE_VALUE)
        {
            LOG_err << "CreateNamedPipe failed, Error=" << GetLastError() << " " << mega::winErrorMessage(GetLastError());
            break;
        }

        // not first instance for next iteration
        first_instance = 0;

        bool stopRunning = false;
        auto err_code = waitForClient(hPipe, overlap.data());
        if (err_code)
        {
            // if has timeout and expires, we'll stop running
            stopRunning = (mWaitMs != INFINITE && err_code == std::make_error_code(std::errc::timed_out));
            CloseHandle(hPipe);
        }
        else if (mRequestProcessor)
        {
            stopRunning = mRequestProcessor->process(mega::make_unique<Win32NamedPipeEndpointServer>(hPipe, "server"));
        }

        if (stopRunning)
        {
            LOG_info << "Exiting listening loop";
            break;
        }
    }
}

}
}
