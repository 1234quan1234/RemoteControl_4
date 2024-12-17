#include "Power.h"            

bool PowerManager::GetShutdownPrivilege() {
    HANDLE hToken;
    TOKEN_PRIVILEGES tkp;

    if (!OpenProcessToken(GetCurrentProcess(),
        TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken))
        return false;

    LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME,
        &tkp.Privileges[0].Luid);

    tkp.PrivilegeCount = 1;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;

    return AdjustTokenPrivileges(hToken, FALSE, &tkp, 0,
        (PTOKEN_PRIVILEGES)NULL, 0);
}

bool PowerManager::ExecutePowerAction(PowerAction action, bool force) {
    switch (action) {
    case PowerAction::SHUTDOWN:
        return Shutdown(force);
    case PowerAction::RESTART:
        return Restart(force);
    case PowerAction::HIBERNATE:
        return Hibernate();
    case PowerAction::SLEEP:
        return Sleep();
    case PowerAction::LOCK:
        return Lock();
    case PowerAction::LOGOFF:
        return Logoff(force);
    default:
        return false;
    }
}

bool PowerManager::Shutdown(bool force) {
    if (!GetShutdownPrivilege()) return false;
    UINT flags = EWX_SHUTDOWN;
    if (force) flags |= EWX_FORCE;
    return ExitWindowsEx(flags, SHTDN_REASON_MAJOR_APPLICATION);
}

bool PowerManager::Restart(bool force) {
    if (!GetShutdownPrivilege()) return false;
    UINT flags = EWX_REBOOT;
    if (force) flags |= EWX_FORCE;
    return ExitWindowsEx(flags, SHTDN_REASON_MAJOR_APPLICATION);
}

bool PowerManager::Hibernate() {
    return SetSuspendState(TRUE, TRUE, FALSE);
}

bool PowerManager::Sleep() {
    return SetSuspendState(FALSE, TRUE, FALSE);
}

bool PowerManager::Lock() {
    return LockWorkStation();
}

bool PowerManager::Logoff(bool force) {
    UINT flags = EWX_LOGOFF;
    if (force) flags |= EWX_FORCE;
    return ExitWindowsEx(flags, SHTDN_REASON_MAJOR_APPLICATION);
}