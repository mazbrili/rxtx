/*-------------------------------------------------------------------------
|   rxtx is a native interface to serial ports in java.
|   Copyright 1997, 1998, 1999 by Trent Jarvi jarvi@ezlink.com.
|
|   This library is free software; you can redistribute it and/or
|   modify it under the terms of the GNU Library General Public
|   License as published by the Free Software Foundation; either
|   version 2 of the License, or (at your option) any later version.
|
|   This library is distributed in the hope that it will be useful,
|   but WITHOUT ANY WARRANTY; without even the implied warranty of
|   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
|   Library General Public License for more details.
|
|   You should have received a copy of the GNU Library General Public
|   License along with this library; if not, write to the Free
|   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
--------------------------------------------------------------------------*/
#include "config.h"
#include "gnu_io_RXTXPort.h"
#include <time.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include <sys/param.h>
#include <sys/time.h>
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_SYS_FCNTL_H
#   include <sys/fcntl.h>
#endif
#ifdef HAVE_SYS_FILE_H
#   include <sys/file.h>
#endif
#ifdef HAVE_SYS_SIGNAL_H
#   include <sys/signal.h>
#endif

#if defined(__linux__)
#	include <linux/serial.h>
#endif

extern int errno;

#include "SerialImp.h"

/*----------------------------------------------------------
RXTXPort.Initialize

   accept:      none
   perform:     Initialize the native library
   return:      none
----------------------------------------------------------*/ 
JNIEXPORT void JNICALL Java_gnu_io_RXTXPort_Initialize( JNIEnv *env,
	jclass jclazz )
{
	/* This bit of code checks to see if there is a signal handler installed
	   for SIGIO, and installs SIG_IGN if there is not.  This is necessary
	   for the native threads jdk, but we don't want to do it with green
	   threads, because it slows things down.  Go figure. */
	struct sigaction handler;
	sigaction( SIGIO, NULL, &handler );
	if( !handler.sa_handler ) signal( SIGIO, SIG_IGN );
}


/*----------------------------------------------------------
RXTXPort.open

   accept:      The device to open.  ie "/dev/ttyS0"
   perform:     open the device, set the termios struct to sane settings and 
                return the filedescriptor
   return:      fd
   exceptions:  IOExcepiton
   comments:    Very often people complain about not being able to get past
                this function and it turns out to be permissions on the 
                device file or bios has the device disabled.
----------------------------------------------------------*/ 
JNIEXPORT jint JNICALL Java_gnu_io_RXTXPort_open( JNIEnv *env, jobject jobj,
	jstring jstr )
{
	struct termios ttyset;
	int fd;
	const char *filename = (*env)->GetStringUTFChars( env, jstr, 0 );

	do {
		fd=open (filename, O_RDWR | O_NOCTTY | O_NONBLOCK );
	}  while (fd < 0 && errno==EINTR);
	(*env)->ReleaseStringUTFChars( env, jstr, NULL );
	if( fd < 0 ) goto fail;

	ttyset.c_iflag = INPCK;
	ttyset.c_lflag = 0;
	ttyset.c_oflag = 0;
	ttyset.c_cflag = CREAD | CS8;
	ttyset.c_cc[ VMIN ] = 0;
	ttyset.c_cc[ VTIME ] = 1;

#ifdef __FreeBSD__
	if( cfsetspeed( &ttyset, B9600 ) < 0 ) goto fail;
#else
	if( cfsetispeed( &ttyset, B9600 ) < 0 ) goto fail;
	if( cfsetospeed( &ttyset, B9600 ) < 0 ) goto fail;
#endif
	if( tcsetattr( fd, TCSAFLUSH, &ttyset ) < 0 ) goto fail;

	fcntl( fd, F_SETOWN, getpid() );
	fcntl( fd, F_SETFL, FASYNC );

	return (jint)fd;

fail:
	throw_java_exception( env, IO_EXCEPTION, "open", strerror( errno ) );
	return -1;
}


/*----------------------------------------------------------
RXTXPort.nativeClose

   accept:      none
   perform:     get the fd from the java end and close it
   return:      none
   exceptions:  none
----------------------------------------------------------*/ 
JNIEXPORT void JNICALL Java_gnu_io_RXTXPort_nativeClose( JNIEnv *env,
	jobject jobj )
{
	int result;
	int fd = get_java_fd( env, jobj );

	do {
		result=close (fd);
	}  while (result < 0 && errno==EINTR);
	return;
}


