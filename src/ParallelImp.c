/*-------------------------------------------------------------------------
|   rxtx is a native interface to serial ports in java.
|   Copyright 1997, 1998, 1999 by Trent Jarvi trentjarvi@yahoo.com
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
/* 
   fear he who enter here.  It appears that things have changed.  An attempt
   has been made to put things the way the should be.

	basic problem is LP_P* appears to be ifdefed __KERNEL__ in the
        lp.h header file on linux.  It also looks as if LP_P* should be
        LP_* for POSOX compliance.  So... Some P's got chopped out.
        Its not clear what LP_PACK is supposed to become so its commented out
        below.

        Some errors may have occured during the change.  It compiles and
        ParallelBlackBox runs.  No further garantees.
   
        - Trent Jarvi
*/

#include "config.h"
/* work around for libc5 */
/*#include <typedefs_md.h>*/
#include "gnu_io_LPRPort.h"
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
#	include <termios.h>
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
#	include <linux/lp.h>
#endif
#if defined(__FreeBSD__)
#	include <machine/lpt.h>
#endif

extern int errno;

#include "ParallelImp.h"



/*----------------------------------------------------------
LPRPort.getOutputBufferFree
   accept:    none  
   perform:     
   return:    number of bytes available in buffer.  
   exceptions: none 
   comments:  have not seen how to do this in the kernel yet.  
----------------------------------------------------------*/ 
JNIEXPORT jint JNICALL Java_gnu_io_LPRPort_getOutputBufferFree(JNIEnv *env,
        jclass jclazz) {
	printf("getOutputBufferFree is not implemented yet\n");

	return(0);

}
/*----------------------------------------------------------
LPRPort.setLPRMode
   accept:     mode 
   perform:    set the Printer communication mode 
	LPT_MODE_ANY:     pick the best possible mode
	LPT_MODE_SPP:     compatibility mode/unidirectional
	LPT_MODE_PS2:     byte mode/bidirectional
	LPT_MODE_EPP:     extended parallel port
	LPT_MODE_ECP:     enhanced capabilities port
	LPT_MODE_NIBBLE:  Nibble Mode. Bi-directional. HP Bi-tronics. 
	                  4 bits at a time.
   return:     none 
   exceptions: UnsupportedCommOperationException
   comments:    
----------------------------------------------------------*/ 
JNIEXPORT jboolean JNICALL Java_gnu_io_LPRPort_setLPRMode(JNIEnv *env,
        jclass jclazz, jint mode) {
	switch(mode){
		case LPT_MODE_ANY:
			break;
		case LPT_MODE_SPP:
		case LPT_MODE_PS2:
		case LPT_MODE_EPP:
		case LPT_MODE_ECP:
		case LPT_MODE_NIBBLE:
		default:
			UnsupportedCommOperationException(env, "setLPRMode was unable to proced the requested mode" );

	}

}
/*----------------------------------------------------------
LPRPort.isPaperOut
   accept:      none   
   perform:     check if printer reports paper is out   
   return:      Paper Out: JNI_TRUE  Not Paper Out: JNI_FALSE     
   exceptions:  none
   comments:    LP_NOPA    unchanged out-of-paper input, active high
----------------------------------------------------------*/ 
JNIEXPORT jboolean JNICALL Java_gnu_io_LPRPort_isPaperOut(JNIEnv *env,
	jobject jobj){

	int status;
	int fd = get_java_fd( env, jobj );
#if defined (__linux__)
	ioctl(fd, LPGETSTATUS,&status);
	return( status & LP_NOPA ? JNI_TRUE : JNI_FALSE );
#else
/*  FIXME??  */
	return(JNI_TRUE);
#endif
}
/*----------------------------------------------------------
LPRPort.isPrinterBusy
   accept:      none   
   perform:     Check to see if the printer is printing.   
   return:      JNI_TRUE if the printer is Busy, JNI_FALSE if its idle   
   exceptions:  none
   comments:    LP_BUSY     inverted busy input, active high
----------------------------------------------------------*/ 
JNIEXPORT jboolean JNICALL Java_gnu_io_LPRPort_isPrinterBusy(JNIEnv *env,
	jobject jobj){
	int status;
	int fd = get_java_fd( env, jobj );
#if defined (__linux__)
	ioctl(fd, LPGETSTATUS, &status);
#else
/*  FIXME??  */
#endif
#if defined(__linux__)
	return( status & LP_BUSY ? JNI_TRUE : JNI_FALSE );
#endif
#if defined(__FreeBSD__)
	return( status & EBUSY ? JNI_TRUE : JNI_FALSE );
#endif
	return(JNI_FALSE);
}
/*----------------------------------------------------------
LPRPort.isPrinterError
   accept:      none
   perform:     check for printer error
   return:      JNI_TRUE if there is an printer error otherwise JNI_FALSE
   exceptions:  none
   comments:    LP_ERR   unchanged error input, active low 
----------------------------------------------------------*/ 
JNIEXPORT jboolean JNICALL Java_gnu_io_LPRPort_isPrinterError(JNIEnv *env,
	jobject jobj){
	int status;
	int fd = get_java_fd( env, jobj );
#if defined (__linux__)
	ioctl(fd, LPGETSTATUS, &status);
	return( status & LP_ERR ? JNI_TRUE : JNI_FALSE );
#else
/*  FIXME??  */
	return(JNI_FALSE);
#endif
}
/*----------------------------------------------------------
LPRPort.isPrinterSelected
   accept:      none
   perform:     check if printer is selected
   return:      JNI_TRUE if printer is selected other wise JNI_FALSE
   exceptions:  none
   comments:    LP_SELEC   unchanged selected input, active high 
----------------------------------------------------------*/ 
JNIEXPORT jboolean JNICALL Java_gnu_io_LPRPort_isPrinterSelected(JNIEnv *env,
	jobject jobj){
	int status;
	int fd = get_java_fd( env, jobj );
#if defined (__linux__)
	ioctl(fd, LPGETSTATUS, &status);
	return( status & LP_SELEC ? JNI_TRUE : JNI_FALSE );
#else
/*  FIXME??  */
	return(JNI_FALSE);
#endif
}
/*----------------------------------------------------------
LPRPort.isPrinterTimedOut
   accept:       none
   perform:      Not really sure see isPaperOut
   return:       JNI_FALSE if the printer does not return out of paper other
                 wise JNI_TRUE.
   exceptions:   none
   comments:     Is this documented right in the javadocs?
	         not sure this is correct FIXME
----------------------------------------------------------*/ 
JNIEXPORT jboolean JNICALL Java_gnu_io_LPRPort_isPrinterTimedOut(JNIEnv *env,
	jobject jobj){
	int status;
	int fd = get_java_fd( env, jobj );
#if defined(__linux__)
	ioctl(fd, LPGETSTATUS, &status);
	return( status & LP_BUSY ? JNI_TRUE : JNI_FALSE );
#endif
#if defined(__FreeBSD__)
	return( status & EBUSY ? JNI_TRUE : JNI_FALSE );
#endif
	return( JNI_FALSE );
}


