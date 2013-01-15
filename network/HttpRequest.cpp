/*****************************************************************
 * Http Request Generate and parse
 * Frank Song 2005-8-22
 ****************************************************************/
#include "CmBase.h"
#include "TimeValue.h"
#include "HttpRequest.h"
#include "Addr.h"
#include "NetworkInterface.h"
#include "base64.h"

/*
 *	CHTTPRequest
 */
CHTTPRequest::CHTTPRequest()
{
}

CHTTPRequest::~CHTTPRequest()
{
}

int CHTTPRequest::BuildHttpGetRequest(char *pBuf, int *pLen, 
									char *strHostName, short nHostPort, 
									void *pProxySetting)
{
	return BuildHttpMethod(HTTP_GET, pBuf, pLen, strHostName, nHostPort, -1, pProxySetting);
}

int CHTTPRequest::BuildHttpPutRequest(char *pBuf, int *pLen, 
									char *strHostName, short nHostPort, 
									int nContentLen, void *pProxySetting /* = NULL */)
{
	return BuildHttpMethod(HTTP_PUT, pBuf, pLen, strHostName, nHostPort, 0, pProxySetting);
}

int CHTTPRequest::BuildHttpPostRequest(char *pBuf, int *pLen, 
									 char *strHostName, short nHostPort, 
									 int nContentLen, void *pProxySetting /* = NULL */)
{
	return BuildHttpMethod(HTTP_POST, pBuf, pLen, strHostName, nHostPort, 0, pProxySetting);
}

char *CHTTPRequest::HttpMethodToString (Http_method nMethod)
{
	switch (nMethod)
    {
		case HTTP_GET: return "GET";
		case HTTP_PUT: return "PUT";
		case HTTP_POST: return "POST";
		case HTTP_OPTIONS: return "OPTIONS";
		case HTTP_HEAD: return "HEAD";
		case HTTP_DELETE: return "DELETE";
		case HTTP_TRACE: return "TRACE";
    }
	return "(uknown)";
}

int CHTTPRequest::HttpStringToMethod(char *pStr)
{
	if (strncmp (pStr, "GET", 3) == 0)
		return HTTP_GET;
	if (strncmp (pStr, "PUT", 3) == 0)
		return HTTP_PUT;
	if (strncmp (pStr, "POST", 4) == 0)
		return HTTP_POST;
	if (strncmp (pStr, "OPTIONS", 7) == 0)
		return HTTP_OPTIONS;
	if (strncmp (pStr, "HEAD", 4) == 0)
		return HTTP_HEAD;
	if (strncmp (pStr, "DELETE", 6) == 0)
		return HTTP_DELETE;
	if (strncmp (pStr, "TRACE", 5) == 0)
		return HTTP_TRACE;
	return -1;
}
/*
 *	The value of proxy addr and proxy port should be dest addr and dest port here
 */
int CHTTPRequest::BuildHttpMethod(Http_method nMethod, char *pBuf, 
								int *pLen, char *strHostName, 
								short nHostPort, int nContentLen, void *pProxySetting /* = NULL */)
{
	int n = 0;
	int m = 0;
	char str[512]  = {0};
	char auth[256] = {0};
	struct ProxySetting* pProxy = (struct ProxySetting*)pProxySetting;
	
	if (pProxy != NULL) {
		CInetAddr addr(pProxy->ProxyAddr, pProxy->ProxyPort);
		n = sprintf (str, "http://%s:%d", addr.GetHostAddr(), addr.GetPort());
	}
	
	n += sprintf (str + n, "/index.html?crap=%u", (unsigned long)((CTimeValue::GetTimeOfDay()).GetSec()));
	
	m = sprintf (pBuf, "%s %s HTTP/%d.%d\r\n",
	       HttpMethodToString (nMethod),
		   str,
		   1, /*Major version*/
		   1); /*Minor version*/
	
	m += sprintf (pBuf+m, "Host: %s:%d\r\n", strHostName, nHostPort);
	
	if (nContentLen >= 0) {
		m += sprintf (pBuf+m, "Content-Length: 9999999999999999\r\n", nContentLen);
    }
	
	m += sprintf(pBuf+m, "Connection: close\r\n");
	if (pProxy != NULL && pProxy->UserName[0] != '\0') {
		sprintf(auth, "%s:%s", pProxy->UserName, pProxy->Password);
		encode_base64(auth, strlen(auth), str, 512);
		m += sprintf(pBuf+m, "Proxy-Authorization: Basic %s\r\n", str);
	}
	m += sprintf(pBuf+m, "\r\n");
	
	*pLen = m;
	return m;
}

