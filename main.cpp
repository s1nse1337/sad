#include <iostream>
#include <fstream>
#include <cstdlib>
#include <vector>
#include <string>
#include <direct.h>
#include <windows.h>
#include <tlhelp32.h>
#include <chrono>
#include <thread>
#include <random>
#include "flux.hpp"
#include "skStr.h"
#include <curl/curl.h>
#include <curl/easy.h>
#include <fstream>
// invalid imput letter
#define NOMINMAX
#include <limits>
#include <windows.h>
#ifdef max
#undef max
#endif
// icon
#define IDI_ICON1                       107

// Функция для записи данных
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}


// Функция для загрузки файла через GitHub API
std::string downloadFileFromGitHubAPI(const std::string& url, const std::string& token) {
    CURL* curl;
    CURLcode res;
    std::string readBuffer;

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: token " + token).c_str());
        headers = curl_slist_append(headers, "Accept: application/vnd.github.v3.raw");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

        // Опции SSL
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "CURL error: " << curl_easy_strerror(res) << std::endl;
        }
        else {
            long http_code = 0;
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
            if (http_code != 200) {
                std::cerr << "HTTP error: " << http_code << std::endl;
            }
        }

        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
    }

    return readBuffer;
}
// Function to get current UTC time
std::time_t getCurrentUTCTime() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::tm* utc_tm = std::gmtime(&now_c);
    return std::mktime(utc_tm);
}

// Adjust the time to UTC+2
std::time_t getCurrentUTCPlus2Time() {
    return getCurrentUTCTime() + 2 * 60 * 60; // Add 2 hours
}

#include <sstream>
#include <ctime>
#include <iomanip>

std::string get_hwid(); // Используемая ранее функция

bool processKey(const std::string& key, std::string& fileContent, std::string& remainingDays) {
    if (fileContent.find("<!DOCTYPE html>") != std::string::npos) {
        std::cerr << "Error: Received HTML content instead of keys file. Check your token or URL." << std::endl;
        return false;
    }

    std::istringstream iss(fileContent);
    std::string line;
    bool keyFound = false;

    while (std::getline(iss, line)) {
        std::istringstream lineStream(line);
        std::string fileKey, fileHWID, expiryDate;

        std::getline(lineStream, fileKey, '|');
        std::getline(lineStream, fileHWID, '|');
        std::getline(lineStream, expiryDate);

        fileKey.erase(fileKey.find_last_not_of(" \n\r\t") + 1);
        fileKey.erase(0, fileKey.find_first_not_of(" \n\r\t"));

        if (key == fileKey) {
            keyFound = true;

            std::tm tm = {};
            std::istringstream ss(expiryDate);
            ss >> std::get_time(&tm, "%Y-%m-%d");

            std::time_t currentTime = getCurrentUTCPlus2Time();

            if (std::difftime(std::mktime(&tm), currentTime) < 0) {
                std::cerr << "Key has expired." << std::endl;
                return false;
            }

            remainingDays = std::to_string((std::mktime(&tm) - currentTime) / (60 * 60 * 24));
            return true;
        }
    }

    if (!keyFound) {
        std::cerr << "Key not found." << std::endl;
        return false;
    }

    return true;
}
void uploadKeysFile(const std::string& url, const std::string& token, const std::string& content) {
    CURL* curl;
    CURLcode res;

    curl = curl_easy_init();
    if (curl) {
        struct curl_slist* headers = NULL;
        headers = curl_slist_append(headers, ("Authorization: Bearer " + token).c_str());
        headers = curl_slist_append(headers, "Accept: application/vnd.github.v3.raw");

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, content.c_str());
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

        // убрать нахуй
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            std::cerr << "Failed to upload file: " << curl_easy_strerror(res) << std::endl;
        }
        curl_easy_cleanup(curl);
    }
}


// Function to get HWID (hardware ID)
std::string get_hwid() {
    DWORD volumeSerialNumber;
    if (!GetVolumeInformationW(L"C:\\", NULL, 0, &volumeSerialNumber, NULL, NULL, NULL, 0)) {
        throw std::runtime_error("Failed to get HWID");
    }
    return std::to_string(volumeSerialNumber);
}

