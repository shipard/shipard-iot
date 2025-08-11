#ifndef SHP_CONTROL_MATRIX_H
#define SHP_CONTROL_MATRIX_H


#define CONTROL_MATRIX_MAX_ROWS 10
#define CONTROL_MATRIX_MAX_COLS 10

class ShpControlMatrix : public ShpIOPort
{
	public:

		ShpControlMatrix();

		virtual void init(JsonVariant portCfg);
		virtual void init2();
		virtual void loop();
		virtual void onMessage(byte* payload, unsigned int length, const char* subCmd);
    void onStop(int pin);


	protected:

    void matrixOn(uint8_t col, uint8_t row);
    void matrixOff();

    uint8_t m_cntCols;
    uint8_t m_cntRows;
		int8_t m_pinStop;
    int8_t m_pinCurrent;

    bool m_isBusy;
    int8_t m_activeCol;
    int8_t m_activeRow;
    volatile uint8_t m_needStop;
    long m_stopTimeout;
    long m_stopAfter;
    bool m_needSendBusy;

    int8_t m_pinsCols[CONTROL_MATRIX_MAX_COLS];
    int8_t m_pinsRows[CONTROL_MATRIX_MAX_ROWS];

    String m_topicBusy;

};


#endif
