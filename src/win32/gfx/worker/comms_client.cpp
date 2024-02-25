#include "mega/logging.h"
#include "mega/win32/gfx/worker/comms_client.h"
#include "mega/gfx/worker/comms.h"

namespace mega {
namespace gfx {

CommError WinGfxCommunicationsClient::do_connect(LPCTSTR pipeName, HANDLE &hPipe)
{
    CommError error = CommError::ERR;
    hPipe = INVALID_HANDLE_VALUE;
    while (1)
    {
        hPipe = CreateFile(
            pipeName,       // pipe name
            GENERIC_READ |  // read and write access
            GENERIC_WRITE,
            0,              // no sharing
            NULL,           // default security attributes
            OPEN_EXISTING,  // opens existing pipe
            FILE_FLAG_OVERLAPPED,   // flag and attributes
            NULL);          // no template file

        // Break if the pipe handle is valid.
        if (hPipe != INVALID_HANDLE_VALUE)
        {
            error = CommError::OK;
            break;
        }

        // Exit if an error other than ERROR_PIPE_BUSY occurs.
        auto lastError = GetLastError();
        if (lastError != ERROR_PIPE_BUSY)
        {
            LOG_err << "Could not open pipe. Error Code=" << lastError << " " << mega::winErrorMessage(lastError);
            error = toCommError(lastError);
            break;
        }

        // All pipe instances are busy, so wait for 10 seconds.
        if (!WaitNamedPipe(pipeName, 10000))
        {
            LOG_warn << "Could not open pipe: 10 second wait timed out.";
            error = CommError::TIMEOUT;
            break;
        }
    }

    LOG_verbose << "Connected Handle:" << hPipe << "error: " << static_cast<BYTE>(error);

    return error;
}

CommError WinGfxCommunicationsClient::connect(std::unique_ptr<IEndpoint>& endpoint)
{
    std::string pipename = "\\\\.\\pipe\\" + mPipename;
    std::wstring wpipename = std::wstring(pipename.begin(), pipename.end());

    HANDLE hPipe = INVALID_HANDLE_VALUE;
    CommError error  = do_connect(wpipename.c_str(), hPipe);
    endpoint = hPipe == INVALID_HANDLE_VALUE ? nullptr : mega::make_unique<Win32NamedPipeEndpointClient>(hPipe, "client");
    return error;
}

CommError WinGfxCommunicationsClient::toCommError(DWORD winError) const
{
    switch (winError) {
    case ERROR_SUCCESS:
        return CommError::OK;
    case ERROR_FILE_NOT_FOUND:
        return CommError::NOT_EXIST;
    default:
        return CommError::ERR;
    }
}

}
}