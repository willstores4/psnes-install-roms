#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <orbis/libkernel.h>


void log_msg(const char* msg) {
    // Basic stdout print, which can be seen if someone checks PS4 UART
    printf("%s\n", msg);
}

int copy_file(const char* src_path, const char* dst_path) {
    FILE* src = fopen(src_path, "rb");
    if (!src) return -1;
    FILE* dst = fopen(dst_path, "wb");
    if (!dst) {
        fclose(src);
        return -1;
    }
    
    char buffer[16384];
    size_t bytes;
    while ((bytes = fread(buffer, 1, sizeof(buffer), src)) > 0) {
        fwrite(buffer, 1, bytes, dst);
    }
    
    fclose(src);
    fclose(dst);
    return 0;
}

int copy_directory(const char* src_dir, const char* dst_dir) {
    DIR* dir = opendir(src_dir);
    if (!dir) return -1;
    
    // Ensure destination exists
    mkdir(dst_dir, 0777);
    
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
            continue;
            
        char src_path[1024];
        char dst_path[1024];
        snprintf(src_path, sizeof(src_path), "%s/%s", src_dir, entry->d_name);
        snprintf(dst_path, sizeof(dst_path), "%s/%s", dst_dir, entry->d_name);
        
        struct stat st;
        if (stat(src_path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                copy_directory(src_path, dst_path);
            } else {
                copy_file(src_path, dst_path);
            }
        }
    }
    closedir(dir);
    return 0;
}

int main(void) {
    log_msg("RomInstaller App Starting...");
    
    // Dynamically load necessary modules to avoid DT_NEEDED boot crashes
    int sys_util_handle = sceKernelLoadStartModule("libSceSysUtil.sprx", 0, NULL, 0, NULL, NULL);
    int sys_svc_handle = sceKernelLoadStartModule("libSceSystemService.sprx", 0, NULL, 0, NULL, NULL);
    int user_svc_handle = sceKernelLoadStartModule("libSceUserService.sprx", 0, NULL, 0, NULL, NULL);

    int (*sysUtilSendSystemNotificationWithText)(int, const char*) = NULL;
    int (*userServiceInitialize)(void*) = NULL;
    int (*systemServiceHideSplashScreen)(void) = NULL;

    void* ptr = NULL;
    if (sys_util_handle > 0) {
        sceKernelDlsym(sys_util_handle, "sceSysUtilSendSystemNotificationWithText", &ptr);
        sysUtilSendSystemNotificationWithText = (int (*)(int, const char*))ptr;
    }
    if (user_svc_handle > 0) {
        sceKernelDlsym(user_svc_handle, "sceUserServiceInitialize", &ptr);
        userServiceInitialize = (int (*)(void*))ptr;
    }
    if (sys_svc_handle > 0) {
        sceKernelDlsym(sys_svc_handle, "sceSystemServiceHideSplashScreen", &ptr);
        systemServiceHideSplashScreen = (int (*)(void))ptr;
    }

    struct OrbisUserServiceInitializeParams {
        uint32_t priority;
    };
    OrbisUserServiceInitializeParams user_params;
    user_params.priority = 120;
    
    if (userServiceInitialize) userServiceInitialize(&user_params);
    if (systemServiceHideSplashScreen) systemServiceHideSplashScreen();

    if (sysUtilSendSystemNotificationWithText) {
        sysUtilSendSystemNotificationWithText(222, "ROM Installer started! Copying files...");
    }
    
    // Create base directories if they don't exist
    mkdir("/data/psnes", 0777);
    mkdir("/data/psnes/roms", 0777);
    
    // Copy the roms from the PKG app0 folder to the PS4 HDD
    log_msg("Copying roms from /app0/roms to /data/psnes/roms...");
    int res = copy_directory("/app0/roms", "/data/psnes/roms");
    
    if (res == 0) {
        log_msg("Copy completed successfully.");
        if (sysUtilSendSystemNotificationWithText) {
            sysUtilSendSystemNotificationWithText(222, "Copy completed successfully! You can close the app.");
        }
    } else {
        log_msg("Copy failed.");
        if (sysUtilSendSystemNotificationWithText) {
            sysUtilSendSystemNotificationWithText(222, "Copy failed! Please check your paths.");
        }
    }
    
    // Enter an infinite loop so the app doesn't crash on exit.
    // The user must close it manually via the PS button.
    for(;;) {
        sleep(1);
    }
    
    // Returning 0 terminates the application natively on PS4
    return 0;
}
