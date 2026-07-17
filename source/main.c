#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <orbis/libkernel.h>


void log_msg(const char* msg) {
    // Empty to prevent any potential stdout crashes
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
// Utility function for system notifications
void notify_popup(const char *p_Format, ...)
{
    OrbisNotificationRequest s_Request;
    memset(&s_Request, '\0', sizeof(s_Request));

    s_Request.reqId = 0; // NotificationRequest
    s_Request.unk3 = 0;
    s_Request.useIconImageUri = 0;
    s_Request.targetId = -1;

    va_list p_Args;
    va_start(p_Args, p_Format);
    vsnprintf(s_Request.message, sizeof(s_Request.message), p_Format, p_Args);
    va_end(p_Args);

    sceKernelSendNotificationRequest(0, &s_Request, sizeof(s_Request), 0);
}

int main(int argc, char **argv) {
    log_msg("RomInstaller App Starting...");
    
    notify_popup("Instalando ROMs de SNES no PS4...");

    log_msg("Creating directories...");
    
    // Create base directories if they don't exist
    mkdir("/data/psnes", 0777);
    mkdir("/data/psnes/roms", 0777);
    
    // Copy the roms from the PKG app0 folder to the PS4 HDD
    log_msg("Copying roms from /app0/roms to /data/psnes/roms...");
    int res = copy_directory("/app0/roms", "/data/psnes/roms");
    
    if (res == 0) {
        log_msg("Copy completed successfully.");
        notify_popup("Instalacao concluida com sucesso!");
    } else {
        log_msg("Copy failed.");
        notify_popup("Erro na instalacao das ROMs!");
    }
    
    // Give the notification 2 seconds to appear before auto-closing
    sceKernelUsleep(2000000);
    
    // Returning 0 terminates the application natively on PS4
    return 0;
}
