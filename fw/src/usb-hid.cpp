#include <string.h>

#define HID_GET_DESCRIPTOR 0x06
#define HID_DESCRIPTOR_TYPE_HID 0x21
#define HID_DESCRIPTOR_TYPE_REPORT 0x22

struct __attribute__((packed)) HidDescriptor {
  unsigned char bLength;
  unsigned char bDescriptorType;
  unsigned short bcdHID;
  unsigned char bCountryCode;
  unsigned char bNumDescriptors;
  unsigned char bDescriptorType2;
  unsigned short wDescriptorLength;
};

const unsigned char hidReportDescriptor[] = {
    0x05, 0x0C,        // Usage Page (Consumer Devices)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x09, 0xE9,        //   Usage (Volume Up)
    0x09, 0xEA,        //   Usage (Volume Down)
    0x09, 0xE2,        //   Usage (Mute)
    0x09, 0xF8,        //   Usage (Microphone Mute)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1 bit)
    0x95, 0x04,        //   Report Count (4) - 4 buttons (Volume Up, Volume Down, Mute, Mic Mute)
    0x81, 0x02,        //   Input (Data, Variable, Absolute)
    0x95, 0x04,        //   Report Count (4) - Padding bits to make a byte (8 bits)
    0x81, 0x03,        //   Input (Constant, Variable, Absolute)
    0xC0               // End Collection
};

class HidEndpoint : public usbd::UsbEndpoint {
public:
  int key = 0;
  int count = 0;
  unsigned char txBuffer[1];

  void init() {
    txBufferPtr = txBuffer;
    txBufferSize = sizeof(txBuffer);
    usbd::UsbEndpoint::init();
  }

  void reportKey(int key, int count) {
    if (count) {
      this->key = key;
      this->count = count * 2; // one for key down and one for key up
      sendReport();
    }
  }

  void sendReport() {
    txBuffer[0] = (count & 1) ? 0 : (1 << key);
    startTx(sizeof(txBuffer));
    count--;
  }

  void txComplete() {
    if (count) {
      sendReport();
    }
  }

};

class HidInterface : public usbd::UsbInterface {
public:
  HidEndpoint hidEndpoint;

  virtual UsbEndpoint* getEndpoint(int index) { return index == 0 ? &hidEndpoint : NULL; }

  const char* getLabel() { return "SoundSlide HID"; }

  void checkDescriptor(InterfaceDescriptor* interfaceDescriptor) {
    interfaceDescriptor->bInterfaceClass = 0x03;
    interfaceDescriptor->bInterfaceSubclass = 0x00;
    interfaceDescriptor->bInterfaceProtocol = 0x00;
  };

  int getClassDescriptorLength() { return sizeof(HidDescriptor); }

  void checkClassDescriptor(unsigned char* buffer) {
    HidDescriptor* hidDescriptor = (HidDescriptor*)buffer;
    hidDescriptor->bLength = sizeof(HidDescriptor);
    hidDescriptor->bDescriptorType = HID_DESCRIPTOR_TYPE_HID;
    hidDescriptor->bcdHID = 0x0110; // HID Class Specification 1.10
    hidDescriptor->bCountryCode = 0; // Not Supported
    hidDescriptor->bNumDescriptors = 1; // Number of HID class descriptors to follow
    hidDescriptor->bDescriptorType2 = HID_DESCRIPTOR_TYPE_REPORT;
    hidDescriptor->wDescriptorLength = sizeof(hidReportDescriptor);
  }

  void setup(SetupData* setup) {
    usbd::UsbEndpoint* endpoint = device->getControlEndpoint();
    if (
      setup->bRequest = HID_GET_DESCRIPTOR &&
      setup->wValue == (HID_DESCRIPTOR_TYPE_REPORT << 8) | 0 &&
      setup->wIndex == 0
      ) {
      memcpy(endpoint->txBufferPtr, hidReportDescriptor, sizeof(hidReportDescriptor));
      endpoint->startTx(sizeof(hidReportDescriptor));
    }
    else {
      endpoint->stall();
    }
  }


};