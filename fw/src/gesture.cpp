class GestureDecoder : public genericTimer::Timer {

    TouchSensor* touchSensor;
    KeyReporter* keyReporter;
    DeviceConfiguration* deviceConfiguration;

    int oldFingerPos = -1;

    static const int queueSize = 4;
    signed char queue[queueSize];

    void onTimer() {

        checkSensor();
        optimizeQueue();
        checkQueue();

        // check every 20ms
        start(2);
    }

    void checkSensor() {

        int max = 0;
        int avg = 0;
        int maxIndex = 0;
        int channelCount = touchSensor->getChannelCount();
        for (int i = 0; i < channelCount; i++) {
            int v = touchSensor->getChannel(i);
            avg += v;
            if (v > max) {
                max = v;
                maxIndex = i;
            }
        }
        avg = avg / channelCount;

        int newFingerPos;
        if (max > avg * 2) {
            newFingerPos = maxIndex;
        }
        else {
            newFingerPos = -1;
        }

        if (newFingerPos != oldFingerPos && newFingerPos >= 0 && oldFingerPos >= 0) {

            int change = oldFingerPos - newFingerPos;

            if (change != 0) {

                if (deviceConfiguration->data.fields.flip) {
                    change = -change;
                }
                queue[0] = change * deviceConfiguration->data.fields.scale;

            }

        }

        oldFingerPos = newFingerPos;
    }

    void optimizeQueue() {

        // optimize queue to move always in one direction
        // check optimize-queue.jpg

        int positives = 0;
        int negatives = 0;
        for (int i = 0; i < queueSize; i++) {
            if (queue[i] > 0) {
                positives += queue[i];
            }
            if (queue[i] < 0) {
                negatives -= queue[i];
            }
        }
        int correction = positives;
        int side = 1;
        if (positives > negatives) {
            correction = negatives;
            side = -1;
        }


        for (int i = 0; i < queueSize; i++) {
            if (queue[i] * side > 0) {
                queue[i] = 0;
            }
        }

        for (int i = queueSize - 1; i >= 0; i--) {
            int v = queue[i];
            if (v < 0) {
                v = -v;
            }

            int c = correction;
            if (c > v) {
                c = v;
            }

            queue[i] += c * side;
            correction -= c;
        }

    }

    void checkQueue() {

        // report according to the last change
        int change = queue[queueSize - 1];

        switch (deviceConfiguration->data.fields.function) {

        case DEVICE_FUNCTION_VOLUME:
            if (change > 0) {
                keyReporter->reportKey(KEY_VOLUME_UP, change);
            }
            if (change < 0) {
                keyReporter->reportKey(KEY_VOLUME_DOWN, -change);
            }

            break;

        case DEVICE_FUNCTION_BRIGHTNESS:
            if (change > 0) {
                keyReporter->reportKey(KEY_BRIGHTNESS_UP, change);
            }
            if (change < 0) {
                keyReporter->reportKey(KEY_BRIGHTNESS_DOWN, -change);
            }

            break;

        case DEVICE_FUNCTION_SCROLL:
            keyReporter->reportScroll(change);
            break;
        }

        // shift queue
        for (int i = queueSize - 2; i >= 0; i--) {
            queue[i + 1] = queue[i];
        }
        queue[0] = 0;

    }


public:
    void init(TouchSensor* touchSensor, KeyReporter* keyReporter, DeviceConfiguration* deviceConfiguration) {
        this->touchSensor = touchSensor;
        this->keyReporter = keyReporter;
        this->deviceConfiguration = deviceConfiguration;

        // give user 2 seconds to remove finger, in case he just inserted SoundSlide in the USB port
        start(200);
    }

};

