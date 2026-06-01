using System.ComponentModel;
using System.Text;
using System.Runtime.CompilerServices;
using Plugin.BLE;
using Plugin.BLE.Abstractions.Contracts;

namespace DCMotorControlApp.Services;

public class MotorBluetoothService : IMotorBluetoothService
{
    private readonly IBluetoothLE _ble;
    private readonly IAdapter _adapter;
    private IDevice _connectedDevice;
    private ICharacteristic _writeCharacteristic;

    private bool _isConnected;
    private bool _isScanning;

    public bool IsConnected
    {
        get => _isConnected;
        private set => SetProperty(ref _isConnected, value);
    }

    public bool IsScanning
    {
        get => _isScanning;
        private set => SetProperty(ref _isScanning, value);
    }

    public List<string> DiscoveredDevices { get; } = new();

    public event PropertyChangedEventHandler PropertyChanged;

    public MotorBluetoothService()
    {
        _ble = CrossBluetoothLE.Current;
        _adapter = CrossBluetoothLE.Current.Adapter;

        _adapter.DeviceDiscovered += (s, a) =>
        {
            if (!string.IsNullOrEmpty(a.Device.Name) && !DiscoveredDevices.Contains(a.Device.Name))
            {
                DiscoveredDevices.Add(a.Device.Name);
                OnPropertyChanged(nameof(DiscoveredDevices));
            }
        };
    }

    public async Task StartScanningAsync()
    {
        if (IsScanning) return;

        DiscoveredDevices.Clear();
        OnPropertyChanged(nameof(DiscoveredDevices));

        IsScanning = true;
        _adapter.ScanTimeout = 5; // set timeout in seconds (was assigning a TimeSpan)
        await _adapter.StartScanningForDevicesAsync();
        IsScanning = false;
    }

    public async Task<bool> ConnectToDeviceAsync(string deviceName)
    {
        try
        {
            // البحث عن الجهاز ضمن الأجهزة المكتشفة
            var device = _adapter.DiscoveredDevices.FirstOrDefault(d => d.Name == deviceName);
            if (device == null) return false;

            await _adapter.ConnectToDeviceAsync(device);
            _connectedDevice = device;

            // الحصول على الخدمات الافتراضية للبلوتوث السيريال (GATT Services)
            var services = await _connectedDevice.GetServicesAsync();

            // في الغالب الـ ESP32 تفتح خدمة مخصصة، سنأخذ الخدمة الأولى المتاحة للكتابة
            foreach (var service in services)
            {
                var characteristics = await service.GetCharacteristicsAsync();
                _writeCharacteristic = characteristics.FirstOrDefault(c => c.CanWrite);
                if (_writeCharacteristic != null) break;
            }

            IsConnected = _writeCharacteristic != null;
            return IsConnected;
        }
        catch
        {
            IsConnected = false;
            return false;
        }
    }

    public async Task DisconnectAsync()
    {
        if (_connectedDevice != null)
        {
            await _adapter.DisconnectDeviceAsync(_connectedDevice);
            IsConnected = false;
            _writeCharacteristic = null;
        }
    }

    public async Task SendCommandAsync(string command)
    {
        if (!IsConnected || _writeCharacteristic == null) return;

        try
        {
            // تحويل النص البرمجي إلى مصفوفة بايتات وإرسالها فوراً للـ ESP32
            byte[] bytes = Encoding.UTF8.GetBytes(command);
            await _writeCharacteristic.WriteAsync(bytes);
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"Error sending data: {ex.Message}");
        }
    }

    protected void SetProperty<T>(ref T storage, T value, [CallerMemberName] string propertyName = null)
    {
        if (Equals(storage, value)) return;
        storage = value;
        OnPropertyChanged(propertyName);
    }

    protected void OnPropertyChanged([CallerMemberName] string propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }

    public static class CrossBlueto
    {
        public static IBluetoothLE Current => CrossBluetoothLE.Current;
        public static IAdapter Adapter => CrossBluetoothLE.Current?.Adapter;
    }
}