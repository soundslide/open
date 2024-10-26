    #define WRITE_PAGE(dst, src) \
  \
        if ((int)dst % (PAGES_PER_ROW * PAGE_SIZE) == 0) {  \
            target::NVMCTRL.ADDR.setADDR((int)dst >> 1);  \
            target::NVMCTRL.CTRLA = target::NVMCTRL.CTRLA.bare().setCMD(target::nvmctrl::CTRLA::CMD::ER).setCMDEX(target::nvmctrl::CTRLA::CMDEX::KEY);  \
            while (target::NVMCTRL.INTFLAG.getREADY() == 0);  \
        }  \
  \
        target::NVMCTRL.CTRLB.setMANW(true);  \
        target::NVMCTRL.ADDR.setADDR((int)dst >> 1);  \
        for (int i = 0; i < PAGE_SIZE; i += 4) {  \
            unsigned int* dstPtr = (unsigned int*)((int)dst + i);  \
            unsigned int* srcPtr = (unsigned int*)((int)src + i);  \
            *dstPtr = *srcPtr;  \
        }  \
        target::NVMCTRL.CTRLA = target::NVMCTRL.CTRLA.bare().setCMD(target::nvmctrl::CTRLA::CMD::WP).setCMDEX(target::nvmctrl::CTRLA::CMDEX::KEY);  \
        while (target::NVMCTRL.INTFLAG.getREADY() == 0);

namespace flash {
    const int PAGE_SIZE = 64;
    const int PAGES_PER_ROW = 4;

    void writePage(void* dst, void* src) {
        WRITE_PAGE(dst, src);
    }

    extern "C" void (*moveAndReset)(void* dst, void* src, int pages);

    __attribute__((section(".mover"))) void myMoverFnc(void* dst, void* src, int pages) {

        asm volatile("cpsid i");

        for (int page = 0; page < pages; page++) {
            int offset = page * flash::PAGE_SIZE;
            WRITE_PAGE((void*)((int)dst + offset), (void*)((int)src + offset));
        }

        #define AIRCR (*(volatile unsigned int*)0xE000ED0C)
        #define AIRCR_VECTKEY_MASK 0x05FA0000  // VECTKEY, needed to write to the AIRCR
        #define SYSRESETREQ_BIT    0x00000004  // SYSRESETREQ bit position

        AIRCR = AIRCR_VECTKEY_MASK | SYSRESETREQ_BIT;
    }

    __attribute__((used)) __attribute__((section(".mover_ptr"))) volatile const void* myMoverPtr = (void*)&myMoverFnc;
}


