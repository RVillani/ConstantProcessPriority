#pragma once

#include <string>

class SettingsManager
{
public:

	static const char JsonFile_Name[];
	static const char JsonFile_NamesField[];
	static const char JsonFile_PriorityField[];
	static const char JsonFile_StartMinimized[];

	std::wstring Names;
	int Priority;
	bool bStartMinimized;

	SettingsManager();

	void LoadSettings(bool& bLoadedPrioritySettings, bool& bLoadedMinimizedState);
	void SaveSettings();
};