std::string getTempDirectory() {
    const char* tempDir = std::getenv(skCrypt("TEMP").decrypt());
    if (tempDir == nullptr) {
        tempDir = skCrypt("C:\\Temp").decrypt(); // Default directory if TEMP environment variable is not set
    }
    return std::string(tempDir) + skCrypt("\\Gorin").decrypt();
}

std::string getConfigDirectory() {
    return skCrypt("C:\\comref").decrypt();
}

std::string getConfigFilePath() {
    return getConfigDirectory() + skCrypt("\\config.cfg").decrypt();
}

// Function to save the configuration to a file
bool loadConfig(std::string& prefireBind, std::string& retakeBindBuilding, std::string& retakeTrigger, std::string& color1, std::string& color2, std::string& licenseKey, bool& file5Opened, std::string& fastLootBind, std::string& fastLootTake) {
    std::ifstream configFile(getConfigFilePath());
    if (!configFile) {
        return false; // Config file not found
    }

    std::getline(configFile, licenseKey); // License key should be the first line
    std::getline(configFile, prefireBind);
    std::getline(configFile, retakeBindBuilding);
    std::getline(configFile, retakeTrigger);
    std::getline(configFile, color1);
    std::getline(configFile, color2);
    std::getline(configFile, fastLootBind);
    std::getline(configFile, fastLootTake);
    std::string file5OpenedStr;
    std::getline(configFile, file5OpenedStr);
    file5Opened = (file5OpenedStr == "1337");

    return true;
}

void saveConfig(const std::string& prefireBind, const std::string& retakeBindBuilding, const std::string& retakeTrigger, const std::string& color1, const std::string& color2, const std::string& licenseKey, bool file5Opened, const std::string& fastLootBind, const std::string& fastLootTake) {
    std::ofstream configFile(getConfigFilePath());
    if (!configFile) {
        std::cerr << "Failed to open config file for writing: " << std::endl;
        return;
    }

    configFile << licenseKey << std::endl; // Save license key as the first line
    configFile << prefireBind << std::endl;
    configFile << retakeBindBuilding << std::endl;
    configFile << retakeTrigger << std::endl;
    configFile << color1 << std::endl;
    configFile << color2 << std::endl;
    configFile << fastLootBind << std::endl;
    configFile << fastLootTake << std::endl;
    configFile << (file5Opened ? "1337" : "0") << std::endl; // Save file5Opened status
}

// Fixed macro names
const std::string prefireMacroName = skCrypt("WWkjrUFwwiiasdasjd431j4eCfNwyg").decrypt();
const std::string retakeMacroName = skCrypt("cxD6tTMdYAJhQf1337").decrypt();
const std::string colorPickerMacroName = skCrypt("YourWallColor").decrypt();
const std::string BatAHKName = skCrypt("jasdghgasu22wnldsJhahafhpOASJ").decrypt();
const std::string FastLootName = skCrypt("uubdsdfkh228SINSE01ssdjh2").decrypt();

