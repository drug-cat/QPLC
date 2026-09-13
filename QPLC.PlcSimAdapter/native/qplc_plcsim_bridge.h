// QPLC ↔ PLCSIM Advanced bridge — flat C API (safe to P/Invoke from .NET)
#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Status codes (mirror the API error codes we care about)
#define QPLC_PLSIM_OK 0
#define QPLC_PLSIM_ERR_INIT -1
#define QPLC_PLSIM_ERR_NO_MANAGER -2
#define QPLC_PLSIM_ERR_NO_INSTANCE -3
#define QPLC_PLSIM_ERR_NOT_RUNNING -4
#define QPLC_PLSIM_ERR_API -5

// Lifecycle ----------------------------------------------------------
int qplc_plcsim_init(const wchar_t* apiDllPath);       // 0 = ok
void qplc_plcsim_shutdown(void);
int qplc_plcsim_connect_instance(const wchar_t* name); // 0 = ok
int qplc_plcsim_run(int timeoutMs);
int qplc_plcsim_stop(int timeoutMs);
int qplc_plcsim_is_running(void);                      // 1 = running

// Tag IO ------------------------------------------------------------
int qplc_plcsim_read_bool(const wchar_t* tag, int* value);
int qplc_plcsim_write_bool(const wchar_t* tag, int value);
int qplc_plcsim_read_int16(const wchar_t* tag, short* value);
int qplc_plcsim_write_int16(const wchar_t* tag, short value);
int qplc_plcsim_read_real(const wchar_t* tag, float* value);
int qplc_plcsim_write_real(const wchar_t* tag, float value);
int qplc_plcsim_read_string(const wchar_t* tag, char* outBuf, int bufSize);
int qplc_plcsim_write_string(const wchar_t* tag, const char* value);

#ifdef __cplusplus
}
#endif
