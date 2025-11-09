#ifndef SHP_EPD_DISPLAY_H
#define SHP_EPD_DISPLAY_H


#define EPD_PHASE_WAIT 					0
#define EPD_PHASE_LOAD_IMAGE 		1
#define EPD_PHASE_DISPLAY_IMAGE 2

#define EPD_RELOAD_MODE_NONE 							0
#define EPD_RELOAD_MODE_TIME_IN_MINUTES 	1


class ShpEpdDisplay : public ShpIOPort
{
	public:

		ShpEpdDisplay();

		virtual void init(JsonVariant portCfg);
		virtual void loop();
		virtual void shutdown();


		void loadNewImageVersion();
		void loadImage();
		void displayImage();

		void clearImgInfo();
		void parseFileHeader (const uint8_t *hdr);

	protected:
		String m_epdId;
		uint8_t m_phase;
		bool m_imageLoaded;
		bool m_imageDisplayed;
		long m_nextImageReloadAfter;
		long m_imageReloadInterval;

		bool m_imgInfoLoaded;
		uint8_t m_imgOrientation;
		uint8_t m_sleepReloadMode;
		int m_sleepReloadInterval;

		int m_newImageVersion;
		bool m_loadNewImageVersionDone;


		ShpEpdDisplayDeviceCore* m_displayDevice;

		public:
			uint16_t m_imgWidth;
			uint16_t m_imgHeight;
			byte* m_imgData;
			uint32_t m_imgDataSize;
};


#endif
