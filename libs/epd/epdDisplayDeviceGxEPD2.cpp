extern SHP_APP_CLASS *app;


#define SHP_GxEPD2_420c               1
#define SHP_GxEPD2_750c               2
#define SHP_GxEPD2_1248c              3
#define SHP_GxEPD2_1248               4
#define SHP_GxEPD2_750c_Z08           5
#define SHP_GxEPD2_266c_GDEY0266F51H  6
#define SHP_GxEPD2_583_GDEQ0583T31    7
#define SHP_GxEPD2_730c_GDEY073D46    8
#define SHP_GxEPD2_730c_GDEP073E01    9


#define SHP_BF_COUNT_BITS             6
#define SHP_BF_COUNT_MASK             0b00111111

#define BUSY 4
#define RST  16
#define DC   17
#define CS   5

#ifdef SHP_EPD_BOARD_M25
#define BUSY 15
#define RST  16
#define DC   14
#define CS   10
#endif

#if !defined(GxEPD2_DRIVER_CLASS)
  #error "ERROR! symbol GxEPD2_DRIVER_CLASS is not defined.";
#endif

#define epdColorMap {GxEPD_BLACK,GxEPD_WHITE,GxEPD_RED,GxEPD_YELLOW,GxEPD_BLUE,GxEPD_GREEN,GxEPD_ORANGE}


#if GxEPD2_DRIVER_CLASS == SHP_GxEPD2_420c
  GxEPD2_3C<GxEPD2_420c, GxEPD2_420c::HEIGHT> display(GxEPD2_420c(CS, DC, RST, BUSY));
#elif GxEPD2_DRIVER_CLASS == SHP_GxEPD2_750c
  GxEPD2_3C<GxEPD2_750c, GxEPD2_750c::HEIGHT> display(GxEPD2_750c(CS, DC, RST, BUSY));
#elif GxEPD2_DRIVER_CLASS == SHP_GxEPD2_750c_Z08
  GxEPD2_3C<GxEPD2_750c_Z08, GxEPD2_750c_Z08::HEIGHT / 2> display(GxEPD2_750c_Z08(CS, DC, RST, BUSY));
#elif GxEPD2_DRIVER_CLASS == SHP_GxEPD2_266c_GDEY0266F51H
  GxEPD2_4C<GxEPD2_266c_GDEY0266F51H, GxEPD2_266c_GDEY0266F51H::HEIGHT> display(GxEPD2_266c_GDEY0266F51H(CS, DC, RST, BUSY));
#elif GxEPD2_DRIVER_CLASS == SHP_GxEPD2_730c_GDEY073D46
  #define SHP_BF_COUNT_BITS             5
  #define SHP_BF_COUNT_MASK             0b00011111
  GxEPD2_7C<GxEPD2_730c_GDEY073D46, GxEPD2_730c_GDEY073D46::HEIGHT / 4> display(GxEPD2_730c_GDEY073D46(CS, DC, RST, BUSY));
#elif GxEPD2_DRIVER_CLASS == GxEPD2_730c_GDEP073E01
  #define SHP_BF_COUNT_BITS             5
  #define SHP_BF_COUNT_MASK             0b00011111
  GxEPD2_7C<GxEPD2_730c_GDEP073E01, GxEPD2_730c_GDEP073E01::HEIGHT / 4> display(GxEPD2_730c_GDEP073E01(CS, DC, RST, BUSY));
#elif GxEPD2_DRIVER_CLASS == SHP_GxEPD2_583_GDEQ0583T31
  #define SHP_BF_COUNT_BITS             7
  #define SHP_BF_COUNT_MASK             0b01111111
  GxEPD2_BW<GxEPD2_583_GDEQ0583T31, GxEPD2_583_GDEQ0583T31::HEIGHT> display(GxEPD2_583_GDEQ0583T31(CS, DC, RST, BUSY));
#elif GxEPD2_DRIVER_CLASS == SHP_GxEPD2_1248c
  GxEPD2_3C<GxEPD2_1248c, GxEPD2_1248c::HEIGHT / 8>
  display(GxEPD2_1248c(
      12, /* sck */ 13, /* miso */ 11, /* mosi */
      39, /* cs_m1 */ 40, /* cs_s1 */ 7, /* cs_m2 */ 8, /* cs_s2 */
      14, /* dc1 */ 15, /* dc2 */
      9, /* rst1 */ 10, /* rst2 */
      41, /* busy_m1 */ 42, /* busy_s1 */ 5, /* busy_m2 */ 6  /* busy_s2 */
      ));
#elif GxEPD2_DRIVER_CLASS == SHP_GxEPD2_1248
  #define SHP_BF_COUNT_BITS             7
  #define SHP_BF_COUNT_MASK             0b01111111
  GxEPD2_BW<GxEPD2_1248, GxEPD2_1248c::HEIGHT / 8>
  display(GxEPD2_1248(
      13, /* sck */ 2, /* miso */ 14, /* mosi */
      23, /* cs_m1 */ 22, /* cs_s1 */ 16, /* cs_m2 */ 19, /* cs_s2 */
      25, /* dc1 */ 17, /* dc2 */
      33, /* rst1 */ 5, /* rst2 */
      32, /* busy_m1 */ 26, /* busy_s1 */ 18, /* busy_m2 */ 4  /* busy_s2 */
      ));
#else
  #error "ERROR - UNKNOWN DISPLAY TYPE!";
#endif


#include <Fonts/FreeMonoBold9pt7b.h>



ShpEpdDisplayDeviceGxEPD2::ShpEpdDisplayDeviceGxEPD2(ShpEpdDisplay *epdDisplay) : ShpEpdDisplayDeviceCore(epdDisplay)
{
}


void ShpEpdDisplayDeviceGxEPD2::initDevice()
{
}

void ShpEpdDisplayDeviceGxEPD2::setRotation(uint8_t rotation)
{
  display.setRotation(rotation);
}

void ShpEpdDisplayDeviceGxEPD2::displayImage()
{
  display.init(115200, true, 2, false);
  display.setFullWindow();
  display.firstPage();

  do
  {
    showBitmap_PSRAM();
    //delay(100);
  } while (display.nextPage());
}

void ShpEpdDisplayDeviceGxEPD2::showBitmap_PSRAM()
{

  uint16_t colorMap[] = epdColorMap;

  //Serial.printf("showBitmap - width: %d, height: %d \n", m_imgWidth, m_imgHeight);

  uint16_t displayPosX = 0;
  uint16_t displayPosY = 0;

  for (size_t pos = 64; pos < m_epdDisplay->m_imgDataSize; pos++)
  {
    uint8_t count = m_epdDisplay->m_imgData[pos] & SHP_BF_COUNT_MASK;
    uint8_t pixel_color = m_epdDisplay->m_imgData[pos] >> SHP_BF_COUNT_BITS;

    uint16_t color = colorMap[pixel_color];

    for (uint8_t xx = 0; xx < count; xx++)
    {
      display.drawPixel(displayPosX, displayPosY, color);
      displayPosX++;
      if (displayPosX == m_epdDisplay->m_imgWidth)
      {
        displayPosY++;
        displayPosX = 0;
        yield();
      }
    }
  }
}
