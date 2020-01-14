#include <Windows.h>
#include <thread>
#include <mutex>
#include <iostream>
#include <queue>
#include <string>

const std::string	writePipeName = "\\\\.\\pipe\\mynamedpipe";
const std::string	readPipeName = "\\\\.\\pipe\\mynamedpipe2";

std::thread trdWrite;
HANDLE hdPipeWrite;

std::thread trdRead;
HANDLE hdPipeRead;

void runWritePipe();
void runReadPipe();

#define BUFFERSIZE 512
//BOOL m_bClintConnected;
std::queue<std::string>	m_queDataToWrite;
std::string			m_sReadData;
std::condition_variable cv;
std::mutex m;

int index = 0;
void productData();
std::thread trdProduce;

void CreateProc();
STARTUPINFO			m_procStartupInfo;
PROCESS_INFORMATION	m_procInfo;
const std::string	c_sProcName = "C:\\Users\\Yongcheng Wu\\source\\repos\\doublePipeClient\\Debug\\doublePipeClient.exe";


int main()
{
	std::cout << "this is parent, create child process and transmit argvs-------------" << std::endl;

	trdWrite = std::thread(runWritePipe);
	trdWrite.detach();
	trdRead = std::thread(runReadPipe);
	trdRead.detach();
	trdProduce = std::thread(&productData);
	trdProduce.detach();

	CreateProc();

	::WaitForSingleObject(m_procInfo.hProcess, INFINITE);
	::CloseHandle(m_procInfo.hThread);
	::CloseHandle(m_procInfo.hProcess);
}

void CreateProc()
{
	auto h = ::GetCurrentProcess();

	HANDLE target;
	if (!::DuplicateHandle(::GetCurrentProcess(), ::GetCurrentProcess(), ::GetCurrentProcess(), &target, 0, TRUE, DUPLICATE_SAME_ACCESS))
		return;

	auto sHandle = std::to_string((UINT)target);

	char *p = new char[100]{0};
	memcpy(p, sHandle.c_str(), sHandle.length());
	std::cout << "duplicated handle value: " << p << std::endl;

	ZeroMemory(&m_procStartupInfo, sizeof(m_procStartupInfo));
	m_procStartupInfo.cb = sizeof(m_procStartupInfo);
	ZeroMemory(&m_procInfo, sizeof(m_procInfo));

	// Start the child process. 
	if (!CreateProcess(
		c_sProcName.c_str(),			// module name
		p,							// Command line
		NULL,							// Process handle not inheritable
		NULL,							// Thread handle not inheritable
		TRUE,							// Set handle inheritance to FALSE
		CREATE_NEW_CONSOLE,				// No creation flags
		NULL,							// Use parent's environment block
		NULL,							// Use parent's starting directory 
		&m_procStartupInfo,				// Pointer to STARTUPINFO structure
		&m_procInfo)					// Pointer to PROCESS_INFORMATION structure
		)
	{

		auto error = GetLastError();
		std::cout << "create proc error: " << error << std::endl;
	}

	delete p;
}

