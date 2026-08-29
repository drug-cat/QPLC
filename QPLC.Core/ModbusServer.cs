using System;
using System.Collections.Generic;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;

namespace QPLC.Core
{
    /// <summary>
    /// Modbus TCP server connecting SCADA/HMI to the QPLC simulator.
    /// Minimal implementation of the Modbus TCP protocol (FC 1/2/3/5/6/15/16) with no external dependencies.
    /// </summary>
    public class ModbusServer : IDisposable
    {
        private readonly LadderSimulator sim;
        private readonly Config config;
        private TcpListener? listener;
        private CancellationTokenSource? cts;
        private Task? acceptTask;
        private bool disposed;

        // Address -> index mapping
        private readonly Dictionary<string, int> boolAddr = new();
        private readonly Dictionary<string, int> intAddr = new();
        public int Port { get; }
        public int CoilCount => boolAddr.Count;
        public int HoldingRegisterCount => intAddr.Count;

        public ModbusServer(LadderSimulator sim, Config config, int port = 5020)
        {
            this.sim = sim;
            this.config = config;
            this.Port = port;
            BuildAddressMap();
        }

        private void BuildAddressMap()
        {
            int coil = 0, holding = 0;
            foreach (var kvp in config.Io)
            {
                var mapping = kvp.Value;
                if (mapping.Type == "BOOL")
                {
                    for (int i = 0; i < mapping.ArrayLength; i++)
                        boolAddr[$"{kvp.Key}[{i}]"] = coil++;
                }
                else if (mapping.Type == "INT")
                {
                    for (int i = 0; i < mapping.ArrayLength; i++)
                        intAddr[$"{kvp.Key}[{i}]"] = holding++;
                }
                else if (mapping.Type == "REAL")
                {
                    for (int i = 0; i < mapping.ArrayLength; i++)
                    {
                        intAddr[$"{kvp.Key}[{i}].lo"] = holding++;
                        intAddr[$"{kvp.Key}[{i}].hi"] = holding++;
                    }
                }
            }
        }

        public void Start()
        {
            if (listener != null) return;
            cts = new CancellationTokenSource();
            listener = new TcpListener(IPAddress.Loopback, Port);
            try { listener.Start(); }
            catch { listener = new TcpListener(IPAddress.Any, Port); listener.Start(); }
            acceptTask = Task.Run(() => AcceptLoop(cts.Token));
        }

        private async Task AcceptLoop(CancellationToken ct)
        {
            while (!ct.IsCancellationRequested && listener != null)
            {
                try
                {
                    var client = await listener.AcceptTcpClientAsync(ct);
                    _ = Task.Run(() => HandleClient(client, ct), ct);
                }
                catch (OperationCanceledException) { break; }
                catch (ObjectDisposedException) { break; }
            }
        }

        private async Task HandleClient(TcpClient client, CancellationToken ct)
        {
            using (client)
            {
                var stream = client.GetStream();
                var buf = new byte[1024];
                while (!ct.IsCancellationRequested && client.Connected)
                {
                    int n;
                    try { n = await stream.ReadAsync(buf, ct); }
                    catch { break; }
                    if (n < 8) break;
                    var resp = ProcessFrame(buf.AsSpan(0, n).ToArray());
                    if (resp != null)
                    {
                        try { await stream.WriteAsync(resp, ct); }
                        catch { break; }
                    }
                }
            }
        }

        private byte[]? ProcessFrame(byte[] frame)
        {
            if (frame.Length < 8) return null;
            ushort txId = (ushort)((frame[0] << 8) | frame[1]);
            ushort proto = (ushort)((frame[2] << 8) | frame[3]);
            if (proto != 0) return null;
            byte unit = frame[6];
            byte fc = frame[7];

            return fc switch
            {
                1 or 2 => ReadCoils(frame, txId, unit, fc),
                3 => ReadHoldingRegisters(frame, txId, unit),
                5 => WriteSingleCoil(frame, txId, unit),
                6 => WriteSingleRegister(frame, txId, unit),
                15 => WriteMultipleCoils(frame, txId, unit),
                16 => WriteMultipleRegisters(frame, txId, unit),
                _ => null
            };
        }

        private static byte[] Mbap(ushort txId, byte unit, byte[] pdu)
        {
            var r = new byte[7 + pdu.Length];
            r[0] = (byte)(txId >> 8);
            r[1] = (byte)(txId & 0xFF);
            r[2] = 0; r[3] = 0;
            r[4] = (byte)((pdu.Length + 1) >> 8);
            r[5] = (byte)((pdu.Length + 1) & 0xFF);
            r[6] = unit;
            Array.Copy(pdu, 0, r, 7, pdu.Length);
            return r;
        }

