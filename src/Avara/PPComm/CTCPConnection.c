/*
    Copyright �1996, Juri Munkki
    All rights reserved.

    File: CTCPConnection.c
    Created: Sunday, January 28, 1996, 11:42
    Modified: Sunday, January 28, 1996, 21:56
*/

#include "CTCPConnection.h"
#include "CTCPComm.h"

static	ushort	debugcode = -1;
static	ushort	debugreason = -1;

static pascal void TCPNotify(
	StreamPtr		tcp,
	ushort			code,
	Ptr				user,
	ushort			term,
	ICMPReportPtr	icmp)
{
	debugcode = code;
	debugreason = term;
	((CTCPConnection *)user)->Notify(code, term);
}

static	void	TCPReadComplete(
	TCPiopbPtr		pb)
{
	((CTCPConnection *)pb->csParam.receive.userDataPtr)->ReadComplete(pb);
}

static	void	TCPWriteComplete(
	TCPiopbPtr		pb)
{
	((CTCPConnection *)pb->csParam.send.userDataPtr)->WriteComplete(pb);
}

static	void	TCPPassiveComplete(
	TCPiopbPtr		pb)
{
	((CTCPConnection *)pb->csParam.open.userDataPtr)->PassiveComplete(pb);

}

void	CTCPConnection::ReadComplete(
	TCPiopbPtr		pb)
{
}

void	CTCPConnection::WriteComplete(
	TCPiopbPtr		pb)
{
}

void	CTCPConnection::PassiveComplete(
	TCPiopbPtr		pb)
{
}

void	CTCPConnection::Notify(
	ushort			code,
	ushort			termReason)
{
	switch(code)
	{
		case TCPDataArrival:
		case TCPUrgent:
			break;
		case TCPClosing:
			break;
		case TCPULPTimeout:
		case TCPICMPReceived:
			break;
		case TCPTerminate:
			switch(termReason)
			{	case TCPRemoteAbort:
				case TCPNetworkFailure:
				case TCPSecPrecMismatch:
				case TCPULPTimeoutTerminate:
				case TCPULPAbort:
				case TCPULPClose:
				case TCPServiceError:
					break;
			}
			break;
	}
}


void	CTCPConnection::ITCPConnection(
	CTCPComm	*theOwner)
{
	OSErr err=noErr;

	ICommManager(0);

	itsOwner = theOwner;
	myId = -1;	//	Unknown
	
	notify=NewTCPNotifyProc(TCPNotify);
	
	param.ioCRefNum = gMacTCP;
	param.csParam.create.rcvBuff = streamBuffer;
	param.csParam.create.rcvBuffLen = TCPSTREAMBUFFERSIZE;
	param.csParam.create.notifyProc = notify;
	param.csParam.create.userDataPtr = (Ptr) this;
	
	err = TCPCreate(&param,false); // create the stream...
	
	if(err)	stream = 0;
	else	stream = param.tcpStream;
	
	passiveCompletionProc = NewTCPIOCompletionProc(TCPPassiveComplete);
	writeCompletionProc = NewTCPIOCompletionProc(TCPWriteComplete);
	readCompletionProc = NewTCPIOCompletionProc(TCPWriteComplete);
	
	isListening = false;
	isClosing = false;
}

void	CTCPConnection::Dispose()
{
	isClosing = true;

	if(notify)
		DisposeTCPNotifyProc(notify);
	
	if(stream)
	{
		param.tcpStream = stream;
		if(isListening)
		{	TCPAbort(&param, false);
		}

		TCPRelease(&param,false);
	}
	
	DisposeTCPIOCompletionProc(passiveCompletionProc);
	DisposeTCPIOCompletionProc(writeCompletionProc);
	DisposeTCPIOCompletionProc(readCompletionProc);

	inherited::Dispose();
}

OSErr	CTCPConnection::PassiveOpenAll(
	short		localPort)
{
	OSErr	err;

	param.ioCompletion = passiveCompletionProc;
	param.tcpStream = stream;
	param.csParam.open.ulpTimeoutValue = 0;
	param.csParam.open.ulpTimeoutAction = 0;
	param.csParam.open.commandTimeoutValue = 0;

	param.csParam.open.remoteHost = 0;
	param.csParam.open.remotePort = 0;

	param.csParam.open.localHost = 0;
	param.csParam.open.localPort = localPort;

	param.csParam.open.validityFlags = 0;
	param.csParam.open.tosFlags = 0;
	param.csParam.open.precedence = 0;
	param.csParam.open.dontFrag = false;
	param.csParam.open.security = 0;
	param.csParam.open.optionCnt = 0;
	param.csParam.open.timeToLive = 255;
	param.csParam.open.userDataPtr = (Ptr) this;
	
	err = TCPPassiveOpen(&param,true);
	isListening = true;
	
	if(next)
		PassiveOpenAll(localPort);
}

OSErr	CTCPConnection::ContactServer(
	ip_addr		addr,
	short		port,
	StringPtr	password)
{
	OSErr	err;

	param.ioCompletion = 0L;
	param.tcpStream = stream;
	param.csParam.open.ulpTimeoutValue = 0;
	param.csParam.open.ulpTimeoutAction = 0;

	param.csParam.open.remoteHost = addr;
	param.csParam.open.remotePort = port;

	param.csParam.open.localHost = 0;
	param.csParam.open.localPort = 0;

	param.csParam.open.validityFlags = 0;
	param.csParam.open.tosFlags = 0;
	param.csParam.open.precedence = 0;
	param.csParam.open.dontFrag = false;
	param.csParam.open.security = 0;
	param.csParam.open.optionCnt = 0;
	param.csParam.open.timeToLive = 255;
	
	err = TCPActiveOpen(&param,false);
	
	if(err == noErr)
	{	
		wdsEntry	wd[2];
		
		if(next)
			next->PassiveOpenAll(param.csParam.open.localPort);
		
		wd[0].length = password[0];
		wd[0].ptr = (Ptr)password+1;
		
		wd[1].length = 0;
		wd[1].ptr = 0;

		param.ioCompletion = 0L;
		param.tcpStream = stream;
		param.csParam.send.ulpTimeoutValue = 0;
		param.csParam.send.ulpTimeoutAction = 0;
		param.csParam.send.validityFlags = 0;
		param.csParam.send.pushFlag = true;
		param.csParam.send.urgentFlag = false;
		param.csParam.send.wdsPtr = wd;
		
		err = TCPSend(&param, false); // do it synchronously...
		
		if(err == noErr)
		{
			param.tcpStream = stream;
			param.ioCRefNum = gMacTCP;
			param.csParam.close.ulpTimeoutValue = 0;
			param.csParam.close.ulpTimeoutAction = 0;
			param.csParam.close.validityFlags = 0;
			
			err = TCPClose(&param, false);
		}
	}
	
	return err;
}

/*
**	Borrow a packet from the owner, since we have none.
*/
PacketInfo *	CTCPConnection::GetPacket()
{
	return itsOwner->GetPacket();
}

/*
**	Release the packet to the server, since we shouldn't have any.
*/
void	CTCPConnection::ReleasePacket(
	PacketInfo *thePacket)
{
	itsOwner->ReleasePacket(thePacket);
}

/*
**	Let the owner do all the packet dispatching.
*/
void	CTCPConnection::DispatchPacket(
	PacketInfo	*thePacket)
{
	itsOwner->DispatchPacket(thePacket);
}

