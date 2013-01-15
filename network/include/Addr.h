/*------------------------------------------------------*/
/* Internet address                                     */
/*                                                      */
/* Addr.h                                               */
/*                                                      */
/*                                                      */
/* History                                              */
/*                                                      */
/*	11/24/2003	Create                                  */
/*                                                      */
/*------------------------------------------------------*/

#ifndef ADDR_H
#define ADDR_H

class  CInetAddr  
{
public:
	CInetAddr();
	
	/// Creates an <CInetAddr> from a <aPort> and the remote
	/// <aHostName>. The port number is assumed to be in host byte order.
	CInetAddr(LPCSTR aHostName, WORD aPort);
	CInetAddr(unsigned long aHostIP, WORD aPort);

	/**
	* Initializes an <CInetAddr> from the <aAddress>, which can be
	* "ip-number:port-number" (e.g., "tango.cs.wustl.edu:1234" or
	* "128.252.166.57:1234").  If there is no ':' in the <address> it
	* is assumed to be a port number, with the IP address being
	* INADDR_ANY.
	*/
	CInetAddr(LPCSTR aAddress);
	
	~CInetAddr();

	int Set(LPCSTR aHostName, WORD aPort);
	int Set(LPCSTR aAddress);

	int SetPort(WORD aPort);

	/// Compare two addresses for equality.  The addresses are considered
	/// equal if they contain the same IP address and port number.
	BOOL operator == (const CInetAddr &aRight) const;
        bool operator < (const CInetAddr &aRight ) const;

	char *GetHostAddr(){return inet_ntoa(m_SockAddr.sin_addr);};

	WORD GetPort() const 
	{ 
		return ntohs(m_SockAddr.sin_port); 
	}

	DWORD GetSize() const { return sizeof (sockaddr_in); }

	DWORD GetType() const { return m_SockAddr.sin_family; }

	const sockaddr_in* GetPtr() const { return &m_SockAddr; }

private:
	sockaddr_in m_SockAddr;
};

#endif // !ADDR_H