// Function to create directory and macro files
void createDirectoryAndFiles(const std::string& prefireBind, const std::string& retakeBindBuilding, const std::string& retakeTrigger, const std::string& color1, const std::string& color2, const std::string& FastLootTake, const std::string& FastLootBind, bool file5Opened) {
    std::string dirPath = getTempDirectory();
    if (_mkdir(dirPath.c_str()) != 0 && errno != EEXIST) {
        std::cerr << skCrypt("Failed to create directory: ").decrypt() << std::endl;
        return;
    }

    // Creating prefire macro file
    std::ofstream file1(dirPath + "\\" + prefireMacroName + ".ahk");
    if (!file1) {
        std::cerr << skCrypt("Failed to open file for writing: ").decrypt() << std::endl;
        return;
    }
    file1 << R"(
#Persistent
#SingleInstance Force
SetBatchLines, -1
ListLines, Off
#NoEnv
#NoTrayIcon
#KeyHistory 0
Process, Priority, , A
SetBatchLines, -1
SetKeyDelay, -1, -1
SetMouseDelay, -1
SetDefaultMouseSpeed, 0
SetWinDelay, -1
SetControlDelay, -1
SendMode Input
Suspend, Off

; Hex color values converted to RGB
color1 := ")" << color1 << R"("
color2 := ")" << color2 << R"("

~$)" << prefireBind << R"(:: 
    screenWidth := A_ScreenWidth
    screenHeight := A_ScreenHeight
    searchWidth := 1500
    searchHeight := 650
    startX := (screenWidth - searchWidth) // 2
    startY := (screenHeight - searchHeight) // 2
    endX := startX + searchWidth
    endY := startY + searchHeight

    While GetKeyState(")" << prefireBind << R"(", "P")
    {
        PixelSearch, Px, Py, startX, startY, endX, endY, color1, 0, Fast RGB
        if ErrorLevel = 0
            continue

        PixelSearch, Px, Py, startX, startY, endX, endY, color2, 0, Fast RGB
        if ErrorLevel = 0
            continue

        Click
    }
return
)";
    file1.close();

    // Creating retake macro file
    std::ofstream file2(dirPath + "\\" + retakeMacroName + ".ahk");
    if (!file2) {
        std::cerr << skCrypt("Failed to open file for writing: ").decrypt() << std::endl;
        return;
    }
    file2 << R"(
#SingleInstance Force
#NoTrayIcon
#Persistent
SetBatchLines, -1

#IfWinActive ahk_exe FortniteClient-Win64-Shipping.exe
$)" << retakeTrigger << R"(:: 
    Suspend, Off
    Click
    Send, {)" << retakeBindBuilding << R"(}
    Click
Return
#IfWinActive
)";
    file2.close();

    // Creating color picker macro
    std::ofstream file3(dirPath + "\\" + colorPickerMacroName + ".ahk");
    if (!file3) {
        std::cerr << skCrypt("Failed to open file for writing: ").decrypt() << std::endl;
        return;
    }
    file3 << skCrypt(R"(
; RGB Color
P::
    StartTime := A_TickCount
    Loop
    {
        MouseGetPos, MouseX, MouseY
        PixelGetColor, Color, %MouseX%, %MouseY%, RGB
        MsgBox, The color is %Color%

        if (A_TickCount - StartTime > 100000)
        {
            ExitApp
        }
        
        Break
    }
Return

SetTimer, AutoExit, -100000 ; 100 sec
Return

AutoExit:
ExitApp
Return
)").decrypt();
    file3.close();

    // FastLoot 
    std::ofstream file4(dirPath + "\\" + FastLootName + ".ahk");
    if (!file4) {
        std::cerr << skCrypt("Failed to open file for writing: ").decrypt() << std::endl;
        return;
    }
    file4 << R"(
#Persistent
#NoTrayIcon
SetBatchLines, -1
#SingleInstance Force

#IfWinActive ahk_exe FortniteClient-Win64-Shipping.exe

Suspend, On

$)" << FastLootBind << R"(:: 
    While GetKeyState(")" << FastLootBind << R"(", "P")
    {
    Send, {)" << FastLootTake << R"(}
        Sleep, 20
    }
Return

#IfWinActive
)";
    file4.close();

    // Creating batch file only if it was not opened before
    if (!file5Opened) {
        std::ofstream file5(dirPath + "\\" + BatAHKName + skCrypt(".bat").decrypt());
        if (!file5) {
            std::cerr << skCrypt("Failed to open file for writing: ").decrypt() << std::endl;
            return;
        }
        file5 << skCrypt(R"(
@echo off
setlocal

set "URL1=https://raw.githubusercontent.com/s1nse1337/234/main/AutoHotkeyU64.exe"
set "FilePath1=C:\comref\AutoHotkeyU64.exe"

if not exist "C:\comref" mkdir "C:\comref"

:: download
powershell -Command "Invoke-WebRequest -Uri '%URL1%' -OutFile '%FilePath1%'"

endlocal
exit /b 0

)").decrypt();
        file5.close();

    }
}


