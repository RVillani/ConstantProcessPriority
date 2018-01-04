#include "stdafx.h"
#include "SettingsManager.h"

#include <fstream>
#include "json.hpp"

// static initializers
const char SettingsManager::JsonFile_Name[] = "ConstantProcessPrioritySettings.json";
const char SettingsManager::JsonFile_NamesField[] = "processList";
const char SettingsManager::JsonFile_PriorityField[] = "priority";
const char SettingsManager::JsonFile_StartMinimized[] = "startMinimized";


SettingsManager::SettingsManager() : Names(L""), Priority(0), bStartMinimized(false) {}

void SettingsManager::LoadSettings(bool& bLoadedPrioritySettings, bool& bLoadedMinimizedState)
{
	bLoadedPrioritySettings = bLoadedMinimizedState = false;

	std::ifstream jsonFile(JsonFile_Name);
	if (jsonFile.is_open())
	{
		nlohmann::json json;
		jsonFile >> json;

		// check required fields for process priority control
		if (json.find(JsonFile_NamesField) != json.end()
			&& json.find(JsonFile_PriorityField) != json.end())
		{
			std::string names(json[JsonFile_NamesField].get<std::string>());
			Names = std::wstring(names.begin(), names.end()).c_str();
			Priority = json[JsonFile_PriorityField].get<int>();
			bLoadedPrioritySettings = true;
		}

		// check field for minimized state
		if (json.find(JsonFile_StartMinimized) != json.end())
		{
			bStartMinimized = json[JsonFile_StartMinimized].get<bool>();
			bLoadedMinimizedState = true;
		}
	}
}

void SettingsManager::SaveSettings()
{
	nlohmann::json json;
	json[JsonFile_NamesField] = std::string(Names.begin(), Names.end());
	json[JsonFile_PriorityField] = Priority;
	json[JsonFile_StartMinimized] = bStartMinimized;

	std::ofstream jsonFile(JsonFile_Name);
	if (jsonFile.is_open())
	{
		jsonFile << std::setw(4) << json << std::endl;
		jsonFile.close();
	}
}