/*----------------------------------------------------------
 RXTXPort.nativeSetSerialPortParams

   accept:     speed, data bits, stop bits, parity
   perform:    set the serial port parameters
   return:     void
   exceptions: UnsupportedCommOperationException
----------------------------------------------------------*/ 
JNIEXPORT void JNICALL Java_gnu_io_RXTXPort_nativeSetSerialPortParams(
	JNIEnv *env, jobject jobj, jint speed, jint dataBits, jint stopBits,
	jint parity )
{
	struct termios ttyset;
	int fd = get_java_fd( env, jobj );
	int cspeed = translate_speed( env, speed );
	if( !cspeed ) return;
	if( tcgetattr( fd, &ttyset ) < 0 ) goto fail;
	if( !translate_data_bits( env, &(ttyset.c_cflag), dataBits ) ) return;
	if( !translate_stop_bits( env, &(ttyset.c_cflag), stopBits ) ) return;
	if( !translate_parity( env, &(ttyset.c_cflag), parity ) ) return;
#ifdef __FreeBSD__
	if( cfsetspeed( &ttyset, cspeed ) < 0 ) goto fail;
#else
	if( cfsetispeed( &ttyset, cspeed ) < 0 ) goto fail;
	if( cfsetospeed( &ttyset, cspeed ) < 0 ) goto fail;
#endif
	if( tcsetattr( fd, TCSAFLUSH, &ttyset ) < 0 ) goto fail;
	return;

fail:
	throw_java_exception( env, UNSUPPORTED_COMM_OPERATION,
		"nativeSetSerialPortParams", strerror( errno ) );
}


/*----------------------------------------------------------
 translate_speed

   accept:     speed in bits-per-second
   perform:    convert bits-per-second to a speed_t constant
   return:     speed_t constant
   exceptions: UnsupportedCommOperationException
   comments:   Only the lowest level code should know about
               the magic constants.
----------------------------------------------------------*/ 
int translate_speed( JNIEnv *env, jint speed )
{
	switch( speed ) {
		case 0:		return B0;
		case 50:		return B50;
		case 75:		return B75;
		case 110:	return B110;
		case 134:	return B134;
		case 150:	return B150;
		case 200:	return B200;
		case 300:	return B300;
		case 600:	return B600;
		case 1200:	return B1200;
		case 1800:	return B1800;
		case 2400:	return B2400;
		case 4800:	return B4800;
		case 9600:	return B9600;
		case 19200:	return B19200;
		case 38400:	return B38400;
		case 57600:	return B57600;
		case 115200:	return B115200;
		case 230400:	return B230400;
		case 460800:	return B460800;
	}

	throw_java_exception( env, UNSUPPORTED_COMM_OPERATION,
		"translate_speed", "speed" );
	return 0;
}


/*----------------------------------------------------------
 translate_data_bits

   accept:     javax.comm.SerialPort.DATABITS_* constant
   perform:    set proper termios c_cflag bits
   return:     1 if successful
					0 if an exception is thrown
   exceptions: UnsupportedCommOperationException
----------------------------------------------------------*/ 
int translate_data_bits( JNIEnv *env, int *cflag, jint dataBits )
{
	int temp = (*cflag) & ~CSIZE;

	switch( dataBits ) {
		case DATABITS_5:
			(*cflag) = temp | CS5;
			return 1;
		case DATABITS_6:
			(*cflag) = temp | CS6;
			return 1;
		case DATABITS_7:
			(*cflag) = temp | CS7;
			return 1;
		case DATABITS_8:
			(*cflag) = temp | CS8;
			return 1;
	}

	throw_java_exception( env, UNSUPPORTED_COMM_OPERATION,
		"translate_data_bits", "data bits" );
	return 0;
}


