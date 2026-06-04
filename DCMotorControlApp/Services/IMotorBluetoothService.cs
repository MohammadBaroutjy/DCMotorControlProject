using System.ComponentModel;
using System.Collections.Generic;

namespace DCMotorControlApp.Services
{
    public interface IMotorBluetoothService : INotifyPropertyChanged
    {
    bool IsConnected { get; }
    bool IsScanning { get; }
    bool IsConnecting { get; }
    string? ConnectingDeviceName { get; }
    List<string> DiscoveredDeviceNames { get; }

        Task StartScanningAsync();
        Task<bool> ConnectToDeviceAsync(string deviceName);
        Task DisconnectAsync();
        Task SendCommandAsync(string command);
    }
}