/*----------------------------------------------------------
LPRPort.Initialize

   accept:      none
   perform:     Initialize the native library
   return:      none
   comments:    lots of reading to do here. FIXME
----------------------------------------------------------*/ 
JNIEXPORT void JNICALL Java_gnu_io_LPRPort_Initialize( JNIEnv *env,
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
LPRPort.open

   accept:      The device to open.  ie "/dev/lp0"
   perform:     open the device and return the filedescriptor
   return:      fd
   exceptions:  IOExcepiton
   comments:    Very often people complain about not being able to get past
                this function and it turns out to be permissions on the 
                device file or bios has the device disabled.
----------------------------------------------------------*/ 
JNIEXPORT jint JNICALL Java_gnu_io_LPRPort_open( JNIEnv *env, jobject jobj,
	jstring jstr )
{
	struct termios ttyset;
	const char *filename = (*env)->GetStringUTFChars( env, jstr, 0 );
	int fd = open( filename, O_RDWR | O_NONBLOCK );
	(*env)->ReleaseStringUTFChars( env, jstr, NULL );
	if( fd < 0 ) goto fail;
	return (jint)fd;

fail:
	IOException( env, strerror( errno ) );
	return -1;
}


/*----------------------------------------------------------
LPRPort.nativeClose

   accept:      none
   perform:     get the fd from the java end and close it
   return:      none
   exceptions:  none
----------------------------------------------------------*/ 
JNIEXPORT void JNICALL Java_gnu_io_LPRPort_nativeClose( JNIEnv *env,
	jobject jobj )
{
	int fd = get_java_fd( env, jobj );

	close( fd );
	return;
}

/*----------------------------------------------------------
LPRPort.writeByte

   accept:      byte to write (passed as int)
   perform:     write a single byte to the port
   return:      none
   exceptions:  IOException
----------------------------------------------------------*/ 
JNIEXPORT void JNICALL Java_gnu_io_LPRPort_writeByte( JNIEnv *env,
	jobject jobj, jint ji ) 
{
	unsigned char byte = (unsigned char)ji;
	int fd = get_java_fd( env, jobj );

	if( write( fd, &byte, sizeof( unsigned char ) ) >= 0 ) return;
	IOException( env, strerror( errno ) );
}


/*----------------------------------------------------------
LPRPort.writeArray

   accept:      jbarray: bytes used for writing 
                offset: offset in array to start writing
                count: Number of bytes to write
   perform:     write length bytes of jbarray
   return:      none
   exceptions:  IOException
----------------------------------------------------------*/ 
JNIEXPORT void JNICALL Java_gnu_io_LPRPort_writeArray( JNIEnv *env,
	jobject jobj, jbyteArray jbarray, jint offset, jint count )
{
	int fd = get_java_fd( env, jobj );
	jbyte *body = (*env)->GetByteArrayElements( env, jbarray, 0 );
	unsigned char *bytes = (unsigned char *)malloc( count );
	int i;
	for( i = 0; i < count; i++ ) bytes[ i ] = body[ i + offset ];
	(*env)->ReleaseByteArrayElements( env, jbarray, body, 0 );
	if( write( fd, bytes, count ) < 0 )
		IOException( env, strerror( errno ) );
	free( bytes );
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
			} while(ret < 0 && errno ==EINTR);
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
LPRPort.readByte

   accept:      none
   perform:     Read a single byte from the port
   return:      The byte read
   exceptions:  IOException
----------------------------------------------------------*/ 
JNIEXPORT jint JNICALL Java_gnu_io_LPRPort_readByte( JNIEnv *env,
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
		IOException( env, strerror( errno ) );
		return -1;
	}
	return (bytes ? (jint)buffer[ 0 ] : -1);
}