/*----------------------------------------------------------
 translate_stop_bits

   accept:     javax.comm.SerialPort.STOPBITS_* constant
   perform:    set proper termios c_cflag bits
   return:     1 if successful
					0 if an exception is thrown
   exceptions: UnsupportedCommOperationException
   comments:   If you specify 5 data bits and 2 stop bits, the port will
               allegedly use 1.5 stop bits.  Does anyone care?
----------------------------------------------------------*/ 
int translate_stop_bits( JNIEnv *env, int *cflag, jint stopBits )
{
	switch( stopBits ) {
		case STOPBITS_1:
			(*cflag) &= ~CSTOPB;
			return 1;
		case STOPBITS_2:
			(*cflag) |= CSTOPB;
			return 1;
	}

	throw_java_exception( env, UNSUPPORTED_COMM_OPERATION,
		"translate_stop_bits", "stop bits" );
	return 0;
}


/*----------------------------------------------------------
 translate_parity

   accept:     javax.comm.SerialPort.PARITY_* constant
   perform:    set proper termios c_cflag bits
   return:     1 if successful
               0 if an exception is thrown
   exceptions: UnsupportedCommOperationException
   comments:   The CMSPAR bit should be used for 'mark' and 'space' parity,
               but it's not in glibc's includes.  Oh well, rarely used anyway.
----------------------------------------------------------*/ 
int translate_parity( JNIEnv *env, int *cflag, jint parity )
{
	switch( parity ) {
		case PARITY_NONE:
			(*cflag) &= ~PARENB;
			return 1;
#ifdef CMSPAR
		case PARITY_EVEN:
			(*cflag) |= PARENB & ~PARODD & ~CMSPAR;
			return 1;
		case PARITY_ODD:
			(*cflag) |= PARENB | PARODD & ~CMSPAR;
			return 1;
		case PARITY_MARK:
			(*cflag) |= PARENB | PARODD | CMSPAR;
			return 1;
		case PARITY_SPACE:
			(*cflag) |= PARENB & ~PARODD | CMSPAR;
			return 1;
#else
		case PARITY_EVEN:
			(*cflag) |= PARENB & ~PARODD;
			return 1;
		case PARITY_ODD:
			(*cflag) |= PARENB | PARODD;
			return 1;
#endif
	}

	throw_java_exception( env, UNSUPPORTED_COMM_OPERATION,
		"translate_parity", "parity" );
	return 0;
}


/*----------------------------------------------------------
RXTXPort.writeByte

   accept:      byte to write (passed as int)
   perform:     write a single byte to the port
   return:      none
   exceptions:  IOException
----------------------------------------------------------*/ 
JNIEXPORT void JNICALL Java_gnu_io_RXTXPort_writeByte( JNIEnv *env,
	jobject jobj, jint ji ) 
{
	unsigned char byte = (unsigned char)ji;
	int fd = get_java_fd( env, jobj );
	int result;

	do {
		result=write (fd, &byte, sizeof(unsigned char));
	}  while (result < 0 && errno==EINTR);
	if(result >= 0)
		return;
	throw_java_exception( env, IO_EXCEPTION, "writeByte",
		strerror( errno ) );
}


/*----------------------------------------------------------
RXTXPort.writeArray

   accept:      jbarray: bytes used for writing 
                offset: offset in array to start writing
                count: Number of bytes to write
   perform:     write length bytes of jbarray
   return:      none
   exceptions:  IOException
----------------------------------------------------------*/ 
JNIEXPORT void JNICALL Java_gnu_io_RXTXPort_writeArray( JNIEnv *env,
	jobject jobj, jbyteArray jbarray, jint offset, jint count )
{
	int fd = get_java_fd( env, jobj );
	int result;
	jbyte *body = (*env)->GetByteArrayElements( env, jbarray, 0 );
	unsigned char *bytes = (unsigned char *)malloc( count );
	int i;

	for( i = 0; i < count; i++ ) bytes[ i ] = body[ i + offset ];
	(*env)->ReleaseByteArrayElements( env, jbarray, body, 0 );
	do {
		result=write (fd, bytes, count);
	}  while (result < 0 && errno==EINTR);
	free( bytes );
	if( result < 0 ) throw_java_exception( env, IO_EXCEPTION,
		"writeArray", strerror( errno ) );
}


/*----------------------------------------------------------
RXTXPort.drain

   accept:      none
   perform:     wait until all data is transmitted
   return:      none
   exceptions:  IOException
   comments:    java.io.OutputStream.flush() is equivalent to tcdrain,
                not tcflush, which throws away unsent bytes
----------------------------------------------------------*/
JNIEXPORT void JNICALL Java_gnu_io_RXTXPort_drain( JNIEnv *env,
	jobject jobj )
{
	int fd = get_java_fd( env, jobj );
	int result;

	do {
		result=tcdrain (fd);
	}  while (result && errno==EINTR);

	if( result ) throw_java_exception( env, IO_EXCEPTION, "drain",
		strerror( errno ) );
}


