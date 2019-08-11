/* 
 * Copyright (C) 2012-2014 Chris McClelland
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *  
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <makestuff.h>
#include <libfpgalink.h>
#include <libbuffer.h>
#include <liberror.h>
#include <libdump.h>
#include <argtable2.h>
#include <readline/readline.h>
#include <readline/history.h>
#ifdef WIN32
#include <Windows.h>
#else
#include <sys/time.h>
#endif

bool sigIsRaised(void);
void sigRegisterHandler(void);

static const char *ptr;
static bool enableBenchmarking = false;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

void delay(unsigned int mseconds)
{
	clock_t goal = mseconds + clock();
	while (goal > clock());
}

int myxor(int arr[])
{
	int count = 0;
	for(int i=0; i<8; i++)
		if(arr[i] == 1) count++;

	if(count%2 == 1) return 1;
	else return 0;
}

int bitArrayToInt(int arr[], int count)
{
	int ret = 0;
	int tmp;
	for (int i = 0; i < count; i++) {
		tmp = arr[count - i -1];
		ret |= tmp << (count - i - 1);
	}
	return ret;
}

void BinTouint8(uint8 bin[], int arr[])
{
	int arr0[8], arr1[8], arr2[8], arr3[8];
	for (int i = 0; i < 8; ++i)
	{
		arr0[i] = arr[i];
		arr1[i] = arr[i+8];
		arr2[i] = arr[i+16];
		arr3[i] = arr[i+24];
	}
	
	bin[0] = bitArrayToInt(arr0, 8);
	bin[1] = bitArrayToInt(arr1, 8);
	bin[2] = bitArrayToInt(arr2, 8);
	bin[3] = bitArrayToInt(arr3, 8);
}

void decToBinary(int n, int arr[], int count)
{
	int binaryNum[count];
	for (int i = 0; i < count; ++i)
		binaryNum[i] = 0;

	int i = 0;
	while (n > 0) {
		binaryNum[i] = n % 2;
		n = n / 2;
		i++;
	}

    // printing binary array in reverse order
	for (int i=0; i<count; i++)
		arr[i] = binaryNum[i];
}

void encrypter(int P[], int K[], int bin[])
{
  //int plain_text[32];
	int key[32];
	int cipher_text[32];
	int T[4];
	int count1=0;

	for (int i = 0; i < 32; ++i)
	{
		cipher_text[i] = P[i];
		key[i] = K[i];
		if(K[i] == 1) 
			count1++;
	}

	int arr0[8];
	int arr1[8];
	int arr2[8];
	int arr3[8];

	for(int i=0; i<8; i++)
	{
		arr0[i] = key[4*i];
		arr1[i] = key[4*i+1];
		arr2[i] = key[4*i+2];
		arr3[i] = key[4*i+3];
	}

	T[0] = myxor(arr0);
	T[1] = myxor(arr1);
	T[2] = myxor(arr2);
	T[3] = myxor(arr3);

	int T32[32];
	int dec = bitArrayToInt(T, 4);

	for (int i = 0; i < count1; ++i)
	{ 
		for (int j = 0; j < 8; j++)
		{
			T32[4*j] = T[0];
			T32[4*j+1] = T[1];
			T32[4*j+2] = T[2];
			T32[4*j+3] = T[3];
		}

		for (int j = 0; j < 32; ++j)
			cipher_text[j] = cipher_text[j] ^ T32[j];

    // for (int j = 0; j < 32; ++j)
    //  printf("%d", cipher_text[j]);
    // printf("\n");

		dec++;
		if(dec==16) dec = 0;
		decToBinary(dec, T, 4);
	}

	for (int i = 0; i < 32; ++i)
		bin[i] = cipher_text[i];
}

void decrypter(int P[], int K[], int bin[])
{
  //int plain_text[32];
	int key[32];
	int cipher_text[32];
	int T[4];
	int count1=0;

	for (int i = 0; i < 32; ++i)
	{
		cipher_text[i] = P[i];
		key[i] = K[i];
		if(K[i] == 0) 
			count1++;
	}

	int arr0[8];
	int arr1[8];
	int arr2[8];
	int arr3[8];

	for(int i=0; i<8; i++)
	{
		arr0[i] = key[4*i];
		arr1[i] = key[4*i+1];
		arr2[i] = key[4*i+2];
		arr3[i] = key[4*i+3];
	}

	T[0] = myxor(arr0);
	T[1] = myxor(arr1);
	T[2] = myxor(arr2);
	T[3] = myxor(arr3);

	int T32[32];
	int dec = bitArrayToInt(T, 4);
	dec = (dec+15)%16;
	decToBinary(dec, T, 4);

	for (int i = 0; i < count1; ++i)
	{ 
		for (int j = 0; j < 8; j++)
		{
			T32[4*j] = T[0];
			T32[4*j+1] = T[1];
			T32[4*j+2] = T[2];
			T32[4*j+3] = T[3];
		}

		for (int j = 0; j < 32; ++j)
			cipher_text[j] = cipher_text[j] ^ T32[j];

		dec=(dec+15)%16;
		decToBinary(dec, T, 4);
	}

	for (int i = 0; i < 32; ++i)
		bin[i] = cipher_text[i];
}


void convert_to_binary(int co_ordinates[8], unsigned int* buff)
{  
	int arb = 0;
	int i = 7;
	while(buff[0]!=0)
	{  //printf("%s\n", "Running");
arb = buff[0] % 2;
buff[0] = buff[0]/2;
co_ordinates[i] = arb;
i--;	
}
}

void print(int x_cord, int y_cord, int final_arr[8][8], FILE * fp)
{

	int arr[8][5];
	int count = 0;
	//printf("%s\n", "The x and y ");

	char conversion[8][3] = {"000","001","010","011","100","101","110","111"};

	for (int i = 0; i < 8; i++)
	{
		arr[i][0] = -1;
		arr[i][1] = -1;
		arr[i][2] = -1;
		arr[i][3] = -1;
		arr[i][4] = -1;
		final_arr[i][7] = -1;
	}

	if(fp == NULL)
	{
		fprintf(stderr, "Error, not Running" );
	}
	int a,b,c,d,e;
	printf("%s\n", "The x and y");
	while (fscanf(fp, " %d,%d,%d,%d,%d", &a, &b,&c,&d,&e) == 5) {

		if(a==x_cord && b==y_cord)
		{
			arr[c][0] = x_cord;
			arr[c][1] = y_cord;
			arr[c][2] = c;
			arr[c][3] = d;
			arr[c][4] = e;
			count++;

		}

	}

	for (int i = 0; i < 8; i++)
	{
		if(arr[i][0] == -1 && arr[i][1] == -1 && arr[i][2] == -1 && arr[i][3] == -1 && arr[i][4] == -1)
		{
			arr[i][0] = x_cord;
			arr[i][1] = y_cord;
			arr[i][2] = i;
			arr[i][3] = 0;
			arr[i][4] = 0;
			final_arr[i][7] = 0;
		}
	}

	for (int i = 0; i < 8; i++)
	{
		int next_signal = arr[i][4];
		int track_ok = arr[i][3];
		int direction = arr[i][2];

		final_arr[i][0] = conversion[next_signal][2] -48;
		final_arr[i][1] = conversion[next_signal][1] -48;
		final_arr[i][2] = conversion[next_signal][0] -48;

		final_arr[i][3] = conversion[direction][2] -48;
		final_arr[i][4] = conversion[direction][1] -48;
		final_arr[i][5] = conversion[direction][0] -48;

		final_arr[i][6] = track_ok;

		if(final_arr[i][7] != 0 || final_arr[i][7] == -1)
		{
			final_arr[i][7] = 1;
		}
	}

}

void uint8ToBin(uint8 arr[], int rec[])
{
	printf("%s\n ", "uint8");

	for (int i = 0; i < 4; ++i)
	{
		uint8 tempssss= arr[i];
		for(int k=0;k<8;k++)
		{
			rec[8*i+k] =(int) (tempssss % 2);
			tempssss = tempssss/2;
		}
	}
}

void writecsv(int x_cord, int y_cord, int ok, int direction, int next)
{
	FILE *fp1 = fopen("/home/shubham/Desktop/track_data.csv", "r");
	int mat[100][5];
	//int temp;
	int i =0;
	int a,b,c,d,e;
	while (fscanf(fp1, "%d,%d,%d,%d,%d", &a, &b,&c,&d,&e) == 5)
	{

		mat[i][4] = a;
		mat[i][3] = b;
		mat[i][2] = c;
		mat[i][1] = d;
		mat[i][0] = e;
		i++; 
	}

	int len = i;

	printf("%s\n", "Existing track_data");
	for(int i=0;i<len;i++)
	{
		for(int j=0;j<5;j++)
		{
			printf("%d ", mat[i][4-j]);
		}
		printf("\n");
	}

	printf("\n");
	FILE *fp2 = fopen("/home/shubham/Desktop/track_data.csv", "w");
	
	int counter = 0;
	for (int i = 0; i < len; ++i)
	{
		if(mat[i][4] == x_cord && mat[i][3] == y_cord && mat[i][2] == direction)
		{
			mat[i][1] = ok;
			mat[i][0] = next;
			counter++;
		}
	}

	if(counter == 0)
	{
		mat[len][4] = x_cord;
		mat[len][3] = y_cord;
		mat[len][2] = direction;
		mat[len][1] = ok;
		mat[len][0] = next;
		len++;
	}

	for (int i = 0; i < len; ++i)
	{
		fprintf(fp2, "%d,%d,%d,%d,%d\n", mat[i][4], mat[i][3], mat[i][2], mat[i][1], mat[i][0]);
	}

	printf("%s\n", "Updated track_data");
	for(int i=0;i<len;i++)
	{
		for(int j=0;j<5;j++)
		{
			printf("%d ", mat[i][4-j]);
		}
		printf("\n");
	}
	fclose(fp1);
	fclose(fp2);
}


//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////


static bool isHexDigit(char ch) {
	return
	(ch >= '0' && ch <= '9') ||
	(ch >= 'a' && ch <= 'f') ||
	(ch >= 'A' && ch <= 'F');
}

static uint16 calcChecksum(const uint8 *data, size_t length) {
	uint16 cksum = 0x0000;
	while ( length-- ) {
		cksum = (uint16)(cksum + *data++);
	}
	return cksum;
}

static bool getHexNibble(char hexDigit, uint8 *nibble) {
	if ( hexDigit >= '0' && hexDigit <= '9' ) {
		*nibble = (uint8)(hexDigit - '0');
		return false;
	} else if ( hexDigit >= 'a' && hexDigit <= 'f' ) {
		*nibble = (uint8)(hexDigit - 'a' + 10);
		return false;
	} else if ( hexDigit >= 'A' && hexDigit <= 'F' ) {
		*nibble = (uint8)(hexDigit - 'A' + 10);
		return false;
	} else {
		return true;
	}
}

static int getHexByte(uint8 *byte) {
	uint8 upperNibble;
	uint8 lowerNibble;
	if ( !getHexNibble(ptr[0], &upperNibble) && !getHexNibble(ptr[1], &lowerNibble) ) {
		*byte = (uint8)((upperNibble << 4) | lowerNibble);
		byte += 2;
		return 0;
	} else {
		return 1;
	}
}

static const char *const errMessages[] = {
	NULL,
	NULL,
	"Unparseable hex number",
	"Channel out of range",
	"Conduit out of range",
	"Illegal character",
	"Unterminated string",
	"No memory",
	"Empty string",
	"Odd number of digits",
	"Cannot load file",
	"Cannot save file",
	"Bad arguments"
};

typedef enum {
	FLP_SUCCESS,
	FLP_LIBERR,
	FLP_BAD_HEX,
	FLP_CHAN_RANGE,
	FLP_CONDUIT_RANGE,
	FLP_ILL_CHAR,
	FLP_UNTERM_STRING,
	FLP_NO_MEMORY,
	FLP_EMPTY_STRING,
	FLP_ODD_DIGITS,
	FLP_CANNOT_LOAD,
	FLP_CANNOT_SAVE,
	FLP_ARGS
} ReturnCode;

static ReturnCode doRead(
	struct FLContext *handle, uint8 chan, uint32 length, FILE *destFile, uint16 *checksum,
	const char **error)
{
	ReturnCode retVal = FLP_SUCCESS;
	uint32 bytesWritten;
	FLStatus fStatus;
	uint32 chunkSize;
	const uint8 *recvData;
	uint32 actualLength;
	const uint8 *ptr;
	uint16 csVal = 0x0000;
	#define READ_MAX 65536

	// Read first chunk
	chunkSize = length >= READ_MAX ? READ_MAX : length;
	fStatus = flReadChannelAsyncSubmit(handle, chan, chunkSize, NULL, error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");
	length = length - chunkSize;

	while ( length ) {
		// Read chunk N
		chunkSize = length >= READ_MAX ? READ_MAX : length;
		fStatus = flReadChannelAsyncSubmit(handle, chan, chunkSize, NULL, error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");
		length = length - chunkSize;
		printf("Bafoon\n");
		
		// Await chunk N-1
		fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");

		// Write chunk N-1 to file
		bytesWritten = (uint32)fwrite(recvData, 1, actualLength, destFile);
		CHECK_STATUS(bytesWritten != actualLength, FLP_CANNOT_SAVE, cleanup, "doRead()");

		// Checksum chunk N-1
		chunkSize = actualLength;
		ptr = recvData;
		while ( chunkSize-- ) {
			csVal = (uint16)(csVal + *ptr++);
		}
	}

	// Await last chunk
	fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doRead()");
	
	// Write last chunk to file
	bytesWritten = (uint32)fwrite(recvData, 1, actualLength, destFile);
	CHECK_STATUS(bytesWritten != actualLength, FLP_CANNOT_SAVE, cleanup, "doRead()");

	// Checksum last chunk
	chunkSize = actualLength;
	ptr = recvData;
	while ( chunkSize-- ) {
		csVal = (uint16)(csVal + *ptr++);
	}
	
	// Return checksum to caller
	*checksum = csVal;
	cleanup:
	return retVal;
}

static ReturnCode doWrite(
	struct FLContext *handle, uint8 chan, FILE *srcFile, size_t *length, uint16 *checksum,
	const char **error)
{
	ReturnCode retVal = FLP_SUCCESS;
	size_t bytesRead, i;
	FLStatus fStatus;
	const uint8 *ptr;
	uint16 csVal = 0x0000;
	size_t lenVal = 0;
	#define WRITE_MAX (65536 - 5)
	uint8 buffer[WRITE_MAX];

	do {
		// Read Nth chunk
		bytesRead = fread(buffer, 1, WRITE_MAX, srcFile);
		if ( bytesRead ) {
			// Update running total
			lenVal = lenVal + bytesRead;

			// Submit Nth chunk
			fStatus = flWriteChannelAsync(handle, chan, bytesRead, buffer, error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doWrite()");

			// Checksum Nth chunk
			i = bytesRead;
			ptr = buffer;
			while ( i-- ) {
				csVal = (uint16)(csVal + *ptr++);
			}
		}
	} while ( bytesRead == WRITE_MAX );

	// Wait for writes to be received. This is optional, but it's only fair if we're benchmarking to
	// actually wait for the work to be completed.
	fStatus = flAwaitAsyncWrites(handle, error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup, "doWrite()");

	// Return checksum & length to caller
	*checksum = csVal;
	*length = lenVal;
	cleanup:
	return retVal;
}

static int parseLine(struct FLContext *handle, const char *line, const char **error) {
	ReturnCode retVal = FLP_SUCCESS, status;
	FLStatus fStatus;
	struct Buffer dataFromFPGA = {0,};
	BufferStatus bStatus;
	uint8 *data = NULL;
	char *fileName = NULL;
	FILE *file = NULL;
	double totalTime, speed;
	#ifdef WIN32
	LARGE_INTEGER tvStart, tvEnd, freq;
	DWORD_PTR mask = 1;
	SetThreadAffinityMask(GetCurrentThread(), mask);
	QueryPerformanceFrequency(&freq);
	#else
	struct timeval tvStart, tvEnd;
	long long startTime, endTime;
	#endif
	bStatus = bufInitialise(&dataFromFPGA, 1024, 0x00, error);
	CHECK_STATUS(bStatus, FLP_LIBERR, cleanup);
	ptr = line;
	do {
		while ( *ptr == ';' ) {
			ptr++;
		}
		switch ( *ptr ) {
			case 'r':{
				uint32 chan;
				uint32 length = 1;
				char *end;
				ptr++;

			// Get the channel to be read:
				errno = 0;
				chan = (uint32)strtoul(ptr, &end, 16);
				CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
				CHECK_STATUS(chan > 127, FLP_CHAN_RANGE, cleanup);
				ptr = end;

			// Only three valid chars at this point:
				CHECK_STATUS(*ptr != '\0' && *ptr != ';' && *ptr != ' ', FLP_ILL_CHAR, cleanup);

				if ( *ptr == ' ' ) {
					ptr++;

				// Get the read count:
					errno = 0;
					length = (uint32)strtoul(ptr, &end, 16);
					CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);
					ptr = end;

				// Only three valid chars at this point:
					CHECK_STATUS(*ptr != '\0' && *ptr != ';' && *ptr != ' ', FLP_ILL_CHAR, cleanup);
					if ( *ptr == ' ' ) {
						const char *p;
						const char quoteChar = *++ptr;
						CHECK_STATUS(
							(quoteChar != '"' && quoteChar != '\''),
							FLP_ILL_CHAR, cleanup);

					// Get the file to write bytes to:
						ptr++;
						p = ptr;
						while ( *p != quoteChar && *p != '\0' ) {
							p++;
						}
						CHECK_STATUS(*p == '\0', FLP_UNTERM_STRING, cleanup);
						fileName = malloc((size_t)(p - ptr + 1));
						CHECK_STATUS(!fileName, FLP_NO_MEMORY, cleanup);
						CHECK_STATUS(p - ptr == 0, FLP_EMPTY_STRING, cleanup);
						strncpy(fileName, ptr, (size_t)(p - ptr));
						fileName[p - ptr] = '\0';
						ptr = p + 1;
					}
				}
				if ( fileName ) {
					uint16 checksum = 0x0000;

				// Open file for writing
					file = fopen(fileName, "wb");
					CHECK_STATUS(!file, FLP_CANNOT_SAVE, cleanup);
					free(fileName);
					fileName = NULL;
					
				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					status = doRead(handle, (uint8)chan, length, file, &checksum, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					status = doRead(handle, (uint8)chan, length, file, &checksum, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
					if ( enableBenchmarking ) {
						printf(
							"Read %d bytes (checksum 0x%04X) from channel %d at %f MiB/s\n",
							length, checksum, chan, speed);
					}
					CHECK_STATUS(status, status, cleanup);

				// Close the file
					fclose(file);
					file = NULL;
				} else {
					size_t oldLength = dataFromFPGA.length;
					bStatus = bufAppendConst(&dataFromFPGA, 0x00, length, error);
					CHECK_STATUS(bStatus, FLP_LIBERR, cleanup);
				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					fStatus = flReadChannel(handle, (uint8)chan, length, dataFromFPGA.data + oldLength, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					fStatus = flReadChannel(handle, (uint8)chan, length, dataFromFPGA.data + oldLength, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
					if ( enableBenchmarking ) {
						printf(
							"Read %d bytes (checksum 0x%04X) from channel %d at %f MiB/s\n",
							length, calcChecksum(dataFromFPGA.data + oldLength, length), chan, speed);
					}
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
				}
				break;
			}
			case 'w':{
				unsigned long int chan;
				size_t length = 1, i;
				char *end, ch;
				const char *p;
				ptr++;

			// Get the channel to be written:
				errno = 0;
				chan = strtoul(ptr, &end, 16);
				CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
				CHECK_STATUS(chan > 127, FLP_CHAN_RANGE, cleanup);
				ptr = end;

			// There must be a space now:
				CHECK_STATUS(*ptr != ' ', FLP_ILL_CHAR, cleanup);

			// Now either a quote or a hex digit
				ch = *++ptr;
				if ( ch == '"' || ch == '\'' ) {
					uint16 checksum = 0x0000;

				// Get the file to read bytes from:
					ptr++;
					p = ptr;
					while ( *p != ch && *p != '\0' ) {
						p++;
					}
					CHECK_STATUS(*p == '\0', FLP_UNTERM_STRING, cleanup);
					fileName = malloc((size_t)(p - ptr + 1));
					CHECK_STATUS(!fileName, FLP_NO_MEMORY, cleanup);
					CHECK_STATUS(p - ptr == 0, FLP_EMPTY_STRING, cleanup);
					strncpy(fileName, ptr, (size_t)(p - ptr));
					fileName[p - ptr] = '\0';
				ptr = p + 1;  // skip over closing quote

				// Open file for reading
				file = fopen(fileName, "rb");
				CHECK_STATUS(!file, FLP_CANNOT_LOAD, cleanup);
				free(fileName);
				fileName = NULL;
				
				#ifdef WIN32
				QueryPerformanceCounter(&tvStart);
				status = doWrite(handle, (uint8)chan, file, &length, &checksum, error);
				QueryPerformanceCounter(&tvEnd);
				totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
				totalTime /= freq.QuadPart;
				speed = (double)length / (1024*1024*totalTime);
				#else
				gettimeofday(&tvStart, NULL);
				status = doWrite(handle, (uint8)chan, file, &length, &checksum, error);
				gettimeofday(&tvEnd, NULL);
				startTime = tvStart.tv_sec;
				startTime *= 1000000;
				startTime += tvStart.tv_usec;
				endTime = tvEnd.tv_sec;
				endTime *= 1000000;
				endTime += tvEnd.tv_usec;
				totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
					if ( enableBenchmarking ) {
						printf(
							"Wrote "PFSZD" bytes (checksum 0x%04X) to channel %lu at %f MiB/s\n",
							length, checksum, chan, speed);
					}
					CHECK_STATUS(status, status, cleanup);

				// Close the file
					fclose(file);
					file = NULL;
				} else if ( isHexDigit(ch) ) {
				// Read a sequence of hex bytes to write
					uint8 *dataPtr;
					p = ptr + 1;
					while ( isHexDigit(*p) ) {
						p++;
					}
					CHECK_STATUS((p - ptr) & 1, FLP_ODD_DIGITS, cleanup);
					length = (size_t)(p - ptr) / 2;
					data = malloc(length);
					dataPtr = data;
					for ( i = 0; i < length; i++ ) {
						getHexByte(dataPtr++);
						ptr += 2;
					}
				#ifdef WIN32
					QueryPerformanceCounter(&tvStart);
					fStatus = flWriteChannel(handle, (uint8)chan, length, data, error);
					QueryPerformanceCounter(&tvEnd);
					totalTime = (double)(tvEnd.QuadPart - tvStart.QuadPart);
					totalTime /= freq.QuadPart;
					speed = (double)length / (1024*1024*totalTime);
				#else
					gettimeofday(&tvStart, NULL);
					fStatus = flWriteChannel(handle, (uint8)chan, length, data, error);
					gettimeofday(&tvEnd, NULL);
					startTime = tvStart.tv_sec;
					startTime *= 1000000;
					startTime += tvStart.tv_usec;
					endTime = tvEnd.tv_sec;
					endTime *= 1000000;
					endTime += tvEnd.tv_usec;
					totalTime = (double)(endTime - startTime);
					totalTime /= 1000000;  // convert from uS to S.
					speed = (double)length / (1024*1024*totalTime);
				#endif
					if ( enableBenchmarking ) {
						printf(
							"Wrote "PFSZD" bytes (checksum 0x%04X) to channel %lu at %f MiB/s\n",
							length, calcChecksum(data, length), chan, speed);
					}
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					free(data);
					data = NULL;
				} else {
					FAIL(FLP_ILL_CHAR, cleanup);
				}
				break;
			}
			case '+':{
				uint32 conduit;
				char *end;
				ptr++;

			// Get the conduit
				errno = 0;
				conduit = (uint32)strtoul(ptr, &end, 16);
				CHECK_STATUS(errno, FLP_BAD_HEX, cleanup);

			// Ensure that it's 0-127
				CHECK_STATUS(conduit > 255, FLP_CONDUIT_RANGE, cleanup);
				ptr = end;

			// Only two valid chars at this point:
				CHECK_STATUS(*ptr != '\0' && *ptr != ';', FLP_ILL_CHAR, cleanup);

				fStatus = flSelectConduit(handle, (uint8)conduit, error);
				CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
				break;
			}
			default:
			FAIL(FLP_ILL_CHAR, cleanup);
		}
	} while ( *ptr == ';' );
	CHECK_STATUS(*ptr != '\0', FLP_ILL_CHAR, cleanup);

	dump(0x00000000, dataFromFPGA.data, dataFromFPGA.length);

	cleanup:
	bufDestroy(&dataFromFPGA);
	if ( file ) {
		fclose(file);
	}
	free(fileName);
	free(data);
	if ( retVal > FLP_LIBERR ) {
		const int column = (int)(ptr - line);
		int i;
		fprintf(stderr, "%s at column %d\n  %s\n  ", errMessages[retVal], column, line);
		for ( i = 0; i < column; i++ ) {
			fprintf(stderr, " ");
		}
		fprintf(stderr, "^\n");
	}
	return retVal;
}

static const char *nibbles[] = {
	"0000",  // '0'
	"0001",  // '1'
	"0010",  // '2'
	"0011",  // '3'
	"0100",  // '4'
	"0101",  // '5'
	"0110",  // '6'
	"0111",  // '7'
	"1000",  // '8'
	"1001",  // '9'

	"XXXX",  // ':'
	"XXXX",  // ';'
	"XXXX",  // '<'
	"XXXX",  // '='
	"XXXX",  // '>'
	"XXXX",  // '?'
	"XXXX",  // '@'

	"1010",  // 'A'
	"1011",  // 'B'
	"1100",  // 'C'
	"1101",  // 'D'
	"1110",  // 'E'
	"1111"   // 'F'
};

////////////////////////////////////////////////////////////////////////
int asciiToBinary(int input) {
	int result = 0, i = 1, remainder;

        /* convert decimal to binary format */
	while (input > 0) {
		remainder = input % 2;
		result = result + (i * remainder);
		input = input / 2;
		i = i * 10;
	}

        /* print the resultant binary value */
	return(result);
}

