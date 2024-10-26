static const int SENSOR_CHANNELS = 8;
static const int SENSOR_PINS[SENSOR_CHANNELS] = { 2, 3, 4, 5, 6, 7, 14, 15 };
static const target::adc::INPUTCTRL::MUXPOS ADC_INPUTS[SENSOR_CHANNELS] = {
    target::adc::INPUTCTRL::MUXPOS::PIN0,
    target::adc::INPUTCTRL::MUXPOS::PIN1,
    target::adc::INPUTCTRL::MUXPOS::PIN2,
    target::adc::INPUTCTRL::MUXPOS::PIN3,
    target::adc::INPUTCTRL::MUXPOS::PIN4,
    target::adc::INPUTCTRL::MUXPOS::PIN5,
    target::adc::INPUTCTRL::MUXPOS::PIN6,
    target::adc::INPUTCTRL::MUXPOS::PIN7
};

class ResistiveTouchSensor : public TouchSensor {

    int channel = 0;
    int values[SENSOR_CHANNELS];
    int sensitivity = -1;
    int threshold;

    DeviceConfiguration* deviceConfiguration;

    void startConversion(int ch) {
        channel = ch;
        target::ADC.INPUTCTRL.setMUXPOS(ADC_INPUTS[ch]);
        target::ADC.SWTRIG.setSTART(true);
    }

public:

    void init(DeviceConfiguration* deviceConfiguration) {

        this->deviceConfiguration = deviceConfiguration;

        for (int i = 0; i < SENSOR_CHANNELS; i++) {
            target::PORT.DIRCLR.setDIRCLR(1 << SENSOR_PINS[i]);
            target::PORT.PINCFG[SENSOR_PINS[i]] = target::PORT.PINCFG->bare().setPMUXEN(true);//.setPULLEN(true);
            target::PORT.OUTSET.setOUTSET(1 << SENSOR_PINS[i]);

            if (SENSOR_PINS[i] & 1) {
                target::PORT.PMUX[SENSOR_PINS[i] >> 1].setPMUXO(target::port::PMUX::PMUXO::B);
            }
            else {
                target::PORT.PMUX[SENSOR_PINS[i] >> 1].setPMUXE(target::port::PMUX::PMUXE::B);
            }

        }

        // GC0 -> ADC

        target::GCLK.CLKCTRL = target::GCLK.CLKCTRL.bare()
            .setID(target::gclk::CLKCTRL::ID::ADC)
            .setGEN(target::gclk::CLKCTRL::GEN::GCLK0)
            .setCLKEN(true);

        target::PM.APBCMASK.setADC(true);

        target::ADC.CALIB.setLINEARITY_CAL(target::NVMCALIB.SOFT1.getADC_LINEARITY_MSB() << 5 | target::NVMCALIB.SOFT0.getADC_LINEARITY_LSB());
        target::ADC.CALIB.setBIAS_CAL(target::NVMCALIB.SOFT1.getADC_BIASCAL());

        target::ADC.REFCTRL.setREFSEL(target::adc::REFCTRL::REFSEL::INTVCC1);
        target::ADC.SAMPCTRL.setSAMPLEN(1);

        target::ADC.CTRLB.setRESSEL(target::adc::CTRLB::RESSEL::_8BIT).setPRESCALER(target::adc::CTRLB::PRESCALER::DIV512);

        target::ADC.INPUTCTRL = target::ADC.INPUTCTRL.bare()
            .setMUXNEG(target::adc::INPUTCTRL::MUXNEG::GND)
            .setGAIN(target::adc::INPUTCTRL::GAIN::DIV2);

        target::NVIC.ISER.setSETENA(1 << target::interrupts::External::ADC);
        target::ADC.INTENSET.setRESRDY(true);

        target::ADC.CTRLA.setENABLE(true);

        startConversion(0);
    }

    void interruptHandlerADC() {

        if (target::ADC.INTFLAG.getRESRDY()) {
            target::ADC.INTFLAG.setRESRDY(true);

            int value = 0xFF - target::ADC.RESULT.getRESULT();

            if (sensitivity != deviceConfiguration->data.fields.sensitivity) {
                sensitivity = deviceConfiguration->data.fields.sensitivity;
                threshold = ((100 - sensitivity) * 7 / 100) + 2;
            }

            if (value < threshold || sensitivity == 0) {
                value = 0;
            }

            const int smoothing = 3;
            values[channel] = ((value << 16) + values[channel] * smoothing) / (smoothing + 1);

            channel++;
            if (channel >= SENSOR_CHANNELS) {
                channel = 0;
            }

            startConversion(channel);
        }

    }

    virtual int getChannelCount() {
        return SENSOR_CHANNELS;
    }

    virtual int getChannel(int channel) {
        return values[channel];
    }
};



