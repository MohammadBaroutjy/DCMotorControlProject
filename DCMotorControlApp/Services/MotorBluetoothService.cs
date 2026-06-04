using System.ComponentModel;
using System.Text;
using System.Runtime.CompilerServices;
using Plugin.BLE;
using Plugin.BLE.Abstractions.Contracts;

namespace DCMotorControlApp.Services;

public class MotorBluetoothService : IMotorBluetoothService
{
    private ICharacteristic _txCharacteristic;
    private IService _gattService;

    // Prefer centralized GUIDs so we can reuse them for filtered scanning and lookups
    private readonly Guid _serviceUuid = Guid.Parse("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
    private readonly Guid _characteristicUuid = Guid.Parse("beb5483e-36e1-4688-b7f5-ea07361b26a8");

    private readonly IBluetoothLE _ble;
    private readonly IAdapter _adapter;
    private IDevice _connectedDevice;
    private ICharacteristic _writeCharacteristic;

    private bool _isConnected;
    private bool _isScanning;
    private bool _isConnecting;
    private string? _connectingDeviceName;

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

    public bool IsConnecting
    {
        get => _isConnecting;
        private set => SetProperty(ref _isConnecting, value);
    }

    public string? ConnectingDeviceName
    {
        get => _connectingDeviceName;
        private set => SetProperty(ref _connectingDeviceName, value);
    }

    // Store both device names and IDs for reliable lookup
    public List<string> DiscoveredDeviceNames { get; } = new();
    private Dictionary<string, string> DiscoveredDeviceIds { get; } = new(); // name -> id

    public event PropertyChangedEventHandler? PropertyChanged;

    public MotorBluetoothService()
    {
        _ble = CrossBluetoothLE.Current;
        _adapter = CrossBluetoothLE.Current.Adapter;

        _adapter.DeviceDiscovered += (s, a) =>
        {
            // Always record device by ID. If the advertised name is empty, use a short-id fallback
            string? deviceId = a.Device.Id != Guid.Empty ? a.Device.Id.ToString() : null;
            string displayName;

            if (!string.IsNullOrEmpty(a.Device.Name))
            {
                displayName = a.Device.Name;
            }
            else if (!string.IsNullOrEmpty(deviceId))
            {
                // Use short id as human readable fallback (first 8 chars)
                displayName = $"Device-{deviceId.Substring(0, 8)}";
            }
            else
            {
                // completely unknown device, skip
                return;
            }

            // Only proceed if we have a valid device ID
            if (!string.IsNullOrEmpty(deviceId))
            {
                // If name changed for same id, remove any previous mapping
                if (DiscoveredDeviceIds.ContainsValue(deviceId))
                {
                    var existing = DiscoveredDeviceIds.FirstOrDefault(kv => kv.Value == deviceId);
                    if (!string.IsNullOrEmpty(existing.Key) && existing.Key != displayName)
                    {
                        DiscoveredDeviceNames.Remove(existing.Key);
                        DiscoveredDeviceIds.Remove(existing.Key);
                    }
                }

                if (!DiscoveredDeviceIds.ContainsKey(displayName) || DiscoveredDeviceIds[displayName] != deviceId)
                {
                    DiscoveredDeviceIds[displayName] = deviceId;

                    if (!DiscoveredDeviceNames.Contains(displayName))
                    {
                        DiscoveredDeviceNames.Add(displayName);
                    }

                    OnPropertyChanged(nameof(DiscoveredDeviceNames));
                }
            }
        };
    }

    public async Task StartScanningAsync()
    {
        if (IsScanning) return;

        DiscoveredDeviceNames.Clear();
        DiscoveredDeviceIds.Clear();
        OnPropertyChanged(nameof(DiscoveredDeviceNames));

        IsScanning = true;
        _adapter.ScanTimeout = 10000; // set timeout in milliseconds (10 seconds)

        // Subscribe to scan timeout elapsed event to handle automatic stop
        _adapter.ScanTimeoutElapsed += OnScanTimeoutElapsed;

        // Filter scan by the service UUID to speed discovery of relevant devices
        try
        {
            await _adapter.StartScanningForDevicesAsync(new[] { _serviceUuid });
        }
        catch
        {
            // Fall back to broad scan if filtering isn't supported on platform
            await _adapter.StartScanningForDevicesAsync();
        }
        // Note: StartScanningForDevicesAsync returns immediately, scanning continues
        // until stopped explicitly or timeout elapses
    }

    private void OnScanTimeoutElapsed(object sender, EventArgs e)
    {
        // Handle automatic scan timeout
        _adapter.ScanTimeoutElapsed -= OnScanTimeoutElapsed;
        IsScanning = false;
        OnPropertyChanged(nameof(IsScanning));
        OnPropertyChanged(nameof(DiscoveredDeviceNames));
    }

    public async Task StopScanningAsync()
    {
        if (!IsScanning) return;

        // Unsubscribe from timeout event to prevent memory leaks
        _adapter.ScanTimeoutElapsed -= OnScanTimeoutElapsed;

        await _adapter.StopScanningForDevicesAsync();
        IsScanning = false;
        OnPropertyChanged(nameof(IsScanning));
        OnPropertyChanged(nameof(DiscoveredDeviceNames));
    }

    public async Task<bool> ConnectToDeviceAsync(string deviceName)
    {
        // Use centralized GUIDs

        IsConnecting = true;
        ConnectingDeviceName = deviceName;

        // إيقاف الفحص تماماً ولا داعي لاستئنافه تلقائياً للحفاظ على استقرار الاتصال
        if (IsScanning)
        {
            await StopScanningAsync();
        }

        try
        {
            IDevice device = null;

            // 1. جلب كائن الجهاز بسرعة باستخدام الـ ID أو الاسم
            if (DiscoveredDeviceIds.TryGetValue(deviceName, out string deviceId))
            {
                device = _adapter.DiscoveredDevices.FirstOrDefault(d => d.Id.ToString() == deviceId);
            }

            if (device == null)
            {
                device = _adapter.DiscoveredDevices.FirstOrDefault(d => d.Name == deviceName);
            }

            if (device == null)
            {
                System.Diagnostics.Debug.WriteLine("[BLE] Device not found in discovered list.");
                return false;
            }

            // 2. الاتصال بالجهاز مع وضع مهلة زمنية (Timeout) مدمجة في المكتبة
            System.Diagnostics.Debug.WriteLine("[BLE] Connecting to device...");

            // استخدام ConfigureAwait(false) يمنع تعليق خيط الواجهة (UI Thread Deadlock) ويجعل الاتصال لحظياً
            await _adapter.ConnectToDeviceAsync(device).ConfigureAwait(false);
            _connectedDevice = device;

            System.Diagnostics.Debug.WriteLine("[BLE] Connected! Discovering specific Service directly...");

            // 3. الحل السحري للسرعة: جلب الخدمة المحددة مباشرة بدون لووب foreach
            // جرب أولاً جلب الخدمة الافتراضية للـ ESP32 BLE
            IService service = null;
            try
            {
                service = await _connectedDevice.GetServiceAsync(_serviceUuid);
            }
            catch
            {
                // إذا فشل (بسبب اختلاف الـ UUID)، نأخذ أول خدمة كخيار احتياطي سريع جداً بدلاً من الـ لووب
                var services = await _connectedDevice.GetServicesAsync();
                service = services.FirstOrDefault();
            }

            if (service != null)
            {
                System.Diagnostics.Debug.WriteLine("[BLE] Service obtained. Getting Characteristic...");

                // 4. جلب الخاصية مباشرة
                try
                {
                    _writeCharacteristic = await service.GetCharacteristicAsync(_characteristicUuid);
                }
                catch
                {
                    // احتياطي: إذا لم يجد الـ UUID المحدد، يجلب أول خاصية تقبل الكتابة فوراً
                    var characteristics = await service.GetCharacteristicsAsync();
                    _writeCharacteristic = characteristics.FirstOrDefault(c => c.CanWrite);
                }
            }

            // تحديث المراجع العامة التي تستخدمها دالة إرسال السرعة لمنع الـ ObjectDisposedException
            _txCharacteristic = _writeCharacteristic;

            // تحديث حالة الاتصال في خيط الواجهة بشكل آمن
            IsConnected = _writeCharacteristic != null;

            System.Diagnostics.Debug.WriteLine($"[BLE RESULT] Connection success status: {IsConnected}");


            return IsConnected;
        }
        catch (Exception ex)
        {
            System.Diagnostics.Debug.WriteLine($"[BLE CRITICAL ERROR]: {ex.Message}");
            IsConnected = false;
            return false;
        }
        finally
        {
            IsConnecting = false;
            ConnectingDeviceName = null;

            // نصيحة هندسية: تم إزالة استئناف الـ Scan هنا؛ لأن تشغيل الفحص 
            // بالتزامن مع إرسال الأوامر هو سبب بطء إرسال السرعة وسبب حدوث الـ ObjectDisposedException!
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
        if (!IsConnected || _connectedDevice == null)
        {
            Console.WriteLine("[BLE ERROR] Cannot send command. Device is not connected.");
            return;
        }

        try
        {
            byte[] bytes = Encoding.UTF8.GetBytes(command);

            // 2. الفحص الذكي: إذا كان الكائن null أو تم التخلص منه (Disposed)، نحاول إعادة جلب الخدمة والخاصية فوراً
            if (_txCharacteristic == null)
            {
                Console.WriteLine("[BLE] Characteristic is null, trying to re-discover services...");
                _gattService = await _connectedDevice.GetServiceAsync(_serviceUuid);
                if (_gattService != null)
                {
                    _txCharacteristic = await _gattService.GetCharacteristicAsync(_characteristicUuid);
                }
            }

            if (_txCharacteristic != null)
            {
                Console.WriteLine($"[BLE EXECUTION] Sending command: {command}");
                // إرسال البيانات بشكل آمن
                await _txCharacteristic.WriteAsync(bytes);
            }
            else
            {
                Console.WriteLine("[BLE ERROR] Target Characteristic could not be found.");
            }
        }
        catch (ObjectDisposedException ex)
        {
            Console.WriteLine($"[BLE CRITICAL] Object was disposed: {ex.Message}. Resetting connection references...");
            // تصفير المراجع الميتة حتى يتم إعادة بنائها في المحاولة القادمة
            _txCharacteristic = null;
            _gattService = null;

            // تحديث حالة الواجهة لتظهر للمستخدم أن الاتصال يحتاج إعادة تهيئة
            IsConnected = false;
            OnPropertyChanged(nameof(IsConnected));
        }
        catch (Exception ex)
        {
            Console.WriteLine($"[BLE ERROR] Unexpected error while sending: {ex.Message}");
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