/*
 *	return value:
 *  n>0: Ok, n bytes used 
 *  0: Packet incomplete
 *  -1: parse error
 */
int CHTTPRequest::ParseHttpRequest(char *pBuf, int nLen, int *pMethod)
{
	char *p, *p1;
	int method, retlen;
	int major_version, minor_version;

	if ((p1 = strstr(pBuf, "\r\n\r\n")) == NULL)
		return 0;
	retlen = (p1 - pBuf) + 4;
	p1 = strstr(pBuf, " ");
	if (p1 == NULL)
    {
		return -1;
    }
	method = HttpStringToMethod (pBuf);
	if (method == -1)
		return -1;
	p = p1 + 1;
	*pMethod = method;
	
	p1 = strstr (p, " ");
	if (p1 == NULL)
		return -1;
	p = p1 +1;
	
	p1 = strstr (p, "/");
	if (p1 == NULL)
		return -1;
	else if (p1 - p != 4 || memcmp (p, "HTTP", 4) != 0)
  		return -1;
	p = p1 + 1;
	
	p1 = strstr(p, ".");
	if (p1 == NULL)
		return -1;
	*p1 = 0;
	major_version = atoi (p);
	if (major_version != 1)
		return -1;
	p = p1 + 1;
	
	p1 = strstr(p, "\r");
	if (p1 == NULL)
		return -1;
	*p1 = 0;
	minor_version = atoi (p);
	if (minor_version != 1 && minor_version != 0)
		return -1;
	p = p1 + 1;
	
	return retlen;
}

/*
 *	return value:
 *  n>0: Ok, n bytes used 
 *  0: Packet incomplete
 *  -1: parse error
 */
int CHTTPRequest::ParseHttpResponse(char *pBuf, int nLen)
{
	char *p, *p1;
	int retlen;
	int major_version, minor_version, status_code;

	if ((p1 = strstr(pBuf, "\r\n\r\n")) == NULL)
		return 0;
	retlen = (p1 - pBuf) + 4;
	
	p = strstr (pBuf, "/");
	if (p == NULL)
		return -1;
	else if (p - pBuf != 4 || memcmp (pBuf, "HTTP", 4) != 0)
		return -1;
	p = p+1;
	
	/*
	 *	Major version
	 */
	p1 = strstr (p, ".");
	if (p1 == NULL)
		return -1;
	*p1 = '\0';
	major_version = atoi (p);
	p = p1 + 1;
	if (major_version != 1)
		return -1;

	/*
	 *	Minor Version
	 */
	p1 = strstr(p, " ");
	if (p1 == NULL)
		return -1;
	*p1 = '\0';
	minor_version = atoi (p);
	if(minor_version != 0 && minor_version != 1)
		return -1;
	p = p1 +1;

	p1 = strstr(p, " ");
	if (p1 == NULL)
		return -1;
	*p1 = '\0';
	status_code = atoi (p);
	if (status_code == 407)
		return -407;
	if (status_code != 200)
		return -1;

	return retlen;
}

int CHTTPRequest::BuildHttpResponse(char *pBuf, int *pLen, int nContentLen)
{
	*pLen = sprintf (pBuf,
		"HTTP/1.1 200 OK\r\n"
		"Content-Length: 9999999999999999\r\n"
		"Connection: close\r\n"
		"Pragma: no-cache\r\n"
		"Cache-Control: no-cache, no-store, must-revalidate\r\n"
		"Expires: 0\r\n" /* FIXME: "0" is not a legitimate HTTP date. */
		"Content-Type: text/html\r\n"
		"\r\n");
	
	return *pLen;
}