/*----------------------------------------------------------
RXTXPort.sendBreak

   accept:     duration in milliseconds.
   perform:    send break for actual time.  not less than 0.25 seconds.
   exceptions: none
   comments:   not very precise
----------------------------------------------------------*/ 
JNIEXPORT void JNICALL Java_gnu_io_RXTXPort_sendBreak( JNIEnv *env,
	jobject jobj, jint duration )
{
	int fd = get_java_fd( env, jobj );
	tcsendbreak( fd, (int)( duration / 250 ) );
}


/*----------------------------------------------------------
RXTXPort.isDSR

   accept:      none
   perform:     check status of DSR
   return:      true if TIOCM_DSR is set
                false if TIOCM_DSR is not set
   exceptions:  none
   comments:    DSR stands for Data Set Ready
----------------------------------------------------------*/
JNIEXPORT jboolean JNICALL Java_gnu_io_RXTXPort_isDSR( JNIEnv *env,
	jobject jobj ) 
{
	unsigned int result = 0;
	int fd = get_java_fd( env, jobj );

	ioctl( fd, TIOCMGET, &result );
	if( result & TIOCM_DSR ) return JNI_TRUE;
	else return JNI_FALSE;
}


/*----------------------------------------------------------
RXTXPort.isCD

   accept:      none
   perform:     check status of CD
   return:      true if TIOCM_CD is set
                false if TIOCM_CD is not set
   exceptions:  none
   comments:    CD stands for Carrier Detect

"*pp* well, it works, there might ofcourse be a bug, but making DCD permanently
+on fixed it for me so I don't care"

----------------------------------------------------------*/
JNIEXPORT jboolean JNICALL Java_gnu_io_RXTXPort_isCD( JNIEnv *env,
	jobject jobj )
{
	unsigned int result = 0;
	int fd = get_java_fd( env, jobj );

	ioctl( fd, TIOCMGET, &result );
	if( result & TIOCM_CD ) return JNI_TRUE;
	else return JNI_FALSE;
}


/*----------------------------------------------------------
RXTXPort.isCTS

   accept:      none
   perform:     check status of CTS
   return:      true if TIOCM_CTS is set
                false if TIOCM_CTS is not set
   exceptions:  none
   comments:    CTS stands for Clear To Send.
----------------------------------------------------------*/
JNIEXPORT jboolean JNICALL Java_gnu_io_RXTXPort_isCTS( JNIEnv *env,
	jobject jobj ) 
{
	unsigned int result = 0;
	int fd = get_java_fd( env, jobj );

	ioctl( fd, TIOCMGET, &result );
	if( result & TIOCM_CTS ) return JNI_TRUE;
	else return JNI_FALSE;
}


/*----------------------------------------------------------
RXTXPort.isRI

   accept:      none
   perform:     check status of RI
   return:      true if TIOCM_RI is set
                false if TIOCM_RI is not set
   exceptions:  none
   comments:    RI stands for Ring Indicator
----------------------------------------------------------*/
JNIEXPORT jboolean JNICALL Java_gnu_io_RXTXPort_isRI( JNIEnv *env,
	jobject jobj )
{
	unsigned int result = 0;
	int fd = get_java_fd( env, jobj );

	ioctl( fd, TIOCMGET, &result );
	if( result & TIOCM_RI ) return JNI_TRUE;
	else return JNI_FALSE;
}


/*----------------------------------------------------------
RXTXPort.isRTS

   accept:      none
   perform:     check status of RTS
   return:      true if TIOCM_RTS is set
                false if TIOCM_RTS is not set
   exceptions:  none
   comments:    tcgetattr with c_cflag CRTS_IFLOW
----------------------------------------------------------*/
JNIEXPORT jboolean JNICALL Java_gnu_io_RXTXPort_isRTS( JNIEnv *env,
	jobject jobj )
{
	unsigned int result = 0;
	int fd = get_java_fd( env, jobj );

	ioctl( fd, TIOCMGET, &result );
	if( result & TIOCM_RTS ) return JNI_TRUE;
	else return JNI_FALSE;
}


