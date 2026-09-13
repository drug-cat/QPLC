using System.Runtime.InteropServices;

namespace QPLC.PlcSimAdapter;

/// <summary>
/// P/Invoke layer over the native C bridge (qplc_plcsim_bridge.dll).
/// The bridge keeps the COM-style PLCSIM Advanced Runtime API behind a
/// flat C surface, so .NET never touches vtable pointers directly.
/// </summary>
public static class PlcSimBridge
{
    public const int OK = 0;
    public const int ERR_INIT = -1;
    public const int ERR_NO_MANAGER = -2;
    public const int ERR_NO_INSTANCE = -3;
    public const int ERR_NOT_RUNNING = -4;
    public const int ERR_API = -5;

    [DllImport("qplc_plcsim_bridge.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int qplc_plcsim_init([MarshalAs(UnmanagedType.LPWStr)] string apiDllPath);

    [DllImport("qplc_plcsim_bridge.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern void qplc_plcsim_shutdown();

    [DllImport("qplc_plcsim_bridge.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int qplc_plcsim_connect_instance([MarshalAs(UnmanagedType.LPWStr)] string name);

    [DllImport("qplc_plcsim_bridge.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int qplc_plcsim_run(int timeoutMs);

    [DllImport("qplc_plcsim_bridge.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int qplc_plcsim_stop(int timeoutMs);

    [DllImport("qplc_plcsim_bridge.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int qplc_plcsim_is_running();

    [DllImport("qplc_plcsim_bridge.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int qplc_plcsim_read_bool([MarshalAs(UnmanagedType.LPWStr)] string tag, out int value);

    [DllImport("qplc_plcsim_bridge.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int qplc_plcsim_write_bool([MarshalAs(UnmanagedType.LPWStr)] string tag, int value);

    [DllImport("qplc_plcsim_bridge.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int qplc_plcsim_read_int16([MarshalAs(UnmanagedType.LPWStr)] string tag, out short value);

    [DllImport("qplc_plcsim_bridge.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int qplc_plcsim_write_int16([MarshalAs(UnmanagedType.LPWStr)] string tag, short value);

    [DllImport("qplc_plcsim_bridge.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int qplc_plcsim_read_real([MarshalAs(UnmanagedType.LPWStr)] string tag, out float value);

    [DllImport("qplc_plcsim_bridge.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int qplc_plcsim_write_real([MarshalAs(UnmanagedType.LPWStr)] string tag, float value);

    [DllImport("qplc_plcsim_bridge.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int qplc_plcsim_read_string([MarshalAs(UnmanagedType.LPWStr)] string tag, byte[] outBuf, int bufSize);

    [DllImport("qplc_plcsim_bridge.dll", CallingConvention = CallingConvention.Cdecl)]
    public static extern int qplc_plcsim_write_string([MarshalAs(UnmanagedType.LPWStr)] string tag, [MarshalAs(UnmanagedType.LPStr)] string value);

    public static string LastErrorToText(int code) => code switch
    {
        OK => "OK",
        ERR_INIT => "PLCSIM Advanced API initialization failed",
        ERR_NO_MANAGER => "No runtime manager (call init first)",
        ERR_NO_INSTANCE => "No connected instance",
        ERR_NOT_RUNNING => "Virtual PLC is not running",
        ERR_API => "PLCSIM Advanced API returned an error",
        _ => $"Unknown error code {code}",
    };
}