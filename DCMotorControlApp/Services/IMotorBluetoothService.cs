using System.ComponentModel;

namespace DCMotorControlApp.Services;
    public interface IMotorBluetoothService : INotifyPropertyChanged
    {
        bool IsConnected { get; }
        bool IsScanning { get; }
        List<string> DiscoveredDevices { get; }

        Task StartScanningAsync();
        Task<bool> ConnectToDeviceAsync(string deviceName);
        Task DisconnectAsync();
        Task SendCommandAsync(string command);
    }   

