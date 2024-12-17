#pragma once
#include "..\Libs\Header.h"

class PowerManager {
public:
    // Define all possible power actions
    enum class PowerAction {
        SHUTDOWN,       // Shutdown system
        RESTART,       // Restart system
        HIBERNATE,     // Hibernate system
        SLEEP,         // Sleep system
        LOCK,          // Lock workstation
        LOGOFF        // Log off current user
    };

    // Main power control function
    static bool ExecutePowerAction(PowerAction action, bool force = false);

    // Individual power control functions
    static bool Shutdown(bool force = false);     // Shutdown PC
    static bool Restart(bool force = false);      // Restart PC
    static bool Hibernate();                      // Hibernate PC
    static bool Sleep();                         // Sleep PC
    static bool Lock();                          // Lock PC
    static bool Logoff(bool force = false);      // Log off user

private:
    // Helper function to get privileges
    static bool GetShutdownPrivilege();
};