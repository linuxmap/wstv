#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>

#include "rs485.h"
#include "libPTZ.h"

int  GetFD485(void);
int  Init485();
int  InitPTZPort(int fd, const NC_PORTPARAMS *param);
int  Open485(void);
void Deinit485(void);
BOOL WriteTo485(void *pdata, int nsize);



////////////////////////////////////////////////////////////////////////////////

static const char *		s_dev_485 = "/dev/ttyAMA1";
static int				s_fd_485 = -1;
static NC_PORTPARAMS	s_serial_param = { 4800, 8,  1, PAR_NONE, PTZ_DATAFLOW_NONE };
static struct termios 	s_tio_old;
static struct termios 	s_tio_curr;

////////////////////////////////////////////////////////////////////////////////

int main()
{
	int nAddr = 1;
	int nProtocal = PTZ_PROTOCOL_YAAN;
	//unsigned char cCmdBuf[8] = {0xFF, 0x01, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00};
    unsigned char ch[32];
    int ret;

	//初始化485
	s_fd_485 = DecoderOpenCom(s_dev_485);
	if(-1 == s_fd_485)
	{
	    printf("fail to open com: %s\n",s_dev_485);
	    return -1;
	}

	ret = DecoderSetCom(s_fd_485, &s_serial_param);
    if (-1 == ret)
    { 
	    printf("set com err\n");
	    return -1;
    }
	
    printf("Default PTZ Addr = 1\n");
   	printf("\t0 --> reset\n\t1 --> move\n\t2 --> zoom\n\t3 --> focus\n\t4 --> Iris\n\t5 --> aux\n\t6 --> present\n\t7 --> petrol\n\tq --> exit\n");
    
    while(1)
    {
        fgets(ch, 32, stdin);
        //printf("ch[0]=%d,ch[1]=%d\n",ch[0],ch[1]);
		if(0 == ch[0] || '\n' == ch[0])
		{
			continue;
		}
		if('q' == ch[0])
		{
		    break;
		}
		if('0' == ch[0])
		{
		    switch(ch[1])
		    {
	        case '1': 
	            DecoderReset(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderReset \n");
	            break;

	        default:
	            printf("bad input \n");
	            break;
		    }
		}
		else if('1' == ch[0])
		{
		    switch(ch[1])
		    {
	        case '1':
	        	DecoderLeftStart(s_fd_485, nAddr, nProtocal, 0, 150);
			    printf("Send CMD: DecoderLeftStart \n");
	            break;
	        case '2':
	            DecoderLeftStop(s_fd_485, nAddr, nProtocal, 0);
			    printf("Send CMD: DecoderLeftStop \n");
	            break;
	        case '3':
	            DecoderRightStart(s_fd_485, nAddr, nProtocal, 0, 150);
			    printf("Send CMD: DecoderRightStart \n");
	            break;
	        case '4':
	            DecoderRightStop(s_fd_485, nAddr, nProtocal, 0);
			    printf("Send CMD: DecoderRightStop \n");
	            break;
	        case '5':
	            DecoderUpStart(s_fd_485, nAddr, nProtocal, 0, 150);
			    printf("Send CMD: DecoderUpStart \n");
	            break;
	        case '6':
	            DecoderUpStop(s_fd_485, nAddr, nProtocal, 0);
			    printf("Send CMD: DecoderUpStop \n");
	            break;
	        case '7':
	            DecoderDownStart(s_fd_485, nAddr, nProtocal, 0, 150);
			    printf("Send CMD: DecoderDownStart \n");
	            break;
	        case '8':
	            DecoderDownStop(s_fd_485, nAddr, nProtocal, 0);
			    printf("Send CMD: DecoderDownStop \n");
	            break;
	        case '9':
	            DecoderAutoStart(s_fd_485, nAddr, nProtocal, 0, 150);
			    printf("Send CMD: DecoderAutoStart \n");
	            break;
	        case '0':
	            DecoderAutoStop(s_fd_485, nAddr, nProtocal, 0);
			    printf("Send CMD: DecoderAutoStop \n");
	            break;
	        default:
	            printf("bad input \n");
	            break;
		    }
		}
		else if('2' == ch[0])
		{
		    switch(ch[1])
		    {
	        case '1':
	            DecoderZoomInStart(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderZoomInStart \n");
	            break;
	        case '2':
	            DecoderZoomInStop(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderZoomInStop \n");
	            break;
	        case '3':
	            DecoderZoomOutStart(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderZoomOutStart \n");
	            break;
	        case '4':
	            DecoderZoomOutStop(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderZoomOutStop \n");
	            break;
	        default:
	            printf("bad input \n");
	            break;
		    }
		}
		else if('3' == ch[0])
		{
		    switch(ch[1])
		    {
	        case '1':
	            DecoderFocusNearStart(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderFocusNearStart \n");
	            break;
	        case '2':
	            DecoderFocusNearStop(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderFocusNearStop \n");
	            break;
	        case '3':
	            DecoderFocusFarStart(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderFocusFarStart \n");
	            break;
	        case '4':
	            DecoderFocusFarStop(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderFocusFarStop \n");
	            break;
	        default:
	            printf("bad input \n");
	            break;
		    }
		}
		else if('4' == ch[0])
		{
		    switch(ch[1])
		    {
	        case '1':
	            DecoderIrisOpenStart(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderIrisOpenStart \n");
	            break;
	        case '2':
	            DecoderIrisOpenStop(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderIrisOpenStop \n");
	            break;
	        case '3':
	            DecoderIrisCloseStart(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderIrisCloseStart \n");
	            break;
	        case '4':
	            DecoderIrisCloseStop(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderIrisCloseStop \n");
	            break;
	        default:
	            printf("bad input \n");
	            break;
		    }
		}
		else if('5' == ch[0])
		{
		    switch(ch[1])
		    {
	        case '1':
	            DecoderAUX1On(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderAUX1On \n");
	            break;
	        case '2':
	            DecoderAUX1Off(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderAUX1Off \n");
	            break;
	        case '3':
	            DecoderAUX2On(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderAUX2On \n");
	            break;
	        case '4':
	            DecoderAUX2Off(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderAUX2Off \n");
	            break;
	        case '5':
	            DecoderAUX3On(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderAUX3On \n");
	            break;
	        case '6':
	            DecoderAUX3Off(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderAUX3Off \n");
	            break;
	        case '7':
	            DecoderAUX4On(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderAUX4On \n");
	            break;
	        case '8':
	            DecoderAUX4Off(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderAUX4Off \n");
	            break;
	        default:
	            printf("bad input \n");
	            break;
		    }
		}
		else if('6' == ch[0])
		{
		    switch(ch[1])
		    {
	        case '1':
	            DecoderSetLeftLimitPosition(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderSetLeftLimitPosition \n");
	            break;
	        case '2':
	            DecoderSetRightLimitPosition(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderSetRightLimitPosition \n");
	            break;
	        case '3':
	            DecoderSetPreset(s_fd_485, nAddr, nProtocal, 0, ch[2]-'0');
	            printf("Send CMD: DecoderSetPreset :%d\n",ch[2]-'0');
	            break;
	        case '4':
	            DecoderClearPreset(s_fd_485, nAddr, nProtocal, 0, ch[2]-'0');
	            printf("Send CMD: DecoderClearPreset :%d\n",ch[2]-'0');
	            break;
	        case '5':
	            DecoderClearAllPreset(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderClearAllPreset \n");
	            break;
	        case '6':
	            DecoderLocatePreset(s_fd_485, nAddr, nProtocal, 0, ch[2]-'0', 0);
	            printf("Send CMD: DecoderLocatePreset :%d\n",ch[2]-'0');
	            break;
	        case '7':
	            DecoderStartPatrol(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderStartPatrol \n");
	            break;
	        case '8':
	            DecoderStopPatrol(s_fd_485, nAddr, nProtocal, 0);
	            printf("Send CMD: DecoderStopPatrol \n");
	            break;
	        default:
	            printf("bad input \n");
	            break;
		    }
		}
		else if('7' == ch[0])
		{
		    switch(ch[1])
		    {
	        case '1':
	            DecoderSetScanOnPreset(s_fd_485, nAddr, nProtocal, 0, ch[2]);
	            printf("Send CMD: DecoderSetScanOnPreset :%d\n",ch[2]-'0');
	            break;
	        case '2':
	            DecoderSetScanOffPreset(s_fd_485, nAddr, nProtocal, 0, ch[2]);
	            printf("Send CMD: DecoderSetScanOffPreset :%d\n",ch[2]-'0');
	            break;
	        case '3':
	            DecoderLocateScanPreset(s_fd_485, nAddr, nProtocal, 0, ch[2]);
	            printf("Send CMD: DecoderLocateScanPreset :%d\n",ch[2]-'0');
	            break;
	        case '4':
	            DecoderStopScanPreset(s_fd_485, nAddr, nProtocal, 0, ch[2]);
	            printf("Send CMD: DecoderStopScanPreset :%d\n",ch[2]-'0');
	            break;
	        default:
	            printf("bad input \n");
	            break;
		    }
		}
		else if('8' == ch[0])
		{
		    switch(ch[1])
		    {
	        case '1':
	            printf("Send CMD: DecoderSetPatrolOn \n");
	            DecoderSetPatrolOn(s_fd_485, nAddr, nProtocal, 0);
	            DecoderAddPatrol(s_fd_485, nAddr, nProtocal,0,1,32,2);
	            DecoderAddPatrol(s_fd_485, nAddr, nProtocal,0,2,32,2);
	            DecoderAddPatrol(s_fd_485, nAddr, nProtocal,0,3,32,2);
	            DecoderSetPatrolOff(s_fd_485, nAddr, nProtocal, 0);
	            break;
	            
	        case '2':
	            printf("Send CMD: DecoderSetPatrolOn \n");
	            DecoderSetPatrolOn(s_fd_485, nAddr, nProtocal, 0);
	            DecoderAddPatrol(s_fd_485, nAddr, nProtocal,0,1,32,2);
	            DecoderAddPatrol(s_fd_485, nAddr, nProtocal,0,2,32,2);
	            DecoderSetPatrolOff(s_fd_485, nAddr, nProtocal, 0);
	            break;
	            
	        case '9':
	            printf("Send CMD: DecoderStartHWPatrol \n");
	            DecoderStartHWPatrol(s_fd_485, nAddr, nProtocal, 0);
	            break;
	            
	        case '0':
	            printf("Send CMD: DecoderStopHWPatrol \n");
	            DecoderStopHWPatrol(s_fd_485, nAddr, nProtocal, 0);
	            break;
	            

	        default:
	            printf("bad input \n");
	            break;
		    }
		}
    	printf("\t0 --> reset\n\t1 --> move\n\t2 --> zoom\n\t3 --> focus\n\t4 --> Iris\n\t5 --> aux\n\t6 --> present\n\t7 --> petrol\n\tq --> exit\n");
	}
	
	DecoderCloseCom(s_fd_485);
	
	return 0;
}


int GetFD485( void )
{
	return s_fd_485;
}

int Init485()
{
	if( (s_fd_485 = open(s_dev_485, O_RDWR | O_NOCTTY)) < 0 )
	{
		printf("ERROR. Open RS-485 device...\n");		
		return -1;
	}
	else
		printf("OK. Open RS-485 device...\n");

	if( InitPTZPort(s_fd_485, &s_serial_param) < 0 )
	{
		printf("Error. Init RS-485 device...\n");
		Deinit485();
		return -1;
	}
	else
		printf("OK. Init RS-485 device...\n");

	return 0;
}

void Deinit485( void )
{
	if( s_fd_485 > 0 )
	{
		if( tcsetattr(s_fd_485, TCSANOW, &s_tio_old) < 0 )
			printf("ERROR. Restore s_tio_curr value...\n");
	}
	
	close( s_fd_485 );
	s_fd_485 = -1;
}

int InitPTZPort( int fd, const NC_PORTPARAMS *param )
{
	int ret = -1;

	memset( &s_tio_curr,0, sizeof(s_tio_curr) );

	if( tcgetattr(fd, &s_tio_curr) < 0 )
	{
		printf("tcgetattr");		
		goto done;
	}

	memcpy( &s_tio_old, &s_tio_curr, sizeof(s_tio_curr) );

	if( param->nBaudRate > 0 )
	{
		// Baudrate
		cfsetispeed( &s_tio_curr, param->nBaudRate );
		cfsetospeed( &s_tio_curr, param->nBaudRate );
		printf("nBaudRate=%d\n", param->nBaudRate);
	}
	else
	{
		printf("Invalid BaudRate Value : %d\n", param->nBaudRate );
		return ret;
	}

	// Character size
	s_tio_curr.c_cflag &= ~CSIZE;
	switch( param->nCharSize )
	{
		case 5	:
			s_tio_curr.c_cflag |= CS5;
			break;
		case 6	:
			s_tio_curr.c_cflag |= CS6;
			break;
		case 7	: 
			s_tio_curr.c_cflag |= CS7;
			break;
		case 8	: 	
		default	:
			s_tio_curr.c_cflag |= CS8;
			break;
	}

	// Parity bit
	switch( param->nParityBit )
	{
		case PAR_NONE	:
			s_tio_curr.c_cflag &= ~PARENB;
			break;
		case PAR_EVEN	:	
			s_tio_curr.c_cflag |= PARENB;	
			s_tio_curr.c_cflag &= ~PARODD;
			break;
		case PAR_ODD	:	
			s_tio_curr.c_cflag |= PARENB;	
			s_tio_curr.c_cflag |= PARODD;	
			break;
	}

	// Stop bit
	if( param->nStopBit == 2 )
		s_tio_curr.c_cflag |= CSTOPB;		// 2 Stop Bit
	else
		s_tio_curr.c_cflag &= ~CSTOPB;		// 1 Stop Bit

	s_tio_curr.c_cflag &= ~CRTSCTS;
	s_tio_curr.c_cflag |= (CLOCAL | CREAD);

	s_tio_curr.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
	s_tio_curr.c_lflag |= FLUSHO;	// Output flush
	
	s_tio_curr.c_oflag &= ~OPOST;
	s_tio_curr.c_cc[VMIN] = 1;
	s_tio_curr.c_cc[VTIME] = 0;
	s_tio_curr.c_iflag = IGNBRK | IGNPAR;

	ret = tcsetattr( fd, TCSANOW, &s_tio_curr );
	if( ret != 0 )
		printf( "tcsetattr\n" );
	
done:

	return ret;
}

BOOL WriteTo485( void *pdata, int nsize )
{
	if( pdata && nsize > 0 )
	{
		if( write(s_fd_485, pdata, nsize) != nsize )
		{
			printf("ERROR. WRITE\n");
			return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PELCO-D Protocol
/*BOOL ControlPELCO_D(int nAddr, int nControl)
{
	COMMPELCOD	Command;

	Command.Synch	= 0xFF;
	Command.Addr	= nAddr;
	Command.Data1	= 0;//nPTZSpeed;	// pan speed : 0x00 ~ 0x3f
	Command.Data2	= 0;//nPTZSpeed;	// tilt speed : 0x00 ~ 0x3f

	switch (nControl)
	{
		case PANTILT_PAN_LEFT:
			Command.Cmd1	= 0x10;
			Command.Cmd2	= 0x00;		// 自动开始
			break;
		case PANTILT_PAN_RIGHT:
			Command.Cmd1	= 0x00;
			Command.Cmd2	= 0x00;		// 自动结束
			break;
		case PANTILT_TILT_UP:
			Command.Cmd1	= 0x00;
			Command.Cmd2	= 0x08;		// TiltUp
			break;
		case PANTILT_TILT_DOWN:
			Command.Cmd1	= 0x00;
			Command.Cmd2	= 0x10;		// TiltDown
			break;
		case PANTILT_ZOOM_IN:
			Command.Cmd1	= 0x00;
			Command.Cmd2	= 0x20;		// ZoomIn
			break;
		case PANTILT_ZOOM_OUT:
			Command.Cmd1	= 0x00;
			Command.Cmd2	= 0x40;		// ZoomOut
			break;
		case PANTILT_FOCUS_NEAR:
			Command.Cmd1	= 0x01;		// FocusNear
			Command.Cmd2	= 0x00;
			break;
		case PANTILT_FOCUS_FAR:
			Command.Cmd1	= 0x00;		// FocusFar
			Command.Cmd2	= 0x80;
			break;
		case PANTILT_IRIS_OPEN:
			Command.Cmd1	= 0x02;		// IrisOpen
			Command.Cmd2	= 0x00;
			break;
		case PANTILT_IRIS_CLOSE:
			Command.Cmd1	= 0x04;		// IrisClose
			Command.Cmd2	= 0x00;
			break;
		default:
			Command.Cmd1	= 0x00;
			Command.Cmd2	= 0x00;		
			break;
	}

	Command.CheckSum =	Command.Addr + 
						Command.Cmd1 + 
						Command.Cmd2 + 
						Command.Data1 + 
						Command.Data2;

	return WriteTo485( &Command, sizeof(Command) );
}
*/