/*----------------------------------------------------------
RXTXPort.setRTS

   accept:      state  flag to set/unset.
   perform:     depends on the state flag
                if true TIOCM_RTS is set
                if false TIOCM_RTS is unset
   return:      none
   exceptions:  none
   comments:    tcsetattr with c_cflag CRTS_IFLOW
----------------------------------------------------------*/
JNIEXPORT void JNICALL Java_gnu_io_RXTXPort_setRTS( JNIEnv *env,
	jobject jobj, jboolean state ) 
{
	unsigned int result = 0;
	int fd = get_java_fd( env, jobj );

	ioctl( fd, TIOCMGET, &result );
	if( state == JNI_TRUE ) result |= TIOCM_RTS;
	else result &= ~TIOCM_RTS;
	ioctl( fd, TIOCMSET, &result );
	return;
}
/*----------------------------------------------------------
RXTXPort.setDSR

   accept:      state  flag to set/unset.
   perform:     depends on the state flag
                if true TIOCM_DSR is set
                if false TIOCM_DSR is unset
   return:      none
   exceptions:  none
   comments:    tcsetattr with c_cflag CRTS_IFLOW
----------------------------------------------------------*/
JNIEXPORT void JNICALL Java_gnu_io_RXTXPort_setDSR( JNIEnv *env,
	jobject jobj, jboolean state ) 
{
	unsigned int result = 0;
	int fd = get_java_fd( env, jobj );

	ioctl( fd, TIOCMGET, &result );
	if( state == JNI_TRUE ) result |= TIOCM_DSR;
	else result &= ~TIOCM_DSR;
	ioctl( fd, TIOCMSET, &result );
	return;
}


/*----------------------------------------------------------
RXTXPort.isDTR

   accept:      none
   perform:     check status of DTR
   return:      true if TIOCM_DTR is set
                false if TIOCM_DTR is not set
   exceptions:  none
   comments:    DTR stands for Data Terminal Ready
----------------------------------------------------------*/
JNIEXPORT jboolean JNICALL Java_gnu_io_RXTXPort_isDTR( JNIEnv *env,
	jobject jobj )
{
	unsigned int result = 0;
	int fd = get_java_fd( env, jobj );

	ioctl( fd, TIOCMGET, &result );
	if( result & TIOCM_DTR ) return JNI_TRUE;
	else return JNI_FALSE;
}


/*----------------------------------------------------------
RXTXPort.setDTR

   accept:      new DTR state
   perform:     if state is true, TIOCM_DTR is set
                if state is false, TIOCM_DTR is unset
   return:      none
   exceptions:  none
   comments:    DTR stands for Data Terminal Ready
----------------------------------------------------------*/
JNIEXPORT void JNICALL Java_gnu_io_RXTXPort_setDTR( JNIEnv *env,
	jobject jobj, jboolean state )
{
	unsigned int result = 0;
	int fd = get_java_fd( env, jobj );

	ioctl( fd, TIOCMGET, &result );
	if( state == JNI_TRUE ) result |= TIOCM_DTR;
	else result &= ~TIOCM_DTR;
	ioctl( fd, TIOCMSET, &result );
	return;
}


/*----------------------------------------------------------
read_byte_array

   accept:      int                fd   file descriptor to read from
                unsigned char *buffer   buffer to read data into
                int            length   number of bytes to read
                int         threshold   receive threshold
                int           timeout   milliseconds to wait before returning
   perform:     read bytes from the port into a buffer
   return:      status of read
                -1 fail (IOException)
                 0 timeout
                >0 number of bytes read
   comments:    According to the Communications API spec, a receive threshold
                of 1 is the same as having the threshold disabled.
----------------------------------------------------------*/ 
int read_byte_array( int fd, unsigned char *buffer, int length, int threshold,
	int timeout )
{
	int ret, left, bytes = 0;
	fd_set rfds;
	struct timeval sleep;

	FD_ZERO( &rfds );
	FD_SET( fd, &rfds );
	sleep.tv_sec = timeout / 1000;
	sleep.tv_usec = 1000 * ( timeout % 1000 );
	left = length;

	while( bytes < length && bytes < threshold ) {
		if( timeout > 0 ) {
         /* FIXME: In Linux, select updates the timeout automatically, so
            other OSes will need to update it manually if they want to have
            the same behavior.  For those OSes, timeouts will occur after no
            data AT ALL is received for the timeout duration.  No big deal. */

			do {
				ret=select( fd + 1, &rfds, NULL, NULL, &sleep );
			}  while (ret < 0 && errno==EINTR);
			if( ret == 0 ) break;
			if( ret < 0 ) return -1;
		}
		ret = read( fd, buffer + bytes, left );
		if( ret == 0 ) break;
		if( ret < 0 ) return -1;
		bytes += ret;
		left -= ret;
	}
	return bytes;
}


