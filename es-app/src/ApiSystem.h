#ifndef API_SYSTEM
#define API_SYSTEM

#include <string>
#include "Window.h"
#include "components/BusyComponent.h"

struct BiosFile {
  std::string status;
  std::string md5;
  std::string path;
};

struct BiosSystem {
  std::string name;
  std::vector<BiosFile> bios;
};

struct RetroAchievementGame
{
	std::string name;
	std::string achievements;
	std::string points;
	std::string lastplayed;
	std::string badge;
};

struct RetroAchievementInfo
{	
	std::string username;
	std::string totalpoints;
	std::string rank;
	std::string userpic;
	std::string registered;

	std::string error;

	std::vector<RetroAchievementGame> games;
};

class ApiSystem 
{
public:

    static ApiSystem *getInstance();

    const static Uint32 SDL_FAST_QUIT = 0x800F;
    const static Uint32 SDL_SYS_SHUTDOWN = 0X4000;
    const static Uint32 SDL_SYS_REBOOT = 0x2000;

    unsigned long getFreeSpaceGB(std::string mountpoint);

    std::string getFreeSpaceInfo();

    bool isFreeSpaceLimit();

    std::string getVersion();
    std::string getRootPassword();

    bool setOverscan(bool enable);

    bool setOverclock(std::string mode);

    bool createLastVersionFileIfNotExisting();

    bool updateLastVersionFile();

    bool needToShowVersionMessage();

    std::pair<std::string, int> updateSystem(const std::function<void(const std::string)>& func = nullptr);
    std::pair<std::string, int> backupSystem(BusyComponent* ui, std::string device);
    std::pair<std::string, int> installSystem(BusyComponent* ui, std::string device, std::string architecture);
    std::pair<std::string, int> scrape(BusyComponent* ui);

    bool ping();

    bool canUpdate(std::vector<std::string>& output);

    bool launchKodi(Window *window);
    bool launchFileManager(Window *window);

    bool enableWifi(std::string ssid, std::string key);

    bool disableWifi();

    bool reboot();

    bool shutdown();

    bool fastReboot();

    bool fastShutdown();

    std::string getIpAdress();

    bool scanNewBluetooth(const std::function<void(const std::string)>& func = nullptr);

    std::vector<std::string> getAvailableStorageDevices();
    std::vector<std::string> getAvailableBackupDevices();
    std::vector<std::string> getAvailableInstallDevices();
    std::vector<std::string> getAvailableInstallArchitectures();
    std::vector<std::string> getAvailableOverclocking();
    std::vector<std::string> getSystemInformations();
    std::vector<BiosSystem> getBiosInformations();
    std::vector<std::string> getVideoModes();
    bool generateSupportFile();

    std::string getCurrentStorage();

    bool setStorage(std::string basic_string);

    bool forgetBluetoothControllers();

    /* audio card */
    bool setAudioOutputDevice(std::string device);
    std::vector<std::string> getAvailableAudioOutputDevices();
    std::string getCurrentAudioOutputDevice();

    /* video output */
    std::vector<std::string> getAvailableVideoOutputDevices();

    // Batocera
	RetroAchievementInfo getRetroAchievements();
    std::vector<std::string> getBatoceraThemesList();
    std::pair<std::string,int> installBatoceraTheme(std::string thname, const std::function<void(const std::string)>& func = nullptr);
    std::vector<std::string> getBatoceraBezelsList();
    std::pair<std::string,int> installBatoceraBezel(std::string bezelsystem, const std::function<void(const std::string)>& func = nullptr);
    std::pair<std::string,int> uninstallBatoceraBezel(BusyComponent* ui, std::string bezelsystem);

	std::string getCRC32(const std::string fileName, bool fromZipContents = true);

	bool	getBrighness(int& value);
	void	setBrighness(int value);

private:
    static ApiSystem *instance;

    ApiSystem();

    bool halt(bool reboot, bool fast);

    
    void launchExternalWindow_before(Window *window);
    void launchExternalWindow_after(Window *window);
};

#endif

