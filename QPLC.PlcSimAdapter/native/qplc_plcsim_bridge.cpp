// QPLC ↔ PLCSIM Advanced bridge implementation.
// Compiles against the official SimulationRuntimeApi.h shipped with
// PLCSIM Advanced (API/<ver>/SimulationRuntimeApi.h). The API DLL is a
// COM-style vtable interface; we keep it behind this C++ file so .NET
// only ever talks to the flat C functions above.
#include "qplc_plcsim_bridge.h"

#include <windows.h>
#include <wchar.h>
#include <string>

// Path to the Siemens header; resolved at build time by CMake.
#if defined(QPLC_PLCSIM_API_HEADER)
#  include QPLC_PLCSIM_API_HEADER
#else
#  error "QPLC_PLCSIM_API_HEADER must point at SimulationRuntimeApi.h"
#endif

// The header defines inline helpers that lazily LoadLibrary the runtime
// API DLL (via registry path) once InitializeApi is called. We simply use
// those helpers: InitializeApi → CreateInterface → ReadBool/WriteBool/...

namespace {

ISimulationRuntimeManager* gManager = nullptr;
IInstance* gInstance = nullptr;

int ToResult(ERuntimeErrorCode rc) {
    return rc == SREC_OK ? QPLC_PLSIM_OK
         : rc == SREC_INSTANCE_NOT_RUNNING ? QPLC_PLSIM_ERR_NOT_RUNNING
         : QPLC_PLSIM_ERR_API;
}

} // namespace

extern "C" {

int qplc_plcsim_init(const wchar_t* apiDllPath) {
    if (gManager != nullptr) return QPLC_PLSIM_OK;  // already initialized

    ERuntimeErrorCode rc =
        (apiDllPath && *apiDllPath)
            ? InitializeApi(const_cast<wchar_t*>(apiDllPath), &gManager)
            : InitializeApi(&gManager);

    return (rc == SREC_OK) ? QPLC_PLSIM_OK : QPLC_PLSIM_ERR_INIT;
}

void qplc_plcsim_shutdown(void) {
    if (gInstance) { DestroyInterface(gInstance); gInstance = nullptr; }
    if (gManager)  { DestroyInterface(gManager);  gManager = nullptr; }
}

int qplc_plcsim_connect_instance(const wchar_t* name) {
    if (!gManager) return QPLC_PLSIM_ERR_NO_MANAGER;
    if (gInstance) { DestroyInterface(gInstance); gInstance = nullptr; }

    wchar_t fixed[DINSTANCE_NAME_MAX_LENGTH + 1] = L"";
    wcsncpy_s(fixed, name, _TRUNCATE);

    ERuntimeErrorCode rc = gManager->CreateInterface(fixed, &gInstance);
    return (rc == SREC_OK) ? QPLC_PLSIM_OK : QPLC_PLSIM_ERR_API;
}

int qplc_plcsim_run(int timeoutMs) {
    if (!gInstance) return QPLC_PLSIM_ERR_NO_INSTANCE;
    ERuntimeErrorCode rc = (timeoutMs > 0) ? gInstance->Run((UINT32)timeoutMs)
                                           : gInstance->Run();
    return ToResult(rc);
}

int qplc_plcsim_stop(int timeoutMs) {
    if (!gInstance) return QPLC_PLSIM_ERR_NO_INSTANCE;
    ERuntimeErrorCode rc = (timeoutMs > 0) ? gInstance->Stop((UINT32)timeoutMs)
                                           : gInstance->Stop();
    return ToResult(rc);
}

int qplc_plcsim_is_running(void) {
    if (!gInstance) return 0;
    return gInstance->GetOperatingState() == EOperatingState::SROS_RUN ? 1 : 0;
}

int qplc_plcsim_read_bool(const wchar_t* tag, int* value) {
    if (!gInstance || !tag || !value) return QPLC_PLSIM_ERR_NO_INSTANCE;
    bool out = false;
    ERuntimeErrorCode rc = gInstance->ReadBool(const_cast<wchar_t*>(tag), &out);
    if (rc == SREC_OK) *value = out ? 1 : 0;
    return ToResult(rc);
}

int qplc_plcsim_write_bool(const wchar_t* tag, int value) {
    if (!gInstance || !tag) return QPLC_PLSIM_ERR_NO_INSTANCE;
    return ToResult(gInstance->WriteBool(const_cast<wchar_t*>(tag), value != 0));
}

int qplc_plcsim_read_int16(const wchar_t* tag, short* value) {
    if (!gInstance || !tag || !value) return QPLC_PLSIM_ERR_NO_INSTANCE;
    INT16 out = 0;
    ERuntimeErrorCode rc = gInstance->ReadInt16(const_cast<wchar_t*>(tag), &out);
    if (rc == SREC_OK) *value = (short)out;
    return ToResult(rc);
}

int qplc_plcsim_write_int16(const wchar_t* tag, short value) {
    if (!gInstance || !tag) return QPLC_PLSIM_ERR_NO_INSTANCE;
    return ToResult(gInstance->WriteInt16(const_cast<wchar_t*>(tag), (INT16)value));
}

int qplc_plcsim_read_real(const wchar_t* tag, float* value) {
    if (!gInstance || !tag || !value) return QPLC_PLSIM_ERR_NO_INSTANCE;
    float out = 0.0f;
    ERuntimeErrorCode rc = gInstance->ReadFloat(const_cast<wchar_t*>(tag), &out);
    if (rc == SREC_OK) *value = out;
    return ToResult(rc);
}

int qplc_plcsim_write_real(const wchar_t* tag, float value) {
    if (!gInstance || !tag) return QPLC_PLSIM_ERR_NO_INSTANCE;
    return ToResult(gInstance->WriteFloat(const_cast<wchar_t*>(tag), value));
}

int qplc_plcsim_read_string(const wchar_t* tag, char* outBuf, int bufSize) {
    if (!gInstance || !tag || !outBuf || bufSize <= 0) return QPLC_PLSIM_ERR_NO_INSTANCE;
    int charsRead = 0;
    ERuntimeErrorCode rc = gInstance->ReadString(const_cast<wchar_t*>(tag), outBuf, bufSize, &charsRead);
    return ToResult(rc);
}

int qplc_plcsim_write_string(const wchar_t* tag, const char* value) {
    if (!gInstance || !tag || !value) return QPLC_PLSIM_ERR_NO_INSTANCE;
    return ToResult(gInstance->WriteString(tag, value, static_cast<int>(strlen(value))));
}

} // extern "C"