/*----------------------------------------------------------
RXTXPort.readByte

   accept:      none
   perform:     Read a single byte from the port
   return:      The byte read
   exceptions:  IOException
----------------------------------------------------------*/ 
JNIEXPORT jint JNICALL Java_gnu_io_RXTXPort_readByte( JNIEnv *env,
	jobject jobj )
{ 
	int bytes, fd, timeout;
	unsigned char buffer[ 1 ];
	jfieldID jfield;
	jclass jclazz = (*env)->GetObjectClass( env, jobj );
	jfield = (*env)->GetFieldID( env, jclazz, "fd", "I" );
	fd = (int)( (*env)->GetIntField( env, jobj, jfield ) );
	jfield = (*env)->GetFieldID( env, jclazz, "timeout", "I" );
	timeout = (int)( (*env)->GetIntField( env, jobj, jfield ) );


	bytes = read_byte_array( fd, buffer, 1, 1, timeout );
	if( bytes < 0 ) {
		throw_java_exception( env, IO_EXCEPTION, "readByte",
			strerror( errno ) );
		return -1;
	}
	return (bytes ? (jint)buffer[ 0 ] : -1);
}


/*----------------------------------------------------------
RXTXPort.readArray

   accept:       offset (bytes to skip) and Length (bytes to read)
   perform:      read bytes from the port into a byte array
   return:       bytes read on success
                 0 on read timeout
   exceptions:   IOException
   comments:     throws ArrayIndexOutOfBoundsException if asked to
                 read more than SSIZE_MAX bytes
----------------------------------------------------------*/ 
JNIEXPORT jint JNICALL Java_gnu_io_RXTXPort_readArray( JNIEnv *env,
	jobject jobj, jbyteArray jbarray, jint offset, jint length )
{  
	int bytes, i, fd, threshold, timeout;
	jbyte *body;
	unsigned char *buffer;
	jfieldID jfield;
	jclass jclazz = (*env)->GetObjectClass( env, jobj );
	jfield = (*env)->GetFieldID( env, jclazz, "fd", "I" );
	fd = (int)( (*env)->GetIntField( env, jobj, jfield ) );
	jfield = (*env)->GetFieldID( env, jclazz, "threshold", "I" );
	threshold = (int)( (*env)->GetIntField( env, jobj, jfield ) );
	jfield = (*env)->GetFieldID( env, jclazz, "timeout", "I" );
	timeout = (int)( (*env)->GetIntField( env, jobj, jfield ) );

	if( length == 0 ) return 0;
	if( length > SSIZE_MAX || length < 0 ) {
		throw_java_exception( env, ARRAY_INDEX_OUT_OF_BOUNDS,
			"readArray", "Invalid length" );
		return -1;
	}

	buffer = (unsigned char *)malloc( sizeof( unsigned char ) * length );
	if( buffer == 0 ) {
		throw_java_exception( env, OUT_OF_MEMORY, "readArray",
			"Unable to allocate buffer" );
		return -1;
	}

	bytes = read_byte_array( fd, buffer, length, threshold, timeout );
	if( bytes < 0 ) {
		free( buffer );
		throw_java_exception( env, IO_EXCEPTION, "readArray",
			strerror( errno ) );
		return -1;
	}

	body = (*env)->GetByteArrayElements( env, jbarray, 0 );
	for( i = 0; i < bytes; i++ ) body[ i + offset ] = buffer[ i ];
	(*env)->ReleaseByteArrayElements( env, jbarray, body, 0 );
	free( buffer );
	return (bytes ? bytes : -1);
}