/*----------------------------------------------------------
LPRPort.readArray

   accept:       offset (bytes to skip) and Length (bytes to read)
   perform:      read bytes from the port into a byte array
   return:       bytes read on success
                 0 on read timeout
   exceptions:   IOException
   comments:     throws IOException if asked to read > SSIZE_MAX
----------------------------------------------------------*/ 
JNIEXPORT jint JNICALL Java_gnu_io_LPRPort_readArray( JNIEnv *env,
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

	if( length < 1 || length > SSIZE_MAX ) {
		IOException( env, "Invalid length" );
		return -1;
	}

	buffer = (unsigned char *)malloc( sizeof( unsigned char ) * length );
	if( buffer == 0 ) {
		IOException( env, "Unable to allocate buffer" );
		return -1;
	}

	bytes = read_byte_array( fd, buffer, length, threshold, timeout );
	if( bytes < 0 ) {
		free( buffer );
		IOException( env, strerror( errno ) );
		return -1;
	}

	body = (*env)->GetByteArrayElements( env, jbarray, 0 );
	for( i = 0; i < bytes; i++ ) body[ i + offset ] = buffer[ i ];
	(*env)->ReleaseByteArrayElements( env, jbarray, body, 0 );
	free( buffer );
	return (bytes ? bytes : -1);
}


/*----------------------------------------------------------
LPRPort.nativeavailable

   accept:      none
   perform:     find out the number of bytes available for reading
   return:      available bytes
                -1 on error
   exceptions:  none
----------------------------------------------------------*/ 
JNIEXPORT jint JNICALL Java_gnu_io_LPRPort_nativeavailable( JNIEnv *env,
	jobject jobj )
{
	int fd = get_java_fd( env, jobj );
	int result;

	if( ioctl( fd, FIONREAD, &result ) ) return -1;
	else return (jint)result;
}


/*----------------------------------------------------------
LPRPort.setHWFC

   accept:      state (JNI_FALSE 0, JNI_TRUE 1)
   perform:     set hardware flow control to state
   return:      none
   exceptions:  IOException
----------------------------------------------------------*/
JNIEXPORT void JNICALL Java_gnu_io_LPRPort_setHWFC( JNIEnv *env,
	jobject jobj, jboolean state )
{
	int fd = get_java_fd( env, jobj );
	return;
}


