const int CFG_REQUEST_GET_STATUS = 0x01; // IN,  data: bytes [patch version high byte, patch version low byte]

const int CFG_REQUEST_SET_PARAMETER = 0x10; // OUT, wValue low byte: parameter key, wValue high byte: parameter value
const int CFG_REQUEST_GET_PARAMETER = 0x11; // IN,  wValue low byte: parameter key, data: parameter value (one byte)
const int CFG_REQUEST_SET_DEFAULTS = 0x12; // IN, data: none

const int CFG_REQUEST_IMG_PREPARE = 0x20; // OUT, wValue: image size in pages
const int CFG_REQUEST_IMG_INSTALL = 0x21; // OUT, wValue: CRC16

class FwuEndpoint : public usbd::UsbEndpoint {
public:

  FirmwareUpdate firmwareUpdate;

  int key = 0;
  int count = 0;
  unsigned char rxBuffer[flash::PAGE_SIZE];

  void init() {
    rxBufferPtr = rxBuffer;
    rxBufferSize = sizeof(rxBuffer);
    usbd::UsbEndpoint::init();
  }

  void rxComplete(int length) {
    // We expect length to be always equal to PAGE_SIZE. Last packet must be padded by the host.
    if (length == flash::PAGE_SIZE) {
      firmwareUpdate.write(rxBuffer);
    }
  };

};

class CfgInterface : public usbd::UsbInterface {
public:
  FwuEndpoint fwuEndpoint;
  DeviceConfiguration deviceConfiguration;

  virtual UsbEndpoint* getEndpoint(int index) { return index == 0 ? &fwuEndpoint : NULL; }

  const char* getLabel() { return "SoundSlide CFG"; }

  void init() {
    UsbInterface::init();
    deviceConfiguration.init();
  }

  void setup(SetupData* setup) {
    usbd::UsbEndpoint* endpoint = device->getControlEndpoint();
    switch (setup->bRequest) {

    case CFG_REQUEST_GET_STATUS: {
      endpoint->txBufferPtr[0] = project::versionInt[2] >> 8;
      endpoint->txBufferPtr[1] = project::versionInt[2] & 0xff;
      endpoint->startTx(2);
      break;
    }

    case CFG_REQUEST_SET_PARAMETER: {
      unsigned char key = setup->wValue & 0xff;
      unsigned char value = setup->wValue >> 8;
      deviceConfiguration.setParameter(key, value);
      endpoint->startTx(0);
      break;
    }

    case CFG_REQUEST_GET_PARAMETER: {
      unsigned char key = setup->wValue & 0xff;
      endpoint->txBufferPtr[0] = deviceConfiguration.getParameter(key);
      endpoint->startTx(1);
      break;
    }

    case CFG_REQUEST_SET_DEFAULTS: {
      deviceConfiguration.setDefaults();
      endpoint->startTx(0);
      break;
    }

    case CFG_REQUEST_IMG_PREPARE: {
      fwuEndpoint.firmwareUpdate.prepare(setup->wValue);
      endpoint->startTx(0);
      break;
    }

    case CFG_REQUEST_IMG_INSTALL: {
      unsigned int crc16 = setup->wValue;
      if (fwuEndpoint.firmwareUpdate.checkCrc(crc16)) {
        endpoint->startTx(0);
        fwuEndpoint.firmwareUpdate.install();
      } else {
        endpoint->stall();
      }
      break;
    }

    default:
      endpoint->stall();
    }
  }


};
