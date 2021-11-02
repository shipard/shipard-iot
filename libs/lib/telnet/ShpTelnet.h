#ifndef SHP_LIB_TELNET_H
#define SHP_LIB_TELNET_H


#define MAX_SHP_TELNET_CLIENTS 1
#define MAX_DATA_TELNET_BUF_LEN 250

class ShpTelnet 
{
	public: 
		ShpTelnet(int portNumber);

		void loop(Stream *readStream, Stream *writeStream);
		void write(char c);
		void write(char *string);

	protected:

		int m_listenPort;
		WiFiServer *m_server;
		WiFiClient m_serverClients[MAX_SHP_TELNET_CLIENTS];
		char m_buffer[MAX_DATA_TELNET_BUF_LEN];
};




#endif