/*----------------------------------------------------------
LPRPort.eventLoop

   accept:      none
   perform:     periodically check for ParallelPortEvents
   return:      none
   exceptions:  none
   comments:    lots of work needed here
struct lp_stats {
        unsigned long chars;
        unsigned long sleeps;
        unsigned int maxrun;
        unsigned int maxwait;
        unsigned int meanwait;
        unsigned int mdev;
};

----------------------------------------------------------*/ 
JNIEXPORT void JNICALL Java_gnu_io_LPRPort_eventLoop( JNIEnv *env,
	jobject jobj )
{
	int fd, ret, change;
	unsigned int pflags;
	fd_set rfds;
	struct timeval sleep;
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

	FD_ZERO( &rfds );
	while( !interrupted ) {
		FD_SET( fd, &rfds );
		sleep.tv_sec = 1;	/* Check every 1 second, or on receive data */
		sleep.tv_usec = 0;
		do { 
			ret = select( fd + 1, &rfds, NULL, NULL, &sleep );
		}
		while (ret < 0 && errno == EINTR);
		if( ret < 0 ) break;
#if defined(__linux__)
		if( ioctl( fd, LPGETSTATUS, &pflags ) ) break;
#else
/*  FIXME??  */
#endif

/*
                       PAR_EV_BUFFER:
                       PAR_EV_ERROR:
*/

#if defined(__linux__)
		if (pflags&LP_BUSY)    /* inverted input, active high */
#endif
#if defined(__FreeBSD__)
		if (pflags&EBUSY)    /* inverted input, active high */
#endif
			(*env)->CallVoidMethod( env, jobj, method, (jint)PAR_EV_ERROR, JNI_TRUE );
/*  FIXME  this has moved into the ifdef __kernel__?  Need to get the
           posix documentation on this.
		if (pflags&LP_ACK)   
			(*env)->CallVoidMethod( env, jobj, method, (jint)PAR_EV_ERROR, JNI_TRUE );
*/  
#if defined (__linux__)
/* unchanged input, active low */
		if (pflags&LP_NOPA)   /* unchanged input, active high */
			(*env)->CallVoidMethod( env, jobj, method, (jint)PAR_EV_ERROR, JNI_TRUE );
		if (pflags&LP_SELEC)  /* unchanged input, active high */
			(*env)->CallVoidMethod( env, jobj, method, (jint)PAR_EV_ERROR, JNI_TRUE );
		if (pflags&LP_ERR)  /* unchanged input, active low */
			(*env)->CallVoidMethod( env, jobj, method, (jint)PAR_EV_ERROR, JNI_TRUE );
#else
/*  FIXME??  */
#endif
		interrupted = (*env)->CallStaticBooleanMethod( env, jthread, interrupt );
	}
	return;
}


/*----------------------------------------------------------
 send_printer_events

   accept:      int    event     ParallelPortEvent constant
                int    change    Number of times this event happened
                int    state     current state: 0 is false, nonzero is true
   perform:     Send the necessary events
   return:      none
   exceptions:  none
   comments:    Since the interrupt counters tell us how many times the
                state has changed, we can send a ParallelPortEvent for each
                interrupt (change) that has occured.  If we don't do this,
                we'll miss a whole bunch of events.
----------------------------------------------------------*/ 
void send_printer_events( JNIEnv *env, jobject jobj, jmethodID method,
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
                jobj (java NativePort object)
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
IOException

   accept:      env (keyhole to java)
                *msg (error message)
   perform:     Throw a java.io.IOException
   return:      none
   exceptions:  haha!
   comments:
----------------------------------------------------------*/ 
void IOException( JNIEnv *env, char *msg )
{
	jclass clazz = (*env)->FindClass( env, "java/io/IOException" );
	if( clazz == 0 ) return;
	(*env)->ThrowNew( env, clazz, msg );
}


/*----------------------------------------------------------
UnsupportedCommOperationException

   accept:      env (keyhole to java)
                *msg (error message)
   perform:     Throw a javax.comm.UnsupportedCommOperationException
   return:      none
   exceptions:  haha!
   comments:
----------------------------------------------------------*/ 
void UnsupportedCommOperationException( JNIEnv *env, char *msg )
{
	jclass clazz = (*env)->FindClass( env,
		"javax/comm/UnsupportedCommOperationException" );
	if( clazz == 0 ) return;
	(*env)->ThrowNew( env, clazz, msg );
}
JNIEXPORT void JNICALL Java_gnu_io_LPRPort_setInputBufferSize(JNIEnv *env, jobject jobj,  jint size )
{
#ifdef DEBUG
	printf("setInputBufferSize is not implemented\n");
#endif
}
JNIEXPORT jint JNICALL Java_gnu_io_LPRPort_getInputBufferSize(JNIEnv *env, jobject jobj)
{
#ifdef DEBUG
	printf("getInputBufferSize is not implemented\n");
#endif
	return(1);
}
JNIEXPORT void JNICALL Java_gnu_io_LPRPort_setOutputBufferSize(JNIEnv *env, jobject jobj, jint size )
{
#ifdef DEBUG
	printf("setOutputBufferSize is not implemented\n");
#endif
}
JNIEXPORT jint JNICALL Java_gnu_io_LPRPort_getOutputBufferSize(JNIEnv *env, jobject jobj)
{
#ifdef DEBUG
	printf("getOutputBufferSize is not implemented\n");
#endif
	return(1);
}
