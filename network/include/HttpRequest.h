#ifndef _VIGO_NETWORK_HTTP_REQUEST_H_
#define _VIGO_NETWORK_HTTP_REQUEST_H_

typedef enum
{
	HTTP_GET = 1,
	HTTP_PUT,
	HTTP_POST,
	HTTP_OPTIONS,
	HTTP_HEAD,
	HTTP_DELETE,
	HTTP_TRACE
} Http_method;

class CHTTPRequest
{
public:
	CHTTPRequest();
	~CHTTPRequest();
	
	static int BuildHttpGetRequest(char *pBuf, int *pLen, char *strHostName, 
		short nHostPort, void *pProxySetting = NULL);
	static int BuildHttpPutRequest(char *pBuf, int *pLen, char *strHostName, 
		short nHostPort, int nContentLen, void *pProxySetting = NULL);
	static int BuildHttpPostRequest(char *pBuf, int *pLen, char *strHostName, 
		short nHostPort, int nContentLen, void *pProxySetting = NULL);
	static int ParseHttpRequest(char *pBuf, int nLen, int *pMethod);

	static int ParseHttpResponse(char *pBuf, int nLen);
	static int BuildHttpResponse(char *pBuf, int *pLen, int nContentLen);
private:
	static int BuildHttpMethod(Http_method nMethod, char *pBuf, int *pLen, 
		char *strHostName, short nHostPort, int nContentLen, void *pProxySetting = NULL);
	static char *HttpMethodToString (Http_method nMethod);
	static int HttpStringToMethod(char *pStr);
};

#endif