/*----------------------------------------------------------
RXTXPort.nativeavailable

   accept:      none
   perform:     find out the number of bytes available for reading
   return:      available bytes
                -1 on error
   exceptions:  none
----------------------------------------------------------*/ 
JNIEXPORT jint JNICALL Java_gnu_io_RXTXPort_nativeavailable( JNIEnv *env,
	jobject jobj )
{
	int fd = get_java_fd( env, jobj );
	int result;

	if( ioctl( fd, FIONREAD, &result ) ) return -1;
	else return (jint)result;
}


/*----------------------------------------------------------
RXTXPort.setflowcontrol

   accept:      state (JNI_FALSE 0, JNI_TRUE 1)
   perform:     set hardware flow control to state
   return:      none
   exceptions:  IOException
----------------------------------------------------------*/
JNIEXPORT void JNICALL Java_gnu_io_RXTXPort_setflowcontrol( JNIEnv *env,
	jobject jobj, jint flowmode )
{
	struct termios ttyset;
	jfieldID jfield;
	int fd = get_java_fd( env, jobj );
	jint flags=0;

	if( tcgetattr( fd, &ttyset ) ) goto fail;
	
	if ( flowmode & ( FLOWCONTROL_RTSCTS_IN | FLOWCONTROL_RTSCTS_OUT ) )
		ttyset.c_cflag |= HARDWARE_FLOW_CONTROL;
	else ttyset.c_cflag &= ~HARDWARE_FLOW_CONTROL;

	ttyset.c_iflag &= ~IXANY;

	if ( flowmode & FLOWCONTROL_XONXOFF_IN )
		ttyset.c_iflag |= IXOFF;
	else ttyset.c_iflag &= ~IXOFF;

	if ( flowmode & FLOWCONTROL_XONXOFF_OUT )
		ttyset.c_iflag |= IXON;
	else ttyset.c_iflag &= ~IXON;

	if( tcsetattr( fd, TCSAFLUSH, &ttyset ) ) goto fail;
	return;
fail:
	throw_java_exception( env, IO_EXCEPTION, "setHWFC",
		strerror( errno ) );
	return;
}


/*----------------------------------------------------------
RXTXPort.eventLoop

   accept:      none
   perform:     periodically check for SerialPortEvents
   return:      none
   exceptions:  none
----------------------------------------------------------*/ 
JNIEXPORT void JNICALL Java_gnu_io_RXTXPort_eventLoop( JNIEnv *env,
	jobject jobj )
{
	int fd, ret, change;
	unsigned int mflags;
	fd_set rfds;
	struct timeval sleep;
#if defined(__linux__)
	struct serial_icounter_struct sis, osis;
#endif
	jfieldID jfield;
	jmethodID method, interrupt;
	jboolean interrupted = 0;
	jclass jclazz, jthread;
	jclazz = (*env)->GetObjectClass( env, jobj );
	jfield = (*env)->GetFieldID( env, jclazz, "fd", "I" );
	fd = (int)( (*env)->GetIntField( env, jobj, jfield ) );
	method = (*env)->GetMethodID( env, jclazz, "sendEvent", "(IZ)V" );
	jthread = (*env)->FindClass( env, "java/lang/Thread" );
	interrupt = (*env)->GetStaticMethodID( env, jthread, "interrupted", "()Z" );

	/* Some multiport serial cards do not implement TIOCGICOUNT ... */
#if defined(__linux__)
	if( ioctl( fd, TIOCGICOUNT, &osis ) < 0 ) {
		fprintf( stderr, "Port does not support events\n" );
		return;
	}
#else
	fprintf( stderr, "Port does not support events\n" );
	return;
#endif

	FD_ZERO( &rfds );
	while( !interrupted ) {
		FD_SET( fd, &rfds );
		sleep.tv_sec = 1; /* Check every 1 second, or on receive data */
		sleep.tv_usec = 0;
		do {
			ret=select( fd + 1, &rfds, NULL, NULL, &sleep );
		}  while (ret < 0 && errno==EINTR);
		if( ret < 0 ) break;
#if defined(__linux__)
		if( ioctl( fd, TIOCGICOUNT, &sis ) ) break;
#endif
		if( ioctl( fd, TIOCMGET, &mflags ) ) break;
#if defined(FULL_EVENT) && defined(__linux__)
		if( sis.rx != osis.rx ) (*env)->CallVoidMethod( env, jobj, method,
			(jint)SPE_DATA_AVAILABLE, JNI_TRUE );
		while( sis.frame > osis.frame ) {
			(*env)->CallVoidMethod( env, jobj, method, (jint)SPE_FE, JNI_TRUE );
			osis.frame++;
		}
		while( sis.overrun > osis.overrun ) {
			(*env)->CallVoidMethod( env, jobj, method, (jint)SPE_OE, JNI_TRUE );
			osis.overrun++;
		}
		while( sis.parity > osis.parity ) {
			(*env)->CallVoidMethod( env, jobj, method, (jint)SPE_PE, JNI_TRUE );
			osis.parity++;
		}
		while( sis.brk > osis.brk ) {
			(*env)->CallVoidMethod( env, jobj, method, (jint)SPE_BI, JNI_TRUE );
			osis.brk++;
		}
#else
		if( ioctl( fd, FIONREAD, &change ) ) break;
		if( change ) (*env)->CallVoidMethod( env, jobj, method,
			(jint)SPE_DATA_AVAILABLE, JNI_TRUE );
#endif /* FULL_EVENT __linux__ */
		change = sis.cts - osis.cts;
		if( change ) send_modem_events( env, jobj, method, SPE_CTS, abs(change),
			mflags & TIOCM_CTS );
		change = sis.dsr - osis.dsr;
		if( change ) send_modem_events( env, jobj, method, SPE_DSR, abs(change),
			mflags & TIOCM_DSR );
		change = sis.rng - osis.rng;
		if( change ) send_modem_events( env, jobj, method, SPE_RI, change,
			mflags & TIOCM_RNG );
		change = sis.dcd - osis.dcd;
		if( change ) send_modem_events( env, jobj, method, SPE_CD, abs(change),
			mflags & TIOCM_CD );
		osis = sis;
		interrupted = (*env)->CallStaticBooleanMethod( env, jthread, interrupt );
	}
	return;
}


