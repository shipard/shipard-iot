#ifndef SHP_CLIENT_UART_H
#define SHP_CLIENT_UART_H


class ShpClientUART
{
	public:

		ShpClientUART();

		//virtual void init();
		virtual void loop();

  protected:

    uint8_t m_rowAppended;
    String m_oneRowBuffer;
    String m_cmdBuffer;
};

#endif
