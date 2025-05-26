const int CONFIG_BASE_ADDRESS = 0x4000 - flash::PAGES_PER_ROW * flash::PAGE_SIZE; // last row in flash memory

const int DEVICE_FUNCTION_VOLUME = 0x00; // Volume control function
const int DEVICE_FUNCTION_SCROLL = 0x01; // Scroll function
const int DEVICE_FUNCTION_BRIGHTNESS = 0x02; // Brightness control function

class DeviceConfiguration : public applicationEvents::EventHandler {
    int saveConfigEventId;

public:
    union {
        unsigned char raw[4];
        struct {
            unsigned char flip; // 0 - normal, 1 - flip, default: 0
            unsigned char scale; // sensor step multiplier 1..4, default: 2
            unsigned char sensitivity; // sensor sensitivity 0..100, default: 20
            unsigned char function; // see DEVICE_FUNCTION_* constants
        } fields;
    } data;

    void init() {
        saveConfigEventId = applicationEvents::createEventId();
        handle(saveConfigEventId);

        for (int i = 0; i < sizeof(data.raw); i++) {
            unsigned char* ptr = (unsigned char*)(CONFIG_BASE_ADDRESS + i);
            data.raw[i] = *ptr;
        }

        // check for uninitialized flash (new device)
        if (data.fields.flip == 0xff) {
            setDefaults();
        }
    }

    void setParameter(unsigned char key, unsigned char value) {
        if (key < sizeof(data.raw)) {
            data.raw[key] = value;
            applicationEvents::schedule(saveConfigEventId);
        }
    }

    unsigned char getParameter(unsigned char key) {
        if (key < sizeof(data.raw)) {
            return data.raw[key];
        }
        return 0;
    }

    void setDefaults() {
        data.fields.flip = 0;
        data.fields.scale = 2;
        data.fields.sensitivity = 30;
        data.fields.function = DEVICE_FUNCTION_VOLUME;
        applicationEvents::schedule(saveConfigEventId);
    }

    void onEvent() {
        unsigned char buffer[flash::PAGE_SIZE];
        zeromem(buffer, sizeof(buffer));
        for (int i = 0; i < sizeof(data.raw); i++) {
            buffer[i] = data.raw[i];
        }
        flash::writePage((void*)CONFIG_BASE_ADDRESS, buffer);
    }
};