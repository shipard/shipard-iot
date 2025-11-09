#ifndef SHP_EPD_DISPLAY_DEVICE_133S6_H
#define SHP_EPD_DISPLAY_DEVICE_133S6_H


class ShpEpdDisplayDevice133S6 : public ShpEpdDisplayDeviceCore {

  public:

    ShpEpdDisplayDevice133S6(ShpEpdDisplay *epdDisplay);

    virtual void initDevice();
    virtual void setRotation(uint8_t rotation);
    virtual void displayImage();

  protected:
    void showBitmap_PSRAM();
};

#endif // SHP_EPD_DISPLAY_DEVICE_133S6_H