void runWritePipe()
{
	hdPipeWrite = CreateNamedPipe(
		writePipeName.c_str(),      // pipe name 
		PIPE_ACCESS_OUTBOUND,       // read/write access 
		PIPE_TYPE_MESSAGE |       // message type pipe 
		PIPE_READMODE_MESSAGE |   // message-read mode 
		PIPE_WAIT,                // blocking mode 
		PIPE_UNLIMITED_INSTANCES, // max. instances  
		BUFFERSIZE*sizeof(char),				// output buffer size 
		BUFFERSIZE*sizeof(char),				// input buffer size 
		0,                        // client time-out 
		NULL);                    // default security attribute 

	if (hdPipeWrite == INVALID_HANDLE_VALUE)
	{
		std::cout << "write pipe: CreateNamedPipe failed, GLE=" << GetLastError() << std::endl;
		return;
	}

	BOOL m_bClintConnected = ConnectNamedPipe(hdPipeWrite, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
	if (!m_bClintConnected)
	{
		CloseHandle(hdPipeWrite);
		return;
	}
	std::cout << "writepipe: connected this\n";


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
			std::cout << "end write data:" << data << std::endl;
		}
	}

		////DWORD cbWritten;
		////std::string data = "S1";

		////std::cout << "start to write data-----------" << data << std::endl;
		////BOOL fSuccess = WriteFile(
		////	hdPipeWrite,			// handle to pipe 
		////	data.c_str(),     // buffer to write from 
		////	data.length() + 1,	// number of bytes to write 
		////	&cbWritten,			// number of bytes written 
		////	NULL);				// not overlapped I/O 

		////if (!fSuccess || data.length() + 1 != cbWritten)
		////{
		////	std::cout << "WriteFile failed, GLE=" << GetLastError() << std::endl;
		////}
		////else
		////{
		////	std::cout << "end write data:" << data << std::endl;
		////}
		////FlushFileBuffers(hdPipeWrite);

		////data = "S2";
		////std::cout << "start to write data-----------" << data << std::endl;
		////fSuccess = WriteFile(
		////	hdPipeWrite,			// handle to pipe 
		////	data.c_str(),     // buffer to write from 
		////	data.length() + 1,	// number of bytes to write 
		////	&cbWritten,			// number of bytes written 
		////	NULL);				// not overlapped I/O 

		////if (!fSuccess || data.length() + 1 != cbWritten)
		////{
		////	std::cout << "WriteFile failed, GLE=" << GetLastError() << std::endl;
		////}
		////else
		////{
		////	std::cout << "end write data:" << data << std::endl;
		////}
		////FlushFileBuffers(hdPipeWrite);

		////data = "S3";
		////std::cout << "start to write data-----------" << data << std::endl;
		////fSuccess = WriteFile(
		////	hdPipeWrite,			// handle to pipe 
		////	data.c_str(),     // buffer to write from 
		////	data.length() + 1,	// number of bytes to write 
		////	&cbWritten,			// number of bytes written 
		////	NULL);				// not overlapped I/O 

		////if (!fSuccess || data.length() + 1 != cbWritten)
		////{
		////	std::cout << "WriteFile failed, GLE=" << GetLastError() << std::endl;
		////}
		////else
		////{
		////	std::cout << "end write data:" << data << std::endl;
		////}
		////FlushFileBuffers(hdPipeWrite);
}

void runReadPipe()
{
	hdPipeRead = CreateNamedPipe(
		readPipeName.c_str(),      // pipe name 
		PIPE_ACCESS_INBOUND,       // read/write access 
		PIPE_TYPE_MESSAGE |       // message type pipe 
		PIPE_READMODE_MESSAGE |   // message-read mode 
		PIPE_WAIT,                // blocking mode 
		PIPE_UNLIMITED_INSTANCES, // max. instances  
		BUFFERSIZE * sizeof(char),				// output buffer size 
		BUFFERSIZE * sizeof(char),				// input buffer size 
		0,                        // client time-out 
		NULL);                    // default security attribute 

	if (hdPipeRead == INVALID_HANDLE_VALUE)
	{
		std::cout << "read pipe: CreateNamedPipe failed, GLE=" << GetLastError() << std::endl;
		return;
	}

	BOOL m_bClintConnected = ConnectNamedPipe(hdPipeRead, NULL) ? TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);
	if (!m_bClintConnected)
	{
		CloseHandle(hdPipeRead);
		return;
	}
	std::cout << "readpipe: connected this\n";


	TCHAR szBuffer[BUFFERSIZE];
	DWORD cbBytes;

	while(true)
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
			std::cout << "end read data-----------" << m_sReadData << std::endl;
		}
		else
		{
			auto error = GetLastError();
			std::cout << "readfile failed, GLE=" << error << std::endl;
				break;
		}
	}
}

void productData()
{
	char buf[100] = "";

	while (true)
	{	
	//	std::unique_lock<std::mutex> lk(m);
		m_queDataToWrite.push("S" + std::to_string(++index));
	//	lk.unlock();

		cv.notify_one();

		std::chrono::milliseconds mi(2000);
		std::this_thread::sleep_for(mi);

		//test crash
		//static int index = 0;
		//if (index++ == 4)
		//{
		//	char p[3];
		//	delete p;
		//	p[100] = 100;
		//}
	}
}