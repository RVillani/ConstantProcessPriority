#pragma once

#include <thread>
#include <mutex>
#include <TlHelp32.h>
#include <string.h>
#include <vector>

class PriorityClassControl
{
public:
	void(*MessagesCallback)(const wchar_t*);

	PriorityClassControl(const wchar_t *Names, const wchar_t *Priority);
	PriorityClassControl();
	~PriorityClassControl();

	void Setup(const wchar_t *Names, int Priority);
	void Setup(const wchar_t *Names, const wchar_t *Priority);
	void GetSettings(std::wstring &Names, int &Priority);

private:
	DWORD PriorityClass;
	std::vector<std::wstring> ProcessNames;

	std::mutex Mutex;
	bool bKeepWorking;
	class std::thread Worker;

	void SearchProcesses();
	void SetProcessPriority(PROCESSENTRY32 entry);
};
