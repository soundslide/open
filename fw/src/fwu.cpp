const int FWU_UPLOAD_BASE_ADDRESS = 0x2000;
const int FWU_UPLOAD_MAX_PAGES = (0x2000 - flash::PAGES_PER_ROW * flash::PAGE_SIZE) / flash::PAGE_SIZE; // last row in flash memory is reserved for configuration
const unsigned short CRC16_SEED = 0x1234;


class FirmwareUpdate {
    int pagesWritten = 0;

public:
    void prepare(int pages) {
        pagesWritten = 0;
    }

    void write(unsigned char* page) {
        if (pagesWritten < FWU_UPLOAD_MAX_PAGES) {
            flash::writePage((void*)(FWU_UPLOAD_BASE_ADDRESS + pagesWritten * flash::PAGE_SIZE), page);
            pagesWritten++;
        }
    }

    bool checkCrc(unsigned short theirCrc16) {
        unsigned short ourCrc16 = CRC16_SEED;
        for (int offset = 0; offset < pagesWritten * flash::PAGE_SIZE; offset += 2) {
            unsigned short i = *(unsigned short*)(FWU_UPLOAD_BASE_ADDRESS + offset);
            ourCrc16 = ourCrc16 ^ i;
        }

        return ourCrc16 == theirCrc16;
    }

    void install() {
        (*flash::moveAndReset)((void*)0x0000, (void*)FWU_UPLOAD_BASE_ADDRESS, pagesWritten);
    }

};

