#ifndef SHP_EPD_DISPLAY_DEVICE_GxEPD2_H
#define SHP_EPD_DISPLAY_DEVICE_GxEPD2_H


class ShpEpdDisplayDeviceGxEPD2 : public ShpEpdDisplayDeviceCore {

  public:

    ShpEpdDisplayDeviceGxEPD2(ShpEpdDisplay *epdDisplay);

    virtual void initDevice();
    virtual void setRotation(uint8_t rotation);
    virtual void displayImage();

  protected:
    void showBitmap_PSRAM();
};

#endif // SHP_EPD_DISPLAY_DEVICE_GxEPD2_H