/*----------------------------------------------------------
 send_modem_events

   accept:      int    event     SerialPortEvent constant
                int    change    Number of times this event happened
                int    state     current state: 0 is false, nonzero is true
   perform:     Send the necessary events
   return:      none
   exceptions:  none
   comments:    Since the interrupt counters tell us how many times the
                state has changed, we can send a SerialPortEvent for each
                interrupt (change) that has occured.  If we don't do this,
                we'll miss a whole bunch of events.
----------------------------------------------------------*/ 
void send_modem_events( JNIEnv *env, jobject jobj, jmethodID method,
	int event, int change, int state )
{
	int i, s;
	jboolean flag;
	if( state ) s = 1;
	else s = 0;

	for( i = 0; i < change; i++ ) {
		if( ( change + s + i ) % 2 ) flag = JNI_FALSE;
		else flag = JNI_TRUE;
		(*env)->CallVoidMethod( env, jobj, method, (jint)event, flag );
	}
}


/*----------------------------------------------------------
get_java_fd

   accept:      env (keyhole to java)
                jobj (java RXTXPort object)
   return:      the fd field from the java object
   exceptions:  none
   comments:
----------------------------------------------------------*/ 
int get_java_fd( JNIEnv *env, jobject jobj )
{
	jclass jclazz = (*env)->GetObjectClass( env, jobj );
	jfieldID jfd = (*env)->GetFieldID( env, jclazz, "fd", "I" );
	if( !jfd ) {
		(*env)->ExceptionDescribe( env );
		(*env)->ExceptionClear( env );
		return 0;
	}
	return (int)( (*env)->GetIntField( env, jobj, jfd ) );
}


/*----------------------------------------------------------
throw_java_exception

   accept:      env (keyhole to java)
                *exc (exception class name)
                *foo (function name)
                *msg (error message)
   perform:     Throw a new java exception
   return:      none
   exceptions:  haha!
   comments:
----------------------------------------------------------*/ 
void throw_java_exception( JNIEnv *env, char *exc, char *foo, char *msg )
{
	char buf[ 60 ];
	jclass clazz = (*env)->FindClass( env, exc );
	if( !clazz ) {
		(*env)->ExceptionDescribe( env );
		(*env)->ExceptionClear( env );
		return;
	}
	snprintf( buf, 60, "%s in %s", msg, foo );
	(*env)->ThrowNew( env, clazz, buf );
}
