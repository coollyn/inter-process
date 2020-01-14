#include <Windows.h>
#include <iostream>
#include <thread>
#include <mutex>
#include <string>
#include <queue>

const std::string	readPipeName = "\\\\.\\pipe\\mynamedpipe";
const std::string	writePipeName = "\\\\.\\pipe\\mynamedpipe2";

std::thread trdWrite;
HANDLE hdPipeWrite;

std::thread trdRead;
HANDLE hdPipeRead;

void runWritePipe();
void runReadPipe();

#define BUFFERSIZE 512
//BOOL m_bClintConnected;
std::queue<std::string>	m_queDataToWrite;
std::string m_sReadData;
std::condition_variable cv;
std::mutex m;

int index = 0;
void productData();
std::thread trdProduce;

int main(int argc, char *argv[])
{
	std::cout << "-------\nthis is child, argvs from parent is as below:" << std::endl;

	auto cmd = ::GetCommandLine();

	auto parentH=std::stoi(cmd);
	auto handle = (HANDLE)parentH;

	trdRead = std::thread(&runReadPipe);
	trdRead.detach();
	trdWrite = std::thread(&runWritePipe);
	trdWrite.detach();
	trdProduce = std::thread(&productData);
	trdProduce.detach();
	
	DWORD ret;
	ret = ::WaitForSingleObject(handle, INFINITE);
	::CloseHandle(handle);
}

void runReadPipe()
{
	while (true)
	{
		hdPipeRead = CreateFile(
			readPipeName.c_str(),   // pipe name 
			GENERIC_READ,  // read access 
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

	  // Break if the pipe handle is valid. 

		if (hdPipeRead != INVALID_HANDLE_VALUE)
			break;

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 

		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			std::cout << "read pipe: Could not open pipe. GLE=" << GetLastError() << std::endl;
			return;
		}

		// All pipe instances are busy, so wait for 20 seconds. 

		if (!WaitNamedPipe(readPipeName.c_str(), 20000))
		{
			std::cout << "read pipe: Could not open pipe: 20 second wait timed out." << std::endl;
			return;
		}
	}

	TCHAR szBuffer[BUFFERSIZE];
	DWORD cbBytes;

	while (true)
	{
		std::cout << "start to read data\n";
		BOOL bResult = ReadFile(
			hdPipeRead,              // handle to pipe 
			szBuffer,             // buffer to receive data 
			sizeof(szBuffer),     // size of buffer 
			&cbBytes,             // number of bytes read 
			NULL);                // not overlapped I/O 

		if (bResult && 0 != cbBytes)
		{
			m_sReadData = szBuffer;
			std::cout << "-----------" << m_sReadData << std::endl;
		}
		else
		{
			auto error = GetLastError();
			std::cout << error << std::endl;
			//	break;
		}
		std::cout << "end read data\n";
	}

	//std::chrono::milliseconds mi(4000);
	//std::this_thread::sleep_for(mi);

	//DWORD total_available_bytes;
	//if (FALSE == PeekNamedPipe(hdPipeRead,
	//	0,
	//	0,
	//	0,
	//	&total_available_bytes,
	//	0))
	//{
	//	std::cout << "no data\n";
	//}
	//else if (total_available_bytes > 0)
	//{
	//	std::cout << "has data\n";
	//}

	//	std::cout << "start to read data\n";
	//	BOOL bResult = ReadFile(
	//		hdPipeRead,              // handle to pipe 
	//		szBuffer,             // buffer to receive data 
	//		sizeof(szBuffer),     // size of buffer 
	//		&cbBytes,             // number of bytes read 
	//		NULL);                // not overlapped I/O 

	//	if (bResult && 0 != cbBytes)
	//	{
	//		m_sReadData = szBuffer;
	//		std::cout << "-----------" << m_sReadData << std::endl;
	//	}
	//	else
	//	{
	//		auto error = GetLastError();
	//		std::cout << error << std::endl;
	//		//	break;
	//	}
	//	std::cout << "end read data\n";


	//	if (FALSE == PeekNamedPipe(hdPipeRead,
	//		0,
	//		0,
	//		0,
	//		&total_available_bytes,
	//		0))
	//	{
	//		std::cout << "no data\n";
	//	}
	//	else if (total_available_bytes > 0)
	//	{
	//		std::cout << "has data\n";
	//	}

	//	std::cout << "start to read data\n";
	//	bResult = ReadFile(
	//		hdPipeRead,              // handle to pipe 
	//		szBuffer,             // buffer to receive data 
	//		sizeof(szBuffer),     // size of buffer 
	//		&cbBytes,             // number of bytes read 
	//		NULL);                // not overlapped I/O 

	//	if (bResult && 0 != cbBytes)
	//	{
	//		m_sReadData = szBuffer;
	//		std::cout << "-----------" << m_sReadData << std::endl;
	//	}
	//	else
	//	{
	//		auto error = GetLastError();
	//		std::cout << error << std::endl;
	//		//	break;
	//	}
	//	std::cout << "end read data\n";
}

void runWritePipe()
{
	while (true)
	{
		hdPipeWrite = CreateFile(
			writePipeName.c_str(),   // pipe name 
			GENERIC_WRITE,	// write access 
			0,              // no sharing 
			NULL,           // default security attributes
			OPEN_EXISTING,  // opens existing pipe 
			0,              // default attributes 
			NULL);          // no template file 

	  // Break if the pipe handle is valid. 
		if (hdPipeWrite != INVALID_HANDLE_VALUE)
			break;

		// Exit if an error other than ERROR_PIPE_BUSY occurs. 

		if (GetLastError() != ERROR_PIPE_BUSY)
		{
			std::cout << "read pipe: Could not open pipe. GLE=" << GetLastError() << std::endl;
			return;
		}

		// All pipe instances are busy, so wait for 20 seconds. 

		if (!WaitNamedPipe(writePipeName.c_str(), 20000))
		{
			std::cout << "read pipe: Could not open pipe: 20 second wait timed out." << std::endl;
			return;
		}
	}


	while (true)
	{
		std::unique_lock<std::mutex> lk(m);
		cv.wait(lk, [] {return !m_queDataToWrite.empty(); });
		lk.unlock();

		DWORD cbWritten;
		auto data = m_queDataToWrite.front();
		m_queDataToWrite.pop();

		std::cout << "start to write data-----------" << data << std::endl;
		BOOL fSuccess = WriteFile(
			hdPipeWrite,			// handle to pipe 
			data.c_str(),     // buffer to write from 
			data.length() + 1,	// number of bytes to write 
			&cbWritten,			// number of bytes written 
			NULL);				// not overlapped I/O 

		if (!fSuccess || data.length() + 1 != cbWritten)
		{
			std::cout << "WriteFile failed, GLE=" << GetLastError() << std::endl;
			break;
		}
		else
		{
			std::cout << "end write data:\n";
		}
	}
}

void productData()
{
	char buf[100] = "";

	while (true)
	{
	//	std::unique_lock<std::mutex> lk(m);
		m_queDataToWrite.push("C" + std::to_string(++index));
	//	lk.unlock();

		cv.notify_one();

		std::chrono::milliseconds mi(4000);
		std::this_thread::sleep_for(mi);
	}
}