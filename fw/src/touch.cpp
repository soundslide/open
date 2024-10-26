class TouchSensor {
public:
    virtual int getChannelCount() = 0;
    virtual int getChannel(int channel) = 0;
};