const int KEY_VOLUME_UP = 0;
const int KEY_VOLUME_DOWN = 1;
const int KEY_BRIGHTNESS_UP = 2;
const int KEY_BRIGHTNESS_DOWN = 3;
// const int KEY_MUTE = 2;
// const int KEY_MIC_MUTE = 3;

class KeyReporter {
public:
  virtual void reportKey(int key, int count) = 0;
  virtual void reportScroll(int steps) = 0;
};