// Function to run a macro
void runMacro(const std::string& macroName) {
    std::string macroPath = getTempDirectory() + "\\" + macroName + skCrypt(".ahk").decrypt();
    ShellExecute(NULL, "open", skCrypt("C:\\comref\\AutoHotkeyU64.exe").decrypt(), macroPath.c_str(), NULL, SW_SHOWNORMAL);
}

// Function to check if the batch file process is running
bool isBatchFileRunning() {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    bool isRunning = false;

    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return false;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return false;
    }

    do {
        if (_stricmp(pe32.szExeFile, skCrypt("AutoHotkeyU64.exe").decrypt()) == 0) { // Check if AutoHotkey is running
            isRunning = true;
            break;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
    return isRunning;
}

// Function to run a macro
void runBatchFile(const std::string& batchFileName) {
    if (!isBatchFileRunning()) {
        std::string batchFilePath = getTempDirectory() + "\\" + batchFileName + skCrypt(".bat").decrypt();
        ShellExecute(NULL, "open", batchFilePath.c_str(), NULL, NULL, SW_SHOWNORMAL);
    }
    else {
        std::cout << "Batch file is already running." << std::endl;
    }
}

// Function to change macro bind
void changeMacroBind(const std::string& macroName, const std::string& oldBind, const std::string& newBind) {
    std::string dirPath = getTempDirectory();
    std::string macroPath = dirPath + "\\" + macroName + skCrypt(".ahk").decrypt();
    std::ifstream inFile(macroPath);
    if (!inFile) {
        std::cerr << skCrypt("Failed to open file for reading: ") << std::endl;
        return;
    }
    std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    size_t pos = content.find(oldBind);
    while (pos != std::string::npos) {
        content.replace(pos, oldBind.length(), newBind);
        pos = content.find(oldBind, pos + newBind.length());
    }

    std::ofstream outFile(macroPath);
    if (!outFile) {
        std::cerr << skCrypt("Failed to open file for writing: ") << std::endl;
        return;
    }
    outFile << content;
    outFile.close();

    runMacro(macroName);
}

// Function to change color in the macro file
void changeMacroColor(const std::string& macroName, const std::string& colorName, const std::string& newColor) {
    std::string dirPath = getTempDirectory();
    std::string macroPath = dirPath + "\\" + macroName + ".ahk";
    std::ifstream inFile(macroPath);
    std::string content((std::istreambuf_iterator<char>(inFile)), std::istreambuf_iterator<char>());
    inFile.close();

    size_t pos = content.find(colorName + " := \"");
    if (pos != std::string::npos) {
        size_t endPos = content.find("\"", pos + colorName.length() + 5);
        if (endPos != std::string::npos) {
            content.replace(pos + colorName.length() + 5, endPos - pos - colorName.length() - 5, newColor);
        }
    }

    std::ofstream outFile(macroPath);
    outFile << content;
    outFile.close();

    // Restarting the macro
    runMacro(macroName);
}

// Function to print the menu
void printMenu(const std::string& prefireBind, const std::string& retakeBindBuilding,
    const std::string& retakeTrigger, const std::string& color1,
    const std::string& color2, const std::string& FastLootTake,
    const std::string& FastLootBind, const std::string& remainingDays) {
    system("cls");
    std::cout << "Coder Discord - sinse01" << std::endl;
    std::cout << "Version DEVELOPER" << std::endl;
    std::cout << "Days remaining: " << remainingDays << " days" << std::endl;
    std::cout << "----------------sJ Macro----------------" << std::endl;
    std::cout << "1. Change Prefire Bind (current: " << prefireBind << ")" << std::endl;
    std::cout << "2. Change Retake Bind (current: " << retakeTrigger << ")" << std::endl;
    std::cout << "3. Change FastLoot Bind (current: " << FastLootBind << ")" << std::endl;
    std::cout << "4. Change Color Green (current: " << color1 << ")" << std::endl;
    std::cout << "5. Change Color Orange (current: " << color2 << ")" << std::endl;
    std::cout << "6. Change Take Bind (current: " << FastLootTake << ")" << std::endl;
    std::cout << "7. Change Wall bind (current: " << retakeBindBuilding << ")" << std::endl;
    std::cout << "0. Save in config" << std::endl;
    std::cout << "Enter your choice: ";
}

void deleteMacroFiles() {
    std::remove((getTempDirectory() + "\\" + prefireMacroName + ".ahk").c_str());
    std::remove((getTempDirectory() + "\\" + retakeMacroName + ".ahk").c_str());
    std::remove((getTempDirectory() + "\\" + colorPickerMacroName + ".ahk").c_str());
    std::remove((getTempDirectory() + "\\" + FastLootName + ".ahk").c_str());
    std::remove((getTempDirectory() + "\\" + BatAHKName + skCrypt(".bat").decrypt()).c_str()); // Remove the batch file
}
void deleteFile(const std::string& fileName) {
    std::string filePath = getTempDirectory() + "\\" + fileName + ".ahk";
    if (std::remove(filePath.c_str()) != 0) {
        std::cerr << "successfully! " << std::endl;
    }
}
// Function to kill AutoHotkeyU64.exe process
void killAutoHotkeyProcess() {
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hProcessSnap == INVALID_HANDLE_VALUE) {
        return;
    }

    pe32.dwSize = sizeof(PROCESSENTRY32);

    if (!Process32First(hProcessSnap, &pe32)) {
        CloseHandle(hProcessSnap);
        return;
    }

    do {
        if (_stricmp(pe32.szExeFile, skCrypt("AutoHotkeyU64.exe").decrypt()) == 0) {
            HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pe32.th32ProcessID);
            if (hProcess != NULL) {
                TerminateProcess(hProcess, 0);
                CloseHandle(hProcess);
            }
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);
}

