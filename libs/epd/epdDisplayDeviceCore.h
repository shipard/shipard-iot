#ifndef SHP_EPD_DISPLAY_DEVICE_CORE_H
#define SHP_EPD_DISPLAY_DEVICE_CORE_H


class ShpEpdDisplay;



class ShpEpdDisplayDeviceCore {

  public:

    ShpEpdDisplayDeviceCore(ShpEpdDisplay *epdDisplay);

    virtual void initDevice();
    virtual void setRotation(uint8_t rotation);
    virtual void displayImage();

  protected:

    ShpEpdDisplay *m_epdDisplay;
};

#endif // SHP_EPD_DISPLAY_DEVICE_CORE_H
