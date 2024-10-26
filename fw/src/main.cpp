const int LED_PIN = 23;

class SoundSlideUsbDevice : public atsamd::usbd::AtSamdUsbDevice, public KeyReporter {
public:
  HidInterface hidInterface;
  CfgInterface cfgInterface;

  UsbControlEndpoint controlEndpoint;

  UsbInterface* getInterface(int index) {
    switch (index) {
    case 0: return &hidInterface;
    case 1:      return &cfgInterface;
    default:      return NULL;
    }
  };

  UsbEndpoint* getControlEndpoint() { return &controlEndpoint; };

  void checkDescriptor(DeviceDescriptor* deviceDescriptor) {
    deviceDescriptor->idVendor = 0xF5A2;
    deviceDescriptor->idProduct = 0x0001;
    deviceDescriptor->bcdDevice = project::versionInt[0] << 8 | project::versionInt[1];
  };

  const char* getManufacturer() { return "Drake Labs"; }
  const char* getProduct() { return "SoundSlide"; }

  void reportKey(int key, int count) {
    this->hidInterface.hidEndpoint.reportKey(key, count);
  };
};

GestureDecoder gestureDecoder;
ResistiveTouchSensor touchSensor;
SoundSlideUsbDevice usbDevice;

void interruptHandlerUSB() { usbDevice.interruptHandlerUSB(); }
void interruptHandlerADC() { touchSensor.interruptHandlerADC(); }

void initApplication() {
  atsamd::safeboot::init(9, false, LED_PIN);

  usbDevice.useInternalOscillators();
  usbDevice.init();

  touchSensor.init(&usbDevice.cfgInterface.deviceConfiguration);
  gestureDecoder.init(&touchSensor, &usbDevice, &usbDevice.cfgInterface.deviceConfiguration);
}