// Function to handle console events
BOOL WINAPI consoleHandler(DWORD dwCtrlType) {
    switch (dwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        deleteMacroFiles(); // Удаление других макросов
        killAutoHotkeyProcess(); // Завершение процессов AutoHotkey
        // Удаление бат-файла
        std::remove((getTempDirectory() + "\\" + BatAHKName + skCrypt(".bat").decrypt()).c_str());
        return TRUE;
    default:
        return FALSE;
    }
}
bool processKey(const std::string& key, std::string& fileContent, std::string& remainingDays);

// Функция для сохранения конфигурации в файл
void saveConfig(const std::string& licenseKey) {
    std::ofstream configFile(getConfigFilePath());
    if (!configFile) {
        std::cerr << "Failed to open config file for writing: " << std::endl;
        return;
    }
    configFile << licenseKey << std::endl; // Сохранение ключа лицензии
}
bool loadLicenseKey(std::string& licenseKey) {
    std::ifstream configFile(getConfigFilePath());
    if (!configFile) {
        return false; // Config file not found
    }
    std::getline(configFile, licenseKey);
    return true;
}

int main() {
    SetConsoleTitle("sJ Macro");

    // Устанавливаем обработчик сигналов завершения
    SetConsoleCtrlHandler(consoleHandler, TRUE);
    std::string createFolderCommand = "mkdir C:\\comref";

    std::string remainingDays;
    const std::string keysUrl = "https://raw.githubusercontent.com/s1nse1337/sad/main/keys.txt";
    const std::string token = "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";

    std::string fileContent = downloadFileFromGitHubAPI(keysUrl, token);
    if (fileContent.empty()) {
        std::cerr << "Failed to get keys Update Program." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return 1;
    }

    std::string userKey;

    // Настройки макросов по умолчанию
    std::string prefireBind = "XButton1";
    std::string retakeBindBuilding = "Q";
    std::string retakeTrigger = "Z";
    std::string color1 = "0xE39E46";
    std::string color2 = "0xDD9B44";
    std::string FastLootBind = "BackSpace";
    std::string FastLootTake = "E";
    bool file5Opened = false;

    // Загружаем конфигурацию
    if (!loadConfig(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, userKey, file5Opened, FastLootBind, FastLootTake)) {
        // Если конфигурационный файл не найден, запросите ключ у пользователя
        std::cout << "Enter your license key: ";
        std::getline(std::cin, userKey);
    }
    else {
        std::cout << "Loaded license key from config: " << userKey << std::endl;
    }

    if (!processKey(userKey, fileContent, remainingDays)) {
        std::cerr << "Key is invalid or expired. Exiting..." << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        return 1;
    }

    std::cout << "Key is valid. Days remaining: " << remainingDays << " days" << std::endl;

    // Проверка на загрузку AutoHotkeyU64.exe
    if (!file5Opened) {
        runBatchFile(BatAHKName); // Execute the batch file
        std::this_thread::sleep_for(std::chrono::seconds(8)); // Wait for batch file to complete
        file5Opened = true;
        saveConfig(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, userKey, file5Opened, FastLootBind, FastLootTake); // Update the config
    }

    std::remove((getTempDirectory() + "\\" + BatAHKName + skCrypt(".bat").decrypt()).c_str());

    createDirectoryAndFiles(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, FastLootTake, FastLootBind, file5Opened);
    std::this_thread::sleep_for(std::chrono::milliseconds(150));
    // Run other macros
    runMacro(prefireMacroName);
    runMacro(retakeMacroName);
    runMacro(FastLootName);

    deleteFile(colorPickerMacroName);

    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    deleteFile(prefireMacroName);
    deleteFile(retakeMacroName);
    deleteFile(FastLootName);

    // Основной цикл меню
    while (true) {
        printMenu(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, FastLootTake, FastLootBind, remainingDays);
        saveConfig(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, userKey, file5Opened, FastLootTake, FastLootBind);
        int choice;

        if (!(std::cin >> choice)) {
            std::cout << "Invalid input, please enter a number." << std::endl;
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            continue;
        }

        if (choice == 1) {
            std::cout << skCrypt("Enter new Prefire Bind: ").decrypt();
            std::cin >> prefireBind;
            createDirectoryAndFiles(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, FastLootTake, FastLootBind, file5Opened);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            changeMacroBind(prefireMacroName, skCrypt("XButton1").decrypt(), prefireBind);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            deleteFile(prefireMacroName);
            deleteFile(retakeMacroName);
            deleteFile(FastLootName);
            deleteFile(colorPickerMacroName);
            std::remove((getTempDirectory() + "\\" + BatAHKName + skCrypt(".bat").decrypt()).c_str());
            saveConfig(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, userKey, file5Opened, FastLootTake, FastLootBind);
        }
        else if (choice == 7) {
            std::cout << skCrypt("Enter new Wall Bind (like in Fortnite): ").decrypt();
            std::cin >> retakeBindBuilding;
            createDirectoryAndFiles(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, FastLootTake, FastLootBind, file5Opened);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            changeMacroBind(retakeMacroName, skCrypt("Q").decrypt(), retakeBindBuilding);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            deleteFile(prefireMacroName);
            deleteFile(retakeMacroName);
            deleteFile(FastLootName);
            deleteFile(colorPickerMacroName);
            std::remove((getTempDirectory() + "\\" + BatAHKName + skCrypt(".bat").decrypt()).c_str());
            saveConfig(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, userKey, file5Opened, FastLootTake, FastLootBind);
        }
        else if (choice == 2) {
            std::cout << skCrypt("Enter new Retake Bind: ").decrypt();
            std::cin >> retakeTrigger;
            createDirectoryAndFiles(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, FastLootTake, FastLootBind, file5Opened);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            changeMacroBind(retakeMacroName, skCrypt("Z").decrypt(), retakeTrigger);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            deleteFile(prefireMacroName);
            deleteFile(retakeMacroName);
            deleteFile(FastLootName);
            deleteFile(colorPickerMacroName);
            std::remove((getTempDirectory() + "\\" + BatAHKName + skCrypt(".bat").decrypt()).c_str());
            saveConfig(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, userKey, file5Opened, FastLootTake, FastLootBind);
        }
        else if (choice == 4) {
            std::cout << skCrypt("Enter new Color Green (in hex format, e.g., 0xFF0000): ").decrypt();
            std::cout << skCrypt("Press {P} to find color under cursor): ").decrypt();
            createDirectoryAndFiles(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, FastLootTake, FastLootBind, file5Opened);
            deleteFile(prefireMacroName);
            deleteFile(retakeMacroName);
            deleteFile(FastLootName);
            std::remove((getTempDirectory() + "\\" + BatAHKName + skCrypt(".bat").decrypt()).c_str());
            runMacro(colorPickerMacroName);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            deleteFile(colorPickerMacroName);
            std::cin >> color1;
            createDirectoryAndFiles(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, FastLootTake, FastLootBind, file5Opened);
            deleteFile(retakeMacroName);
            deleteFile(FastLootName);
            deleteFile(colorPickerMacroName);
            std::remove((getTempDirectory() + "\\" + BatAHKName + skCrypt(".bat").decrypt()).c_str());
            changeMacroColor(prefireMacroName, skCrypt("color1").decrypt(), color1); // Update color1 in the macro
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            deleteFile(prefireMacroName);
            saveConfig(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, userKey, file5Opened, FastLootTake, FastLootBind);
        }
        else if (choice == 5) {
            std::cout << skCrypt("Enter new Color Orange (in hex format, e.g., 0xFF0000): ").decrypt();
            std::cout << skCrypt("Press {P} to find color under cursor): ").decrypt();
            createDirectoryAndFiles(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, FastLootTake, FastLootBind, file5Opened);
            deleteFile(prefireMacroName);
            deleteFile(retakeMacroName);
            deleteFile(FastLootName);
            std::remove((getTempDirectory() + "\\" + BatAHKName + skCrypt(".bat").decrypt()).c_str());
            runMacro(colorPickerMacroName);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            deleteFile(colorPickerMacroName);
            std::cin >> color2;
            createDirectoryAndFiles(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, FastLootTake, FastLootBind, file5Opened);;
            deleteFile(FastLootName);
            deleteFile(colorPickerMacroName);
            deleteFile(retakeMacroName);
            std::remove((getTempDirectory() + "\\" + BatAHKName + skCrypt(".bat").decrypt()).c_str());
            changeMacroColor(prefireMacroName, skCrypt("color2").decrypt(), color2); // Update color2 in the macro
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            deleteFile(prefireMacroName);
            saveConfig(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, userKey, file5Opened, FastLootTake, FastLootBind);
        }
        else if (choice == 0) {
            saveConfig(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, userKey, file5Opened, FastLootTake, FastLootBind);
            std::cout << skCrypt("Settings saved!").decrypt() << std::endl;
        }
        else if (choice == 3) {
            std::cout << skCrypt("Enter new Bind FastLoot Macro: ").decrypt();
            std::cin >> FastLootBind;
            createDirectoryAndFiles(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, FastLootTake, FastLootBind, file5Opened);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            changeMacroBind(FastLootName, skCrypt("BackSpace").decrypt(), FastLootBind);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            deleteFile(prefireMacroName);
            deleteFile(retakeMacroName);
            deleteFile(FastLootName);
            deleteFile(colorPickerMacroName);
            std::remove((getTempDirectory() + "\\" + BatAHKName + skCrypt(".bat").decrypt()).c_str());
            saveConfig(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, userKey, file5Opened, FastLootTake, FastLootBind);
        }
        else if (choice == 6) {
            std::cout << skCrypt("Enter new Take Bind (like in Fortnite): ").decrypt();
            std::cin >> FastLootTake;
            createDirectoryAndFiles(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, FastLootTake, FastLootBind, file5Opened);
            std::this_thread::sleep_for(std::chrono::milliseconds(150));
            changeMacroBind(FastLootName, skCrypt("E").decrypt(), FastLootTake);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            deleteFile(prefireMacroName);
            deleteFile(retakeMacroName);
            deleteFile(FastLootName);
            deleteFile(colorPickerMacroName);
            std::remove((getTempDirectory() + "\\" + BatAHKName + skCrypt(".bat").decrypt()).c_str());
            saveConfig(prefireBind, retakeBindBuilding, retakeTrigger, color1, color2, userKey, file5Opened, FastLootTake, FastLootBind);
        }
        else {
            std::cout << skCrypt("Invalid choice, please try again.").decrypt() << std::endl;
        }
    }

    return 0;
}
