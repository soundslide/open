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


  // Consumer Control (Report ID 1)
  0x05, 0x0C,        // Usage Page (Consumer Devices)
  0x09, 0x01,        // Usage (Consumer Control)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x01,        //   Report ID (1)
  0x09, 0xE9,        //   Usage (Volume Up)
  0x09, 0xEA,        //   Usage (Volume Down)
  0x09, 0x6F,        //   Usage (Brightness Up)
  0x09, 0x70,        //   Usage (Brightness Down)
  0x15, 0x00,        //   Logical Minimum (0)
  0x25, 0x01,        //   Logical Maximum (1)
  0x75, 0x01,        //   Report Size (1)
  0x95, 0x04,        //   Report Count (4)
  0x81, 0x02,        //   Input (Data, Variable, Absolute)
  0x95, 0x04,        //   Report Count (4) - Padding
  0x81, 0x03,        //   Input (Constant, Variable, Absolute)
  0xC0,              // End Collection

  // Mouse Scroll + Dummy Mouse (Report ID 2)
  0x05, 0x01,        // Usage Page (Generic Desktop)
  0x09, 0x02,        // Usage (Mouse)
  0xA1, 0x01,        // Collection (Application)
  0x85, 0x02,        //   Report ID (2)
  0x09, 0x01,        //   Usage (Pointer)
  0xA1, 0x00,        //   Collection (Physical)

  // Buttons (3 buttons)
  0x05, 0x09,        //     Usage Page (Button)
  0x19, 0x01,        //     Usage Minimum (1)
  0x29, 0x03,        //     Usage Maximum (3)
  0x15, 0x00,        //     Logical Minimum (0)
  0x25, 0x01,        //     Logical Maximum (1)
  0x95, 0x03,        //     Report Count (3)
  0x75, 0x01,        //     Report Size (1)
  0x81, 0x02,        //     Input (Data, Variable, Absolute)

  // Padding for 1 byte
  0x95, 0x01,        //     Report Count (1)
  0x75, 0x05,        //     Report Size (5)
  0x81, 0x03,        //     Input (Constant, Variable, Absolute)

  // X and Y
  0x05, 0x01,        //     Usage Page (Generic Desktop)
  0x09, 0x30,        //     Usage (X)
  0x09, 0x31,        //     Usage (Y)
  0x15, 0x81,        //     Logical Minimum (-127)
  0x25, 0x7F,        //     Logical Maximum (127)
  0x75, 0x08,        //     Report Size (8)
  0x95, 0x02,        //     Report Count (2)
  0x81, 0x06,        //     Input (Data, Variable, Relative)

  // Scroll Wheel
  0x09, 0x38,        //     Usage (Wheel)
  0x15, 0x81,        //     Logical Minimum (-127)
  0x25, 0x7F,        //     Logical Maximum (127)
  0x75, 0x08,        //     Report Size (8)
  0x95, 0x01,        //     Report Count (1)
  0x81, 0x06,        //     Input (Data, Variable, Relative)

  0xC0,              //   End Collection
  0xC0               // End Collection

};

class HidEndpoint : public usbd::UsbEndpoint {
public:
  int key = 0;
  int count = 0;
  int scroll = 0;
  union {
    struct {
      unsigned char report_id;   // 0x01
      unsigned char keys : 4;
      unsigned char padding : 4;
    } consumer;

    struct {
      unsigned char report_id;   // 0x02
      unsigned char buttons : 3;
      unsigned char reserved : 5;
      signed char x;
      signed char y;
      signed char wheel;
    } mouse;
  } txBuffer;

  void init() {
    txBufferPtr = (unsigned char*)&txBuffer;
    txBufferSize = sizeof(txBuffer);
    usbd::UsbEndpoint::init();
  }

  void reportKey(int key, int count) {
    if (count) {
      this->key = key;
      this->count = count * 2; // one for key down and one for key up
      this->scroll;
      sendReport();
    }
  }

  void reportScroll(int steps) {
    this->key = 0;
    this->count = 0;
    this->scroll = steps;
    sendReport();
  }

  void sendReport() {

    if (count > 0) {

      txBuffer.consumer.report_id = 1; // Report ID 1 for key events
      txBuffer.consumer.keys = (count & 1) ? 0 : (1 << key);
      txBuffer.consumer.padding = 0;
      count--;
      startTx(sizeof(txBuffer.consumer));

    }
    else if (scroll != 0) {

      txBuffer.mouse.report_id = 2; // Report ID 2 for mouse events
      txBuffer.mouse.buttons = 0; // No buttons pressed
      txBuffer.mouse.x = 0; // No X movement
      txBuffer.mouse.y = 0; // No Y movement
      txBuffer.mouse.wheel = scroll; // Set the scroll value
      scroll = 0; // Reset scroll after sending
      startTx(sizeof(txBuffer));

    }

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