// void decToBinary(int n, int arr[], int count)
// {
//     int binaryNum[count];
//     for (int i = 0; i < count; ++i)
//       binaryNum[i] = 0;

//     int i = 0;
//     while (n > 0) {
//         binaryNum[i] = n % 2;
//         n = n / 2;
//         i++;
//     }

//      for (int i=0; i<count; i++)
//         arr[i] = binaryNum[i];
// }

//////////////////////////////////////////////////////////////////////////////////


int main(int argc, char *argv[]) {
	ReturnCode retVal = FLP_SUCCESS, pStatus;
	struct arg_str *ivpOpt = arg_str0("i", "ivp", "<VID:PID>", "            vendor ID and product ID (e.g 04B4:8613)");
	struct arg_str *vpOpt = arg_str1("v", "vp", "<VID:PID[:DID]>", "       VID, PID and opt. dev ID (e.g 1D50:602B:0001)");
	struct arg_str *fwOpt = arg_str0("f", "fw", "<firmware.hex>", "        firmware to RAM-load (or use std fw)");
	struct arg_str *portOpt = arg_str0("d", "ports", "<bitCfg[,bitCfg]*>", " read/write digital ports (e.g B13+,C1-,B2?)");
	struct arg_str *queryOpt = arg_str0("q", "query", "<jtagBits>", "         query the JTAG chain");
	struct arg_str *progOpt = arg_str0("p", "program", "<config>", "         program a device");
	struct arg_uint *conOpt = arg_uint0("c", "conduit", "<conduit>", "        which comm conduit to choose (default 0x01)");
	struct arg_str *actOpt = arg_str0("a", "action", "<actionString>", "    a series of CommFPGA actions");
	struct arg_lit *doworkOpt = arg_lit0("z", "some stuff" ,"                 some other stuff");
	struct arg_lit *shellOpt  = arg_lit0("s", "shell", "                    start up an interactive CommFPGA session");
	struct arg_lit *benOpt  = arg_lit0("b", "benchmark", "                enable benchmarking & chefcksumming");
	struct arg_lit *rstOpt  = arg_lit0("r", "reset", "                    reset the bulk endpoints");
	struct arg_str *dumpOpt = arg_str0("l", "dumploop", "<ch:file.bin>", "   write data from channel ch to file");
	struct arg_lit *helpOpt  = arg_lit0("h", "help", "                     print this help and exit");
	struct arg_str *eepromOpt  = arg_str0(NULL, "eeprom", "<std|fw.hex|fw.iic>", "   write firmware to FX2's EEPROM (!!)");
	struct arg_str *backupOpt  = arg_str0(NULL, "backup", "<kbitSize:fw.iic>", "     backup FX2's EEPROM (e.g 128:fw.iic)\n");
	////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////
	struct arg_end *endOpt   = arg_end(20);




	void *argTable[] = {
		ivpOpt, vpOpt, fwOpt, portOpt, queryOpt, progOpt, conOpt, actOpt,
		shellOpt, benOpt, rstOpt, dumpOpt, helpOpt, eepromOpt, backupOpt, doworkOpt, endOpt
	};
	const char *progName = "flcli";
	int numErrors;
	struct FLContext *handle = NULL;
	FLStatus fStatus;
	const char *error = NULL;
	const char *ivp = NULL;
	const char *vp = NULL;
	bool isNeroCapable, isCommCapable;
	uint32 numDevices, scanChain[16], i;
	const char *line = NULL;
	uint8 conduit = 0x01;

	if ( arg_nullcheck(argTable) != 0 ) {
		fprintf(stderr, "%s: insufficient memory\n", progName);
		FAIL(1, cleanup);
	}

	numErrors = arg_parse(argc, argv, argTable);

	if ( helpOpt->count > 0 ) {
		printf("FPGALink Command-Line Interface Copyright (C) 2012-2014 Chris McClelland\n\nUsage: %s", progName);
		arg_print_syntax(stdout, argTable, "\n");
		printf("\nInteract with an FPGALink device.\n\n");
		arg_print_glossary(stdout, argTable,"  %-10s %s\n");
		FAIL(FLP_SUCCESS, cleanup);
	}

	if ( numErrors > 0 ) {
		arg_print_errors(stdout, endOpt, progName);
		fprintf(stderr, "Try '%s --help' for more information.\n", progName);
		FAIL(FLP_ARGS, cleanup);
	}

	fStatus = flInitialise(0, &error);
	CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);

	vp = vpOpt->sval[0];

	printf("Attempting to open connection to FPGALink device %s...\n", vp);
	fStatus = flOpen(vp, &handle, NULL);
	if ( fStatus ) {
		if ( ivpOpt->count ) {
			int count = 60;
			uint8 flag;
			ivp = ivpOpt->sval[0];
			printf("Loading firmware into %s...\n", ivp);
			if ( fwOpt->count ) {
				fStatus = flLoadCustomFirmware(ivp, fwOpt->sval[0], &error);
			} else {
				fStatus = flLoadStandardFirmware(ivp, vp, &error);
			}
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			
			printf("Awaiting renumeration");
			flSleep(1000);
			do {
				printf(".");
				fflush(stdout);
				fStatus = flIsDeviceAvailable(vp, &flag, &error);
				CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
				flSleep(250);
				count--;
			} while ( !flag && count );
			printf("\n");
			if ( !flag ) {
				fprintf(stderr, "FPGALink device did not renumerate properly as %s\n", vp);
				FAIL(FLP_LIBERR, cleanup);
			}

			printf("Attempting to open connection to FPGLink device %s again...\n", vp);
			fStatus = flOpen(vp, &handle, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		} else {
			fprintf(stderr, "Could not open FPGALink device at %s and no initial VID:PID was supplied\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	printf(
		"Connected to FPGALink device %s (firmwareID: 0x%04X, firmwareVersion: 0x%08X)\n",
		vp, flGetFirmwareID(handle), flGetFirmwareVersion(handle)
		);

	if ( eepromOpt->count ) {
		if ( !strcmp("std", eepromOpt->sval[0]) ) {
			printf("Writing the standard FPGALink firmware to the FX2's EEPROM...\n");
			fStatus = flFlashStandardFirmware(handle, vp, &error);
		} else {
			printf("Writing custom FPGALink firmware from %s to the FX2's EEPROM...\n", eepromOpt->sval[0]);
			fStatus = flFlashCustomFirmware(handle, eepromOpt->sval[0], &error);
		}
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
	}

	if ( backupOpt->count ) {
		const char *fileName;
		const uint32 kbitSize = strtoul(backupOpt->sval[0], (char**)&fileName, 0);
		if ( *fileName != ':' ) {
			fprintf(stderr, "%s: invalid argument to option --backup=<kbitSize:fw.iic>\n", progName);
			FAIL(FLP_ARGS, cleanup);
		}
		fileName++;
		printf("Saving a backup of %d kbit from the FX2's EEPROM to %s...\n", kbitSize, fileName);
		fStatus = flSaveFirmware(handle, kbitSize, fileName, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
	}

	if ( rstOpt->count ) {
		// Reset the bulk endpoints (only needed in some virtualised environments)
		fStatus = flResetToggle(handle, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
	}

	if ( conOpt->count ) {
		conduit = (uint8)conOpt->ival[0];
	}

	isNeroCapable = flIsNeroCapable(handle);
	isCommCapable = flIsCommCapable(handle, conduit);

	if ( portOpt->count ) {
		uint32 readState;
		char hex[9];
		const uint8 *p = (const uint8 *)hex;
		printf("Configuring ports...\n");
		fStatus = flMultiBitPortAccess(handle, portOpt->sval[0], &readState, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		sprintf(hex, "%08X", readState);
		printf("Readback:   28   24   20   16    12    8    4    0\n          %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf("  %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s", nibbles[*p++ - '0']);
		printf(" %s\n", nibbles[*p++ - '0']);
		flSleep(100);
	}

	if ( queryOpt->count ) {
		if ( isNeroCapable ) {
			fStatus = flSelectConduit(handle, 0x00, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = jtagScanChain(handle, queryOpt->sval[0], &numDevices, scanChain, 16, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( numDevices ) {
				printf("The FPGALink device at %s scanned its JTAG chain, yielding:\n", vp);
				for ( i = 0; i < numDevices; i++ ) {
					printf("  0x%08X\n", scanChain[i]);
				}
			} else {
				printf("The FPGALink device at %s scanned its JTAG chain but did not find any attached devices\n", vp);
			}
		} else {
			fprintf(stderr, "JTAG chain scan requested but FPGALink device at %s does not support NeroProg\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( progOpt->count ) {
		printf("Programming device...\n");
		if ( isNeroCapable ) {
			fStatus = flSelectConduit(handle, 0x00, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flProgram(handle, progOpt->sval[0], NULL, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		} else {
			fprintf(stderr, "Program operation requested but device at %s does not support NeroProg\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( benOpt->count ) {
		enableBenchmarking = true;
	}
	
	if ( actOpt->count ) {
		printf("Executing CommFPGA actions on FPGALink device %s...\n", vp);
		if ( isCommCapable ) {
			uint8 isRunning;
			fStatus = flSelectConduit(handle, conduit, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flIsFPGARunning(handle, &isRunning, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( isRunning ) {
				pStatus = parseLine(handle, actOpt->sval[0], &error);
				CHECK_STATUS(pStatus, pStatus, cleanup);
			} else {
				fprintf(stderr, "The FPGALink device at %s is not ready to talk - did you forget --program?\n", vp);
				FAIL(FLP_ARGS, cleanup);
			}
		} else {
			fprintf(stderr, "Action requested but device at %s does not support CommFPGA\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

	if ( dumpOpt->count ) {
		const char *fileName;
		unsigned long chan = strtoul(dumpOpt->sval[0], (char**)&fileName, 10);
		FILE *file = NULL;
		const uint8 *recvData;
		uint32 actualLength;
		if ( *fileName != ':' ) {
			fprintf(stderr, "%s: invalid argument to option -l|--dumploop=<ch:file.bin>\n", progName);
			FAIL(FLP_ARGS, cleanup);
		}
		fileName++;
		printf("Copying from channel %lu to %s", chan, fileName);
		file = fopen(fileName, "wb");
		CHECK_STATUS(!file, FLP_CANNOT_SAVE, cleanup);
		sigRegisterHandler();
		fStatus = flSelectConduit(handle, conduit, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		fStatus = flReadChannelAsyncSubmit(handle, (uint8)chan, 22528, NULL, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		do {
			fStatus = flReadChannelAsyncSubmit(handle, (uint8)chan, 22528, NULL, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fwrite(recvData, 1, actualLength, file);
			printf(".");
		} while ( !sigIsRaised() );
		printf("\nCaught SIGINT, quitting...\n");
		fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
		CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
		fwrite(recvData, 1, actualLength, file);
		fclose(file);
	}

	if ( shellOpt->count ) 
	{
		printf("\nEntering CommFPGA command-line mode:\n");
		if ( isCommCapable ) {
			uint8 isRunning;
			fStatus = flSelectConduit(handle, conduit, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flIsFPGARunning(handle, &isRunning, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			if ( isRunning ) {
				do {
					do {
						line = readline("> ");
					} while ( line && !line[0] );
					if ( line && line[0] && line[0] != 'q' ) {
						add_history(line);
						pStatus = parseLine(handle, line, &error);
						CHECK_STATUS(pStatus, pStatus, cleanup);
						free((void*)line);
					}
				} while ( line && line[0] != 'q' );
			} else {
				fprintf(stderr, "The FPGALink device at %s is not ready to talk - did you forget --xsvf?\n", vp);
				FAIL(FLP_ARGS, cleanup);
			}
		} else {
			fprintf(stderr, "Shell requested but device at %s does not support CommFPGA\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}
	}

///////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////
	if (doworkOpt->count) 
	{
		printf("%s\n", "Hello ");
		//int my_read_channel = 0;
		//uint8 my_write_channel = 0x01;
		//uint32 my_time_out = 0x00000002;
		//uint32 my_count = 0x0004;
		//uint32 *buf;
		// uint8 buf[4];
		// uint8 Ack1[4]={221,221,221,221};
		// uint8 Ack2[4]={187,187,187,187};
		printf("%s\n", "Helloghjkl ");
		if ( isCommCapable ) 
		{
			uint8 isRunning;
			fStatus = flSelectConduit(handle, conduit, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			fStatus = flIsFPGARunning(handle, &isRunning, &error);
			CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
			uint8 *recvData;
			recvData = malloc(8);
			uint32 actualLength;
			if ( isRunning ) 
			{
				int K[32] = {0,1,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,0,1,0,1,0,1,1,1,0,1,0};
				int ack1[32] = {0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1};
				int ack2[32] = {0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1,0,0,1,1};
				int reset_bytes[8] = {0,0,0,1,1,0,0,0};
				int reset_int = bitArrayToInt(reset_bytes,8);
				uint8 reset = (uint8)reset_int;
				int del = 1000;
				for (int k = 0; k < 64; ++k)
				{	printf("%s","Channel = " );
					printf("%d\n",2*k );
					uint8 get_encrypted_coord[4];
					printf("%s\n", "----------------------H2.1 started----------------------- ");
					printf("%s\n", "Waiting for encrypted co_ordinates");

					delay(del * 1000);
					fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &get_encrypted_coord[0], &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					if(get_encrypted_coord[0] == reset) 
						{
							k--; 
							printf("%s\n", "-------------------------RESET------------------------");
							continue;
						}
					delay(del);
					fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &get_encrypted_coord[1], &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					if(get_encrypted_coord[1] == reset) 
						{
							k--; 
							printf("%s\n", "-------------------------RESET------------------------");							
							continue;
						}
					delay(del);
					fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &get_encrypted_coord[2], &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					if(get_encrypted_coord[2] == reset) 
						{
							k--; 
							printf("%s\n", "-------------------------RESET------------------------");							
							continue;
						}
					delay(del);
					fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &get_encrypted_coord[3], &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					if(get_encrypted_coord[3] == reset) 
						{
							k--; 
							printf("%s\n", "-------------------------RESET------------------------");
							continue;
						}
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);

					printf("%s\n", "Got the encrypted co_ordinates, they are as in uint8 form --");
					for (int i = 0; i < 4; ++i)
					{
						printf("%d ", get_encrypted_coord[i]);
					}
					printf("\n");
					int encrypted_pos[32];
					uint8ToBin(get_encrypted_coord, encrypted_pos);
					printf("%s\n", "encrypted_pos is converted to binary");
					printf("%s\n", "The encrypted_pos are as --");
					for (int i = 0; i < 32; ++i)
					{
						printf("%d ", encrypted_pos[i]);
					}
					printf("\n");
					int decrypted_pos[32];
					decrypter(encrypted_pos, K, decrypted_pos);
					printf("%s\n", "The decrypted pos in binary form are as --");
					for (int i = 0; i < 32; ++i)
					{
						printf("%d ", decrypted_pos[i]);
					}
					printf("\n");
					printf("%s\n", "----------------------H2.1 ended----------------------- ");
					printf("%s\n", "----------------------H2.2 started----------------------- ");
					
					int encrypted_pos2[32];
					encrypter(decrypted_pos, K, encrypted_pos2);
					printf("%s\n", "The co_ordinates are re-encrypted in binary form");
					printf("%s\n", "The re-encrypted co_ords are as follows");
					for (int i = 0; i < 32; ++i)
					{
						printf("%d ", encrypted_pos2[i]);
					}
					printf("\n");
					uint8 send_encrypted_coord[4];
					printf("%s\n", "Converting the re-encrypted data in binary to uint8 form");
					BinTouint8(send_encrypted_coord, encrypted_pos);
					printf("%s\n", "Preparing to send the encrypted co_ordinates ");
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &send_encrypted_coord[0], &error);
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &send_encrypted_coord[1], &error);
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &send_encrypted_coord[2], &error);
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &send_encrypted_coord[3], &error);

					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					printf("%s\n", "Successfully send the encrypted co_ordinates in uint8 form");

					printf("%s\n", "----------------------H2.2 ended----------------------- ");

					printf("%s\n", "----------------------H2.3 started----------------------- ");
					printf("%s\n", "Waiting to get Ack1 from the fpga controller");
					uint8 first_ack1_received[4];
					delay(del);
					fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &first_ack1_received[0], &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					if(first_ack1_received[0] == reset) 
						{
							k--;
							printf("%s\n", "-------------------------RESET------------------------");							 
							continue;
						}
					delay(del);
					fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &first_ack1_received[1], &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					if(first_ack1_received[1] == reset) 
						{
							k--; 
							printf("%s\n", "-------------------------RESET------------------------");
							continue;
						}
					delay(del);
					fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &first_ack1_received[2], &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					if(first_ack1_received[2] == reset) 
						{
							k--; 
							printf("%s\n", "-------------------------RESET------------------------");
							continue;
						}
					delay(del);
					fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &first_ack1_received[3], &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					if(first_ack1_received[3] == reset) 
						{
							k--; 
							printf("%s\n", "-------------------------RESET------------------------");
							continue;
						}
					printf("%s\n", "First ack1 received successfully");
					printf("%s\n", "The first ack1 is ----- in uint8 form");
					for (int i = 0; i < 4; ++i)
					{
						printf("%d ", first_ack1_received[i]);
					}
					printf("\n");
					printf("%s\n", "----------------------H2.3 ended----------------------- ");
					printf("%s\n", "----------------------H2.4 started----------------------- ");
					
					int encrpyted_ack1[32];
					printf("%s\n", "Converting encrypted ack1 from uint8 to binary");
					uint8ToBin(first_ack1_received, encrpyted_ack1);
					printf("%s\n", "The encrypted ack1 is as follows in binary form --");
					for (int i = 0; i < 32; ++i)
					{
						printf("%d ", encrpyted_ack1[i]);
					}
					printf("\n");
					int decrypted_ack1[32];
					decrypter(encrpyted_ack1, K, decrypted_ack1);
					printf("%s\n", "The decrypted ack1 in binary is as follows --");
					for (int i = 0; i < 32; ++i)
					{
						printf("%d ", decrypted_ack1[i]);
					}
					printf("\n");
					printf("%s\n", "Checking if this ack1 is equal to the original ack1 ");

					int ack_helper = 0;
					for(int i=0; i<32; i++)
					{
						if(decrypted_ack1[i] != ack1[i])
						{
							ack_helper = 1;
						}
					}

					if(ack_helper == 1)
					{
						printf("%s\n", "Ack1 received doesn't match with the original one");
						uint8 encrypted_ack1_again[4];
						delay(1000000);
						printf("%s\n", "Reading the channel 2i again ");
						delay(del);
						fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &encrypted_ack1_again[0], &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						if(encrypted_ack1_again[0] == reset) 
							{
								k--; 
								printf("%s\n", "-------------------------RESET------------------------");
								continue;
							}
						delay(del);
						fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &encrypted_ack1_again[1], &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						if(encrypted_ack1_again[1] == reset) 
							{
								k--; 
								printf("%s\n", "-------------------------RESET------------------------");
								continue;
							}
						delay(del);
						fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &encrypted_ack1_again[2], &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						if(encrypted_ack1_again[2] == reset) 
							{
								k--; 
								printf("%s\n", "-------------------------RESET------------------------");
								continue;
							}
						delay(del);
						fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &encrypted_ack1_again[3], &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						if(encrypted_ack1_again[3] == reset) 
							{
								k--; 
								printf("%s\n", "-------------------------RESET------------------------");
								continue;
							}
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						printf("%s\n", "Get the encrypted ack1 again successfully ---");
						for (int i = 0; i < 4; ++i)
						{
							printf("%d ", encrypted_ack1_again[i]);
						}
						printf("\n");
						int encrypted_ack1_again1[32];
						printf("%s\n", "Converting encrypted ack1 from uint8 to binary");
						uint8ToBin(encrypted_ack1_again, encrypted_ack1_again1);
						printf("%s\n", "Converted encrypted ack1 to binary and it is as ----");
						for (int i = 0; i < 32; ++i)
						{
							printf("%d ", encrypted_ack1_again1[i]);
						}
						printf("\n");
						int decrypted_ack1_again1[32];
						printf("%s\n", "Decrypting the encrypted ack1 ");
						decrypter(encrypted_ack1_again1, K, decrypted_ack1_again1);
						printf("%s\n", "Ack1 got decrpyted and is as ----");
						for (int i = 0; i < 32; ++i)
						{
							printf("%d ", decrypted_ack1_again1[i]);
						}
						printf("\n");
						for(int i=0; i<32; i++)
						{
							if(decrypted_ack1_again1[i] != ack1[i])
							{
								ack_helper = 2;
							}
						}

					}

					printf("%s\n", "----------------------H2.4 ended----------------------- ");

					printf("%s\n", "-------------------------H2.5 started-------------------------");
					
					if(ack_helper == 2)
					{
						printf("%s\n", "Fails to receive encrypted ack1 ");
						if(k == 63)
						{
							k = 0;
						}
						continue;
					}

					printf("%s\n", "-------------------------H2.5 ended-------------------------");

					printf("%s\n", "-------------------------H3 starts--------------------------");

					int encrypted_ack2[32];
					printf("%s\n", "encrypting ack2 ");
					encrypter(ack2, K, encrypted_ack2);
					printf("%s\n", "Ack2 got encrypted and it is as -----");
					for (int i = 0; i < 32; ++i)
					{
						printf("%d ", encrypted_ack2[i]);
					}
					printf("\n");
					uint8 encrpyted_ack2_tosend[4];
					printf("%s\n", "Converting encrypted ack2 from binary to uint8");
					BinTouint8(encrpyted_ack2_tosend, encrypted_ack2);
					printf("%s\n", "Converted encrypted ack2 to uint8 and it is as -----");
					for (int i = 0; i < 4; ++i)
					{
						printf("%d ", encrpyted_ack2_tosend[i]);
					}
					printf("\n");
					printf("%s\n", "Sending encrypted ack2 to fpga ");
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &encrpyted_ack2_tosend[0], &error);
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &encrpyted_ack2_tosend[1], &error);
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &encrpyted_ack2_tosend[2], &error);
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &encrpyted_ack2_tosend[3], &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					printf("%s\n", "Encrypted ack2 send successfully");

					printf("%s\n", "-------------------------H3 ended----------------------------");

					printf("%s\n", "-------------------------H4 started--------------------------");

					printf("%s\n", "Calculating x and y co_ordinates ------------------");

					int co_ordinates[8];
					for (int i = 0; i < 8; ++i)
					{
						co_ordinates[i] = decrypted_pos[i+24];
					}

					int x_cord = 8*co_ordinates[3]+4*co_ordinates[2]+2*co_ordinates[1]+co_ordinates[0];
					int y_cord = 8*co_ordinates[7]+4*co_ordinates[6]+2*co_ordinates[5]+co_ordinates[4];



					printf("%s\n", "x and y co_ordinates calculated, they are as --");
					printf("%d %d ", x_cord, y_cord);
					printf("\n");
					int final_arr[8][8];
					FILE *fp = fopen("/home/shubham/Desktop/track_data.csv", "r");

					print(x_cord,y_cord,final_arr, fp);

					printf("%s\n", "Got the information for x and y co_ordinates, 64 bits filled");
					printf("%s\n", "The final 64 bit array in decrypted form --");

					for (int i = 0; i < 8; ++i)
					{
						for (int j = 0; j < 8; ++j)
						{
							printf("%d ", final_arr[i][j]);
						}
					}

					printf("\n");
					printf("%s\n", "-------------------------H4 ended--------------------------");

					printf("%s\n", "-------------------------H5 starts--------------------------");

					int p1[32], p2[32], bin1[32], bin2[32];

					for (int i = 0; i < 32; ++i)
					{
						bin1[i] = 0;
						bin2[i] = 0;
					}

					int k1 = 0;
					for(int i=0; i<4; i++)
					{
						for(int j=0; j<8; j++)
						{
							p1[k1] = final_arr[i][j];
							k1++;
						}

					}

					int k2 = 0;
					for(int i=4; i<8; i++){
						for(int j=0; j<8; j++)
						{
							p2[k2] = final_arr[i][j];
							k2++;
						}
					}

					printf("%s\n", "Encrypting first 32 bits of final array");
					encrypter(p1, K, bin1);
					printf("%s\n", "First 32 bits got encrypted , they are as ----");
					for (int i = 0; i < 32; ++i)
					{
						printf("%d ", bin1[i]);
					}
					printf("\n");
					uint8 first32_bitsTosend[4];
					printf("%s\n", "Converting first 32 bits to uint8");
					BinTouint8(first32_bitsTosend, bin1);
					printf("%s\n", "First 32 bits got converted to uint8, they are as");
					for (int i = 0; i < 4; ++i)
					{
						printf("%d ", first32_bitsTosend[i]);
					}
					printf("\n");
					printf("%s\n", "Writing on channel first 32 bits encrypted");
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &first32_bitsTosend[0], &error);
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &first32_bitsTosend[1], &error);
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &first32_bitsTosend[2], &error);
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &first32_bitsTosend[3], &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);

					printf("%s\n", "-----------------------------H5 ends------------------");
					printf("%s\n", "-----------------------------H6 starts------------------");

					printf("%s\n", "Waiting for encrypted ack1 ");
					time_t startTime, endTime;
					time(&startTime); 	

					printf("%s\n", "-----------------------------H7 starts------------------");

					double diff;
					int arb3 = 0;

					int breaker = 0;
					while(true)
					{
						time(&endTime);
						diff = difftime(endTime, startTime);
						if(diff > 256)
						{
							printf("%s\n", "Going to H2, time exceeded 256 seconds");
							k = k-1;
							arb3 = 1;
							break;
						}
						int arb = 0;
						uint8 encrypted_ack1_again2[4];
						delay(del);
						fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &encrypted_ack1_again2[0], &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						if(encrypted_ack1_again2[0] == reset) 
							{
								k--; 
								breaker++; 
							    printf("%s\n", "-------------------------RESET------------------------");
								break;
							}
						delay(del);
						fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &encrypted_ack1_again2[1], &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						if(encrypted_ack1_again2[1] == reset) 
							{
								k--; 
								printf("%s\n", "-------------------------RESET------------------------");
								breaker++; 
								break;
							}
						delay(del);
						fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &encrypted_ack1_again2[2], &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						if(encrypted_ack1_again2[2] == reset) 
							{
								k--; 
								printf("%s\n", "-------------------------RESET------------------------");
								breaker++;
								break;
							}
						delay(del);
						fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &encrypted_ack1_again2[3], &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						if(encrypted_ack1_again2[3] == reset) 
							{
								k--; 
								printf("%s\n", "-------------------------RESET------------------------");
								breaker++; 
								break;
							}
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						printf("%s\n", "Ack1 received successfully, not checked yet, it is as----");
						for (int i = 0; i < 4; ++i)
						{
							printf("%d ", encrypted_ack1_again2[i]);
						}
						printf("\n");
						int encrypted_ack1_again2_int[32];
						printf("%s\n", "Converting encrypted ack1 from uint8 to binary");
						uint8ToBin(encrypted_ack1_again2, encrypted_ack1_again2_int);
						printf("%s\n", "Converted encrypted ack1 from uint8 to binary , it is as --");
						for (int i = 0; i < 32; ++i)
						{
							printf("%d ", encrypted_ack1_again2_int[i]);
						}
						printf("\n");
						int decrypted_ack1_again2_int[32];
						printf("%s\n", "Decrypting the encrypted ack1");
						decrypter(encrypted_ack1_again2_int, K, decrypted_ack1_again2_int);
						printf("%s\n", "Encrypted ack1 decrypted successfully, it is as----");
						for (int i = 0; i < 32; ++i)
						{
							printf("%d ", decrypted_ack1_again2_int[i]);
						}
						printf("\n");
						for(int i=0; i<32; i++)
						{
							if(decrypted_ack1_again2_int[i] != ack1[i])
							{
								arb = 1;
								printf("%s\n", "Ack1 doesn't match");

							}
						}
						if(arb == 0)
							break;
					}

					if(breaker>=1) continue;

					if(arb3 == 1)
					{
						continue;
					}

					printf("%s\n", "Ack1 match successfully");

					printf("%s\n", "encrypting last 32 bits to send to fpga ");

					encrypter(p2, K, bin2);

					printf("%s\n", "Last 32 bits got encrypted , they are as in binary form");
					for (int i = 0; i < 32; ++i)
					{
						printf("%d ", bin2[i]);
					}

					uint8 last32_bitsTosend[4];
					printf("%s\n", "Converting last 32 bits to uint8");
					BinTouint8(last32_bitsTosend, bin2);
					printf("%s\n", "Last 32 bits got converted to uint8, they are as");
					for (int i = 0; i < 4; ++i)
					{
						printf("%d ", last32_bitsTosend[i]);
					}
					printf("\n");
					printf("%s\n", "Writing on channel last 32 bits encrypted");
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &last32_bitsTosend[0], &error);
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &last32_bitsTosend[1], &error);
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &last32_bitsTosend[2], &error);
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &last32_bitsTosend[3], &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					printf("%s\n", "Successfully written last 32 bits");

					printf("%s\n", "---------------------------------H7 ends---------------------");

					time_t startTime2, endTime2;
					double diff2;
					time(&startTime2);

					int breaker2 = 0;
					while(true)
					{
						time(&endTime2);
						diff2 = difftime(endTime2, startTime2);
						if(diff2 > 256)
						{
							printf("%s\n", "Going to H2, time exceeded 256 seconds");
							k = k-1;
							continue;
						}
						int arb2 = 0;
						uint8 encrypted_ack1_again3[4];
						delay(del);
						fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &encrypted_ack1_again3[0], &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						if(encrypted_ack1_again3[0] == reset) 
							{
								k--; 
								printf("%s\n", "-------------------------RESET------------------------");
								breaker2++; 
								break;
							}
						delay(del);
						fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &encrypted_ack1_again3[1], &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						if(encrypted_ack1_again3[1] == reset) 
							{
								k--; 
								breaker2++;
								printf("%s\n", "-------------------------RESET------------------------"); 
								break;
							}
						delay(del);
						fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &encrypted_ack1_again3[2], &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						if(encrypted_ack1_again3[2] == reset) 
							{
								k--; 
								breaker2++; 
								printf("%s\n", "-------------------------RESET------------------------");
								break;
							}
						delay(del);
						fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &encrypted_ack1_again3[3], &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						if(encrypted_ack1_again3[3] == reset) 
							{
								k--; 
								breaker2++;
								printf("%s\n", "-------------------------RESET------------------------");
								break;
							}
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						printf("%s\n", "Ack1 received successfully, not checked yet, it is as----");
						for (int i = 0; i < 4; ++i)
						{
							printf("%d ", encrypted_ack1_again3[i]);
						}
						printf("\n");
						int encrypted_ack1_again3_int[32];
						printf("%s\n", "Converting encrypted ack1 from uint8 to binary");
						uint8ToBin(encrypted_ack1_again3, encrypted_ack1_again3_int);
						printf("%s\n", "Converted encrypted ack1 from uint8 to binary , it is as --");
						for (int i = 0; i < 32; ++i)
						{
							printf("%d ", encrypted_ack1_again3_int[i]);
						}
						printf("\n");
						int decrypted_ack1_again3_int[32];
						printf("%s\n", "Decrypting the encrypted ack1");
						decrypter(encrypted_ack1_again3_int, K, decrypted_ack1_again3_int);
						printf("%s\n", "Encrypted ack1 decrypted successfully, it is as----");
						for (int i = 0; i < 32; ++i)
						{
							printf("%d ", decrypted_ack1_again3_int[i]);
						}
						printf("\n");
						for(int i=0; i<32; i++)
						{
							if(decrypted_ack1_again3_int[i] != ack1[i])
							{
								arb2 = 1;
								printf("%s\n", "Ack1 doesn't match");

							}
						}
						if(arb2 == 0)
							break;
					}

					if(breaker2 >= 1) continue;

					printf("\n");
					int encrypted_ack2_again[32];
					printf("%s\n", "Encrypting ack2 ");
					encrypter(ack2, K, encrypted_ack2_again);
					printf("%s\n", "Encrypted ack2 is as follows  ----");
					for (int i = 0; i < 32; ++i)
					{
						printf("%d ", encrypted_ack2_again[i]);
					}
					printf("\n");
					uint8 encrypted_ack2_again_uint8[4];
					printf("%s\n", "Converting encrypted ack2 from binary to uint8");
					BinTouint8(encrypted_ack2_again_uint8, encrypted_ack2_again);
					printf("%s\n", "Converted encrypted ack2 to uint8, it is as follows");
					for (int i = 0; i < 4; ++i)
					{
						printf("%d ", encrypted_ack2_again_uint8[i]);
					}
					printf("\n");
					printf("%s\n", "Writing encrypted ack2 to channel");
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &encrypted_ack2_again_uint8[0], &error);
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &encrypted_ack2_again_uint8[1], &error);
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &encrypted_ack2_again_uint8[2], &error);
					delay(del);
					fStatus = flWriteChannel(handle, 2*k+1, 1, &encrypted_ack2_again_uint8[3], &error);
					CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
					printf("%s\n", "Written successfully encrypted ack2, the written data is --");
					for (int i = 0; i < 4; ++i)
					{
						printf("%d ", encrypted_ack2_again_uint8[i]);
					}

		 			//////////////////////////////////////////////////////////////////////////////
					printf("\n");
					printf("%s\n", "Receiving data to update CSV file");

					int X[8] = {0,1,1,1,1,1,1,1};
					int decX = bitArrayToInt(X, 8);
					int Y[8] = {1,1,1,1,1,1,1,1};
					int decY = bitArrayToInt(Y, 8);

					uint8 direct;
					uint8 updaterFPGA[4];
					int updatesFromFPGA[32];
					int updatesForCSV[8];
					int exists, ok, dir, nxt;
					bool updateCSV=false;

					int cnt1 = 0;
					int cnt2 = 0;

					int breaker3 = 0;
					while(1)
					{	
						printf("%s", "FPGA Count " );
						cnt1++;
						printf("%d\n", cnt1);
						printf("%s\n", "Receiving Controlling signal form FPGA ");
						delay(del);
						fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &direct, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						if(direct == reset) 
							{
								k--; 
								breaker3++; 
								printf("%s\n", "-------------------------RESET------------------------");
								break;
							}
						printf("%s\n", "Controlling signal received from FPGA, it is-----");
						printf("%d\n", direct);
						printf("%s\n", "Checking the controlling singal, whether it is to update CSV data or not");
						int director = (int)direct;
						if(director==decX) break;
						if(director==decY)
						{	
							updateCSV = true;
							printf("%s\n", "Controlling signal is to update the CSV data");
							printf("%s\n", "Receiving encrypted data from the FPGA to update CSV");
							delay(del);
							fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &updaterFPGA[0], &error);
							CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
							fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
							CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
							if(updaterFPGA[0] == reset) 
							{
								k--; 
								breaker3++; 
								printf("%s\n", "-------------------------RESET------------------------");
								break;
							}
							delay(del);
							fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &updaterFPGA[1], &error);
							CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
							fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
							CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
							if(updaterFPGA[1] == reset) 
							{
								k--; 
								breaker3++; 
								printf("%s\n", "-------------------------RESET------------------------");
								break;
							}
							delay(del);
							fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &updaterFPGA[2], &error);
							CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
							fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
							CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
							if(updaterFPGA[2] == reset) 
							{
								k--; 
								breaker3++; 
								printf("%s\n", "-------------------------RESET------------------------");
								break;
							}
							delay(del);
							fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &updaterFPGA[3], &error);
							CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
							fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
							CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
							if(updaterFPGA[3] == reset) 
							{
								k--; 
								breaker3++;
								printf("%s\n", "-------------------------RESET------------------------"); 
								break;
							}

							printf("%s\n", "Encrypted Data received successfully from the FPGA, it is as in uint8 format-----");
							for (int i = 0; i < 4; ++i)
							{
								printf("%d ", updaterFPGA[3-i]);
							}
							printf("\n");

							
							printf("%s\n", "Converting uint8 data to binary");
							uint8ToBin(updaterFPGA, updatesFromFPGA);
							printf("%s\n", "Converted uint8 data to binary, it is as-----");

							for (int i = 0; i < 32; ++i)
							{
								printf("%d ", updatesFromFPGA[31-i]);
							}
							printf("\n");
							printf("%s\n", "Decrypting the data");
							int decryptedUpdates[32];
							decrypter(updatesFromFPGA, K, decryptedUpdates);
							printf("%s\n", "Data decrypted successfully, it is as----");
							for (int i = 0; i < 32; ++i)
							{
								printf("%d ", decryptedUpdates[31-i]);
							}
							printf("\n");
							printf("%s\n", "Getting ready to update CSV");
							
							
							for (int i = 0; i < 8; ++i)
								updatesForCSV[i] = decryptedUpdates[i];

							// exists = updatesForCSV[7];
							// ok = updatesForCSV[6];
							// dir = 4*updatesForCSV[5] + 2*updatesForCSV[4] + updatesForCSV[3];
							// nxt = 4*updatesForCSV[2] + 2*updatesForCSV[1] + updatesForCSV[0];

							// printf("%s\n", "The information received from FPGA");
							// printf("%s ", "Track exists   ---->   ");
							// printf("%d\n", exists);
							// printf("%s ", "Track ok   ----->   ");
							// printf("%d\n", ok);
							// printf("%s ", "Direction   ----->   ");
							// printf("%d\n", dir);
							// printf("%s ", "Next signal   ------>    ");
							// printf("%d\n", nxt);

							// printf("%s\n", "Overwriting the CSV file with the updated information");
							// writecsv(x_cord, y_cord, ok, dir, nxt);
							// printf("%s\n", "CSV file overwritten successfully");
							 break;
						}
					}

					if(breaker3 >= 1) continue;

					printf("%s\n", "-------------------------UART PART----------------------------");
					int Z[8] = {1,0,0,0,0,0,0,0};
					//int W[8] = {0,0,0,0,0,0,0,0};
					int decZ = bitArrayToInt(Z, 8);
					//int decW = bitArrayToInt(W, 8);

					uint8 direct2;
					int breaker4 = 0;

					while(1)
					{	
						printf("%s", " UART Count " );
						cnt2++;
						printf("%d\n", cnt2);
						printf("%s\n", "Waiting for UART signal ");
						delay(del);
						fStatus = flReadChannelAsyncSubmit(handle, 2*k, 1, &direct2, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						fStatus = flReadChannelAsyncAwait(handle, &recvData, &actualLength, &actualLength, &error);
						CHECK_STATUS(fStatus, FLP_LIBERR, cleanup);
						if(direct2 == reset) 
							{
								k--; 
								breaker4++; 
								printf("%s\n", "-------------------------RESET------------------------");
								break;
							}
						printf("%s\n", "Controlling signal received from UART, it is-----");
						printf("%d\n", direct2);
						//printf("%s\n", "Checking the controlling singal, whether it is to update CSV data or not");
						int director = (int)direct2;
						if(director==decZ) break;
						// if(director==decW)
						// {
						// 	printf("%s\n", "Controlling signal is to update the CSV data");
						// 	printf("%s\n", "Receiving encrypted data from the UART update CSV");
						// 	delay(del);
						// 	fStatus = flReadChannel(handle, 2*k, 1, &direct2, &error);
						// 	if(direct2 == reset) 
						// 	{
						// 		k--; 
						// 		breaker4++;
						// 		printf("%s\n", "-------------------------RESET------------------------"); 
						// 		break;
						// 	}
		 			// 		// delay(del);
		 			// 		// fStatus = flReadChannel(handle, 2*k, 1, &updaterUART[1], &error);
		 			// 		// delay(del);
		 			// 		// fStatus = flReadChannel(handle, 2*k, 1, &updaterUART[2], &error);
		 			// 		// delay(del);
		 			// 		// fStatus = flReadChannel(handle, 2*k, 1, &updaterUART[3], &error);

		 			// 		// printf("%s\n", "Encrypted Data received successfully from the UART, it is as in uint8 format-----");
		 			// 		// for (int i = 0; i < 4; ++i)
		 			// 		// {
		 			// 		// 	printf("%d ", updaterUART[i]);
		 			// 		// }
						// 	printf("%d\n", direct2);

						// 	int updatesFromUART[8];
						// 	printf("%s\n", "Converting uint8 data to binary");
						// 	decToBinary(direct2, updatesFromUART, 8);
						// 	printf("%s\n", "Converted uint8 data to binary, it is as-----");

						// 	for (int i = 0; i < 8; ++i)
						// 	{
						// 		printf("%d ", updatesFromUART[7-i]);
						// 	}
						// 	printf("\n");
		 			// 		// printf("%s\n", "Decrypting the data");
		 			// 		// int decryptedUpdates[32];
		 			// 		// decrypter(updatesFromUART, K, decryptedUpdates);
		 			// 		// printf("%s\n", "Data decrypted successfully, it is as----");
		 			// 		// for (int i = 0; i < 32; ++i)
		 			// 		// {
		 			// 		// 	printf("%d ", decryptedUpdates[i]);
		 			// 		// }
		 			// 		// printf("\n");
						// 	printf("%s\n", "Getting ready to update CSV");
		 			// 		// int updatesForCSV[8];
						// 	int exists, ok, dir, nxt;
		 			// 		// for (int i = 0; i < 8; ++i)
		 			// 		// 	updatesForCSV[i] = decryptedUpdates[i];

						// 	exists = updatesFromUART[7];
						// 	ok = updatesFromUART[6];
						// 	dir = 4*updatesFromUART[5] + 2*updatesFromUART[4] + updatesFromUART[3];
						// 	nxt = 4*updatesFromUART[2] + 2*updatesFromUART[1] + updatesFromUART[0];

						// 	printf("%s\n", "The information received from UART");
						// 	printf("%s ", "Track exists   ---->   ");
						// 	printf("%d\n", exists);
						// 	printf("%s ", "Track ok   ----->   ");
						// 	printf("%d\n", ok);
						// 	printf("%s ", "Direction   ----->   ");
						// 	printf("%d\n", dir);
						// 	printf("%s ", "Next signal   ------>    ");
						// 	printf("%d\n", nxt);

						// 	printf("%s\n", "Overwriting the CSV file with the updated information");
						// 	writecsv(x_cord, y_cord, ok, dir, nxt);
						// 	printf("%s\n", "CSV file overwritten successfully");
						// 	break;
						// }
					}

					if(breaker4 >= 1)
					{
						continue;
					}
					
					if(updateCSV==true){
						exists = updatesForCSV[7];
						ok = updatesForCSV[6];
						dir = 4*updatesForCSV[5] + 2*updatesForCSV[4] + updatesForCSV[3];
						nxt = 4*updatesForCSV[2] + 2*updatesForCSV[1] + updatesForCSV[0];

						printf("%s\n", "The information received from FPGA");
						printf("%s", "The x_cord ----->");
						printf("%d\n", x_cord);
						printf("%s", "The y_cord ----->");
						printf("%d\n", y_cord);
						printf("%s ", "Track exists   ---->   ");
						printf("%d\n", exists);
						printf("%s ", "Track ok   ----->   ");
						printf("%d\n", ok);
						printf("%s ", "Direction   ----->   ");
						printf("%d\n", dir);
						printf("%s ", "Next signal   ------>    ");
						printf("%d\n", nxt);
						printf("%s\n", "Overwriting the CSV file with the updated information");
						writecsv(x_cord, y_cord, ok, dir, nxt);
						printf("%s\n", "CSV file overwritten successfully");
								
			 			//delay(32000000);
						
						updateCSV=false;						
					}
					k = k-1;
					delay(5*del);
					continue;


				}

			} 
			else 
			{
				fprintf(stderr, "The FPGALink device at %s is not ready to talk - did you forget --xsvf?\n", vp);
				FAIL(FLP_ARGS, cleanup);
			}
		} 
		else 
		{
			fprintf(stderr, "Shell requested but device at %s does not support CommFPGA\n", vp);
			FAIL(FLP_ARGS, cleanup);
		}


	}

//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////

	cleanup:
	free((void*)line);
	flClose(handle);
	if ( error ) {
		fprintf(stderr, "%s\n", error);
		flFreeError(error);
	}
	return retVal;
}


