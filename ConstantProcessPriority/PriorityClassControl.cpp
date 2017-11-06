#include "stdafx.h"
#include "PriorityClassControl.h"

using namespace std;


PriorityClassControl::PriorityClassControl()
	:bKeepWorking(true)
{
	Worker = thread(&PriorityClassControl::SearchProcesses, this);
}

PriorityClassControl::PriorityClassControl(const wchar_t *Names, const wchar_t *Priority)
	: bKeepWorking(true)
{
	Setup(Names, Priority);
	Worker = thread(&PriorityClassControl::SearchProcesses, this);
}

void PriorityClassControl::Setup(const wchar_t *Names, const wchar_t *Priority)
{
	// parse priority
	unsigned long priority = wcstol(Priority, nullptr, 10);
	priority = min(max(priority, 0), 3);

	Setup(Names, priority);
}

void PriorityClassControl::Setup(const wchar_t *Names, int Priority)
{
	lock_guard<mutex> lg(Mutex);

	switch (Priority)
	{
	case 0: PriorityClass = BELOW_NORMAL_PRIORITY_CLASS; break;
	case 1: PriorityClass = NORMAL_PRIORITY_CLASS; break;
	case 2: PriorityClass = ABOVE_NORMAL_PRIORITY_CLASS; break;
	case 3: PriorityClass = HIGH_PRIORITY_CLASS; break;
	}

	// parse names
	ProcessNames.clear();

	wchar_t chars[256] = { '\0' };
	UINT currNamesChar = 0;
	UINT currNameChar = 0;
	while (Names[currNamesChar] != '\0')
	{
		if (Names[currNamesChar] == ',')
		{
			ProcessNames.push_back(wstring(chars));
			memset(chars, '\0', sizeof(chars));
			currNameChar = 0;
		}
		else
		{
			chars[currNameChar++] = Names[currNamesChar];
		}

		++currNamesChar;
	}
	ProcessNames.push_back(wstring(chars));
}

PriorityClassControl::~PriorityClassControl()
{
	bKeepWorking = false;
	Worker.join();
}

void PriorityClassControl::SearchProcesses()
{
	while (bKeepWorking)
	{
		PROCESSENTRY32 entry;
		entry.dwSize = sizeof(PROCESSENTRY32);

		HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, NULL);
		if (snapshot == INVALID_HANDLE_VALUE)
		{
			if (MessagesCallback) MessagesCallback(L"Failed to get process list");
			this_thread::sleep_for(500ms);
			continue;
		}

		for (BOOL validProcess = Process32First(snapshot, &entry); validProcess; validProcess = Process32Next(snapshot, &entry))
		{
			SetProcessPriority(entry);
		}

		CloseHandle(snapshot);

		this_thread::sleep_for(333ms);
	}
}

void PriorityClassControl::SetProcessPriority(PROCESSENTRY32 entry)
{
	lock_guard<mutex> lg(Mutex);
	for (const wstring &name : ProcessNames)
	{
		if (wcscmp(entry.szExeFile, name.c_str()) == 0)
		{
			HANDLE hProcess = OpenProcess(PROCESS_SET_INFORMATION, FALSE, entry.th32ProcessID);
			if (hProcess)
			{
				if (SetPriorityClass(hProcess, PriorityClass) != 0)
				{
#ifdef _DEBUG
					wprintf(TEXT("-- Changed priority of %s to 0x%x\n"), name.c_str(), PriorityClass);
#endif
				}

				CloseHandle(hProcess);
			}
			else
			{
				wchar_t *buffer = new wchar_t[MAX_PATH];
				_snwprintf_s(buffer, MAX_PATH, MAX_PATH, TEXT("Failed to open process %s (0x%x)\n"), entry.szExeFile, GetLastError());
				if (MessagesCallback) MessagesCallback(buffer);
				delete[] buffer;

#ifdef _DEBUG
				wprintf(TEXT("Failed to open process %s (0x%x)\n"), entry.szExeFile, GetLastError());
#endif
			}

			break;
		}
	}
}

void PriorityClassControl::GetSettings(wstring &Names, int &Priority)
{
	Names.clear();
	for (const wstring &name : ProcessNames)
	{
		Names += name + L",";
	}
	Names.pop_back();

	switch (PriorityClass)
	{
	case BELOW_NORMAL_PRIORITY_CLASS:	Priority = 0; break;
	case NORMAL_PRIORITY_CLASS:			Priority = 1; break;
	case ABOVE_NORMAL_PRIORITY_CLASS:	Priority = 2; break;
	case HIGH_PRIORITY_CLASS:			Priority = 3; break;
	}
}