        private byte[] ReadCoils(byte[] f, ushort txId, byte unit, byte fc)
        {
            ushort addr = (ushort)((f[8] << 8) | f[9]);
            ushort qty = (ushort)((f[10] << 8) | f[11]);
            var coilVals = SnapshotCoils();
            int byteCount = (qty + 7) / 8;
            var pdu = new byte[2 + byteCount];
            pdu[0] = fc;
            pdu[1] = (byte)byteCount;
            for (int i = 0; i < qty; i++)
            {
                if ((addr + i) < coilVals.Length && coilVals[addr + i])
                    pdu[2 + i / 8] |= (byte)(1 << (i % 8));
            }
            return Mbap(txId, unit, pdu);
        }

        private byte[] ReadHoldingRegisters(byte[] f, ushort txId, byte unit)
        {
            ushort addr = (ushort)((f[8] << 8) | f[9]);
            ushort qty = (ushort)((f[10] << 8) | f[11]);
            var regs = SnapshotRegisters();
            var pdu = new byte[2 + qty * 2];
            pdu[0] = 3;
            pdu[1] = (byte)(qty * 2);
            for (int i = 0; i < qty; i++)
            {
                if ((addr + i) < regs.Length)
                {
                    pdu[2 + i * 2] = (byte)(regs[addr + i] >> 8);
                    pdu[3 + i * 2] = (byte)(regs[addr + i] & 0xFF);
                }
            }
            return Mbap(txId, unit, pdu);
        }

        private byte[] WriteSingleCoil(byte[] f, ushort txId, byte unit)
        {
            ushort addr = (ushort)((f[8] << 8) | f[9]);
            ushort val = (ushort)((f[10] << 8) | f[11]);
            SetCoilByAddr(addr, val != 0);
            return Mbap(txId, unit, new[] { f[7], f[8], f[9], f[10], f[11] });
        }

        private byte[] WriteSingleRegister(byte[] f, ushort txId, byte unit)
        {
            ushort addr = (ushort)((f[8] << 8) | f[9]);
            ushort val = (ushort)((f[10] << 8) | f[11]);
            SetRegisterByAddr(addr, val);
            return Mbap(txId, unit, new[] { f[7], f[8], f[9], f[10], f[11] });
        }

        private byte[] WriteMultipleCoils(byte[] f, ushort txId, byte unit)
        {
            ushort addr = (ushort)((f[8] << 8) | f[9]);
            ushort qty = (ushort)((f[10] << 8) | f[11]);
            for (int i = 0; i < qty; i++)
            {
                bool v = (f[13 + i / 8] & (1 << (i % 8))) != 0;
                SetCoilByAddr((ushort)(addr + i), v);
            }
            return Mbap(txId, unit, new[] { f[7], f[8], f[9], f[10], f[11] });
        }

        private byte[] WriteMultipleRegisters(byte[] f, ushort txId, byte unit)
        {
            ushort addr = (ushort)((f[8] << 8) | f[9]);
            ushort qty = (ushort)((f[10] << 8) | f[11]);
            for (int i = 0; i < qty; i++)
            {
                ushort v = (ushort)((f[13 + i * 2] << 8) | f[14 + i * 2]);
                SetRegisterByAddr((ushort)(addr + i), v);
            }
            return Mbap(txId, unit, new[] { f[7], f[8], f[9], f[10], f[11] });
        }

        private bool[] SnapshotCoils()
        {
            var arr = new bool[CoilCount];
            foreach (var kvp in boolAddr)
            {
                if (kvp.Value < arr.Length && sim.BoolVars.TryGetValue(kvp.Key, out var v))
                    arr[kvp.Value] = v;
            }
            return arr;
        }

        private ushort[] SnapshotRegisters()
        {
            var arr = new ushort[HoldingRegisterCount];
            foreach (var kvp in intAddr)
            {
                if (kvp.Value >= arr.Length) continue;
                string key = kvp.Key;
                if (sim.NumVars.TryGetValue(key, out var v))
                {
                    if (key.EndsWith(".hi") || key.EndsWith(".lo"))
                    {
                        var bytes = BitConverter.GetBytes((float)v);
                        ushort word = key.EndsWith(".hi")
                            ? (ushort)((bytes[2] << 8) | bytes[3])
                            : (ushort)((bytes[0] << 8) | bytes[1]);
                        arr[kvp.Value] = word;
                    }
                    else
                    {
                        arr[kvp.Value] = (ushort)((int)v & 0xFFFF);
                    }
                }
            }
            return arr;
        }

        private void SetCoilByAddr(int addr, bool v)
        {
            foreach (var kvp in boolAddr)
                if (kvp.Value == addr) { sim.SetBool(kvp.Key, v); return; }
        }

        private void SetRegisterByAddr(int addr, ushort v)
        {
            foreach (var kvp in intAddr)
                if (kvp.Value == addr) { sim.SetNumeric(kvp.Key, v); return; }
        }

        public void Stop()
        {
            cts?.Cancel();
            listener?.Stop();
            listener = null;
            cts?.Dispose();
            cts = null;
        }

        public void Dispose()
        {
            if (disposed) return;
            disposed = true;
            Stop();
        }
    }
}
