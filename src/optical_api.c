/*
 * HalcyonScript © KAInaps 2026 
   Simple programming for creative minds 
   github.com/Nicetink/HalcyonScript
   
   Optical Drive API - Functions for working with DVD/CD drives
 */

#include "runtime.h"
#include <windows.h>
#include <winioctl.h>
#include <stdio.h>
#include <mmsystem.h>

// Define missing constants if not available
#ifndef IOCTL_STORAGE_LOAD_MEDIA2
#define IOCTL_STORAGE_LOAD_MEDIA2 CTL_CODE(IOCTL_STORAGE_BASE, 0x0204, METHOD_BUFFERED, FILE_READ_ACCESS)
#endif

/* DVD.getDrives() - Get list of optical drives */
HcsValue* builtin_dvd_get_drives(int argc, HcsValue** args) {
    HcsValue* drives = value_array();
    
    DWORD drives_mask = GetLogicalDrives();
    char drive_letter[4] = "A:\\";
    
    for (int i = 0; i < 26; i++) {
        if (drives_mask & (1 << i)) {
            drive_letter[0] = 'A' + i;
            UINT drive_type = GetDriveTypeA(drive_letter);
            
            if (drive_type == DRIVE_CDROM) {
                HcsValue* drive_info = value_object();
                value_object_set(drive_info, "letter", value_string(drive_letter));
                
                // Get volume label
                char volume_name[256] = "";
                BOOL vol_result = GetVolumeInformationA(drive_letter, volume_name, sizeof(volume_name), 
                                    NULL, NULL, NULL, NULL, 0);
                
                if (vol_result) {
                    value_object_set(drive_info, "label", value_string(volume_name));
                } else {
                    value_object_set(drive_info, "label", value_string(""));
                }
                
                value_array_push(drives, drive_info);
            }
        }
    }
    
    return drives;
}

/* DVD.isDiscPresent(drive) - Check if disc is present in drive */
HcsValue* builtin_dvd_is_disc_present(int argc, HcsValue** args) {
    if (argc < 1 || args[0]->type != HCS_VAL_STRING) {
        return value_bool(false);
    }
    
    const char* drive = args[0]->data.string;
    char drive_path[MAX_PATH];
    
    if (strlen(drive) == 1) {
        snprintf(drive_path, sizeof(drive_path), "%s:\\", drive);
    } else {
        strncpy(drive_path, drive, sizeof(drive_path) - 1);
        drive_path[sizeof(drive_path) - 1] = '\0';
    }
    
    // Method 1: Try GetVolumeInformation - this works for most discs
    char volume_name[256];
    DWORD serial_number;
    DWORD max_component_length;
    DWORD file_system_flags;
    char file_system[256];
    
    BOOL result = GetVolumeInformationA(drive_path, volume_name, sizeof(volume_name),
                                       &serial_number, &max_component_length, 
                                       &file_system_flags, file_system, sizeof(file_system));
    
    if (result) {
        return value_bool(true);
    }
    
    // Method 2: Try GetDiskFreeSpaceEx - alternative method
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    result = GetDiskFreeSpaceExA(drive_path, &freeBytesAvailable, &totalBytes, &totalFreeBytes);
    
    if (result) {
        return value_bool(true);
    }
    
    // Method 3: Check drive type and try to access
    UINT driveType = GetDriveTypeA(drive_path);
    if (driveType == DRIVE_CDROM) {
        // For CD-ROM drives, try to open the device directly
        char device_path[MAX_PATH];
        snprintf(device_path, sizeof(device_path), "\\\\.\\%c:", drive[0]);
        
        HANDLE hDevice = CreateFileA(device_path, 0, 
                                    FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                    NULL, OPEN_EXISTING, 0, NULL);
        
        if (hDevice != INVALID_HANDLE_VALUE) {
            DWORD bytesReturned;
            BOOL deviceResult = DeviceIoControl(hDevice, IOCTL_STORAGE_CHECK_VERIFY, 
                                              NULL, 0, NULL, 0, &bytesReturned, NULL);
            CloseHandle(hDevice);
            
            if (deviceResult) {
                return value_bool(true);
            }
        }
    }
    
    return value_bool(false);
}

/* DVD.getDiscInfo(drive) - Get information about disc in drive */
HcsValue* builtin_dvd_get_disc_info(int argc, HcsValue** args) {
    if (argc < 1 || args[0]->type != HCS_VAL_STRING) {
        return value_null();
    }
    
    const char* drive = args[0]->data.string;
    char drive_path[10];
    
    if (strlen(drive) == 1) {
        sprintf(drive_path, "%s:\\", drive);
    } else {
        strcpy(drive_path, drive);
    }
    
    // Check if disc is present first
    if (!builtin_dvd_is_disc_present(argc, args)->data.boolean) {
        return value_null();
    }
    
    HcsValue* disc_info = value_object();
    
    // Get volume information
    char volume_name[256] = "";
    char file_system[256] = "";
    DWORD serial_number = 0;
    DWORD max_component_length = 0;
    DWORD file_system_flags = 0;
    
    if (GetVolumeInformationA(drive_path, volume_name, sizeof(volume_name),
                             &serial_number, &max_component_length, 
                             &file_system_flags, file_system, sizeof(file_system))) {
        
        value_object_set(disc_info, "label", value_string(volume_name));
        value_object_set(disc_info, "fileSystem", value_string(file_system));
        value_object_set(disc_info, "serialNumber", value_number((double)serial_number));
    }
    
    // Get disc space information
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    if (GetDiskFreeSpaceExA(drive_path, &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
        double totalMB = (double)totalBytes.QuadPart / (1024.0 * 1024.0);
        double freeMB = (double)totalFreeBytes.QuadPart / (1024.0 * 1024.0);
        double usedMB = totalMB - freeMB;
        
        value_object_set(disc_info, "totalSize", value_number(totalMB));
        value_object_set(disc_info, "freeSpace", value_number(freeMB));
        value_object_set(disc_info, "usedSpace", value_number(usedMB));
    }
    
    // Determine disc type based on capacity
    if (totalBytes.QuadPart > 0) {
        double totalGB = (double)totalBytes.QuadPart / (1024.0 * 1024.0 * 1024.0);
        const char* disc_type = "Unknown";
        
        if (totalGB <= 0.8) {
            disc_type = "CD";
        } else if (totalGB <= 5.0) {
            disc_type = "DVD";
        } else if (totalGB <= 30.0) {
            disc_type = "Blu-ray";
        } else {
            disc_type = "Other";
        }
        
        value_object_set(disc_info, "type", value_string(disc_type));
    }
    
    return disc_info;
}

/* DVD.eject(drive) - Eject disc from drive */
HcsValue* builtin_dvd_eject(int argc, HcsValue** args) {
    if (argc < 1 || args[0]->type != HCS_VAL_STRING) {
        return value_bool(false);
    }
    
    const char* drive = args[0]->data.string;
    char drive_path[10];
    
    if (strlen(drive) == 1) {
        sprintf(drive_path, "\\\\.\\%s:", drive);
    } else {
        sprintf(drive_path, "\\\\.\\%c:", drive[0]);
    }
    
    HANDLE hDevice = CreateFileA(drive_path, GENERIC_READ, 
                                FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                NULL, OPEN_EXISTING, 0, NULL);
    
    if (hDevice == INVALID_HANDLE_VALUE) {
        return value_bool(false);
    }
    
    DWORD bytesReturned;
    BOOL result = DeviceIoControl(hDevice, IOCTL_STORAGE_EJECT_MEDIA, 
                                 NULL, 0, NULL, 0, &bytesReturned, NULL);
    
    CloseHandle(hDevice);
    return value_bool(result);
}

/* DVD.closeTray(drive) - Close disc tray */
HcsValue* builtin_dvd_close_tray(int argc, HcsValue** args) {
    if (argc < 1 || args[0]->type != HCS_VAL_STRING) {
        return value_bool(false);
    }
    
    const char* drive = args[0]->data.string;
    char drive_path[10];
    
    if (strlen(drive) == 1) {
        sprintf(drive_path, "\\\\.\\%s:", drive);
    } else {
        sprintf(drive_path, "\\\\.\\%c:", drive[0]);
    }
    
    HANDLE hDevice = CreateFileA(drive_path, GENERIC_READ, 
                                FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                NULL, OPEN_EXISTING, 0, NULL);
    
    if (hDevice == INVALID_HANDLE_VALUE) {
        return value_bool(false);
    }
    
    DWORD bytesReturned;
    BOOL result = FALSE;
    
    // Method 1: Try IOCTL_STORAGE_LOAD_MEDIA
    result = DeviceIoControl(hDevice, IOCTL_STORAGE_LOAD_MEDIA, 
                            NULL, 0, NULL, 0, &bytesReturned, NULL);
    
    if (!result) {
        // Method 2: Try IOCTL_STORAGE_LOAD_MEDIA2 (alternative code)
        result = DeviceIoControl(hDevice, IOCTL_STORAGE_LOAD_MEDIA2, 
                                NULL, 0, NULL, 0, &bytesReturned, NULL);
    }
    
    if (!result) {
        // Method 3: Try using MCI (Media Control Interface) commands
        CloseHandle(hDevice);
        
        char mci_command[256];
        snprintf(mci_command, sizeof(mci_command), "set cdaudio door closed wait");
        
        MCIERROR mci_result = mciSendStringA(mci_command, NULL, 0, NULL);
        if (mci_result == 0) {
            return value_bool(true);
        }
        
        // Method 4: Try specific drive letter with MCI
        snprintf(mci_command, sizeof(mci_command), "set cdaudio%c door closed wait", drive[0]);
        mci_result = mciSendStringA(mci_command, NULL, 0, NULL);
        if (mci_result == 0) {
            return value_bool(true);
        }
        
        return value_bool(false);
    }
    
    CloseHandle(hDevice);
    return value_bool(result);
}

/* DVD.readFiles(drive, path) - List files on disc */
HcsValue* builtin_dvd_read_files(int argc, HcsValue** args) {
    if (argc < 1 || args[0]->type != HCS_VAL_STRING) {
        return value_null();
    }
    
    const char* drive = args[0]->data.string;
    const char* path = (argc > 1 && args[1]->type == HCS_VAL_STRING) ? args[1]->data.string : "";
    
    // Validate path to prevent directory traversal
    if (strstr(path, "..") || strstr(path, "\\..\\") || strstr(path, "../")) {
        return value_null(); // Reject path traversal attempts
    }
    
    char search_path[MAX_PATH];
    if (strlen(drive) == 1) {
        snprintf(search_path, sizeof(search_path), "%s:\\%s\\*", drive, path);
    } else {
        snprintf(search_path, sizeof(search_path), "%s\\%s\\*", drive, path);
    }
    
    HcsValue* files = value_array();
    WIN32_FIND_DATAA findData;
    HANDLE hFind = FindFirstFileA(search_path, &findData);
    
    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (strcmp(findData.cFileName, ".") != 0 && strcmp(findData.cFileName, "..") != 0) {
                HcsValue* file_info = value_object();
                value_object_set(file_info, "name", value_string(findData.cFileName));
                
                BOOL is_directory = (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
                value_object_set(file_info, "isDirectory", value_bool(is_directory));
                
                if (!is_directory) {
                    LARGE_INTEGER file_size;
                    file_size.LowPart = findData.nFileSizeLow;
                    file_size.HighPart = findData.nFileSizeHigh;
                    double size_mb = (double)file_size.QuadPart / (1024.0 * 1024.0);
                    value_object_set(file_info, "size", value_number(size_mb));
                }
                
                // Convert FILETIME to readable format
                SYSTEMTIME st;
                if (FileTimeToSystemTime(&findData.ftLastWriteTime, &st)) {
                    char date_str[64];
                    sprintf(date_str, "%04d-%02d-%02d %02d:%02d:%02d", 
                           st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
                    value_object_set(file_info, "modified", value_string(date_str));
                }
                
                value_array_push(files, file_info);
            }
        } while (FindNextFileA(hFind, &findData));
        
        FindClose(hFind);
    }
    
    return files;
}

/* DVD.copyFile(sourceDrive, sourceFile, destPath) - Copy file from disc */
HcsValue* builtin_dvd_copy_file(int argc, HcsValue** args) {
    if (argc < 3 || args[0]->type != HCS_VAL_STRING || 
        args[1]->type != HCS_VAL_STRING || args[2]->type != HCS_VAL_STRING) {
        return value_bool(false);
    }
    
    const char* drive = args[0]->data.string;
    const char* source_file = args[1]->data.string;
    const char* dest_path = args[2]->data.string;
    
    char source_path[MAX_PATH];
    if (strlen(drive) == 1) {
        sprintf(source_path, "%s:\\%s", drive, source_file);
    } else {
        sprintf(source_path, "%s\\%s", drive, source_file);
    }
    
    BOOL result = CopyFileA(source_path, dest_path, FALSE);
    return value_bool(result);
}

/* DVD.getCapabilities(drive) - Get drive capabilities */
HcsValue* builtin_dvd_get_capabilities(int argc, HcsValue** args) {
    if (argc < 1 || args[0]->type != HCS_VAL_STRING) {
        return value_null();
    }
    
    const char* drive = args[0]->data.string;
    char drive_path[10];
    
    if (strlen(drive) == 1) {
        sprintf(drive_path, "\\\\.\\%s:", drive);
    } else {
        sprintf(drive_path, "\\\\.\\%c:", drive[0]);
    }
    
    HANDLE hDevice = CreateFileA(drive_path, GENERIC_READ, 
                                FILE_SHARE_READ | FILE_SHARE_WRITE, 
                                NULL, OPEN_EXISTING, 0, NULL);
    
    if (hDevice == INVALID_HANDLE_VALUE) {
        return value_null();
    }
    
    HcsValue* capabilities = value_object();
    
    // Get device capabilities
    STORAGE_PROPERTY_QUERY query;
    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;
    
    STORAGE_DEVICE_DESCRIPTOR descriptor;
    DWORD bytesReturned;
    
    if (DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY, 
                       &query, sizeof(query), &descriptor, sizeof(descriptor), 
                       &bytesReturned, NULL)) {
        
        value_object_set(capabilities, "canRead", value_bool(true));
        value_object_set(capabilities, "canWrite", value_bool(descriptor.DeviceType == FILE_DEVICE_CD_ROM));
        value_object_set(capabilities, "canEject", value_bool(true));
    }
    
    CloseHandle(hDevice);
    return capabilities;
}

/* DVD.writeFile(sourcePath, destDrive, destPath) - Write file to disc */
HcsValue* builtin_dvd_write_file(int argc, HcsValue** args) {
    if (argc < 3 || args[0]->type != HCS_VAL_STRING || 
        args[1]->type != HCS_VAL_STRING || args[2]->type != HCS_VAL_STRING) {
        return value_bool(false);
    }
    
    const char* source_path = args[0]->data.string;
    const char* drive = args[1]->data.string;
    const char* dest_path = args[2]->data.string;
    
    char full_dest_path[MAX_PATH];
    if (strlen(drive) == 1) {
        sprintf(full_dest_path, "%s:\\%s", drive, dest_path);
    } else {
        sprintf(full_dest_path, "%s\\%s", drive, dest_path);
    }
    
    BOOL result = CopyFileA(source_path, full_dest_path, FALSE);
    return value_bool(result);
}

/* DVD.createDirectory(drive, dirPath) - Create directory on disc */
HcsValue* builtin_dvd_create_directory(int argc, HcsValue** args) {
    if (argc < 2 || args[0]->type != HCS_VAL_STRING || args[1]->type != HCS_VAL_STRING) {
        return value_bool(false);
    }
    
    const char* drive = args[0]->data.string;
    const char* dir_path = args[1]->data.string;
    
    char full_path[MAX_PATH];
    if (strlen(drive) == 1) {
        sprintf(full_path, "%s:\\%s", drive, dir_path);
    } else {
        sprintf(full_path, "%s\\%s", drive, dir_path);
    }
    
    BOOL result = CreateDirectoryA(full_path, NULL);
    return value_bool(result);
}

/* DVD.verifyFile(originalPath, discDrive, discPath) - Verify file integrity */
HcsValue* builtin_dvd_verify_file(int argc, HcsValue** args) {
    if (argc < 3 || args[0]->type != HCS_VAL_STRING || 
        args[1]->type != HCS_VAL_STRING || args[2]->type != HCS_VAL_STRING) {
        return value_bool(false);
    }
    
    const char* original_path = args[0]->data.string;
    const char* drive = args[1]->data.string;
    const char* disc_path = args[2]->data.string;
    
    char full_disc_path[MAX_PATH];
    if (strlen(drive) == 1) {
        sprintf(full_disc_path, "%s:\\%s", drive, disc_path);
    } else {
        sprintf(full_disc_path, "%s\\%s", drive, disc_path);
    }
    
    // Check if both files exist
    WIN32_FIND_DATAA findData;
    HANDLE hFind1 = FindFirstFileA(original_path, &findData);
    HANDLE hFind2 = FindFirstFileA(full_disc_path, &findData);
    
    if (hFind1 == INVALID_HANDLE_VALUE || hFind2 == INVALID_HANDLE_VALUE) {
        if (hFind1 != INVALID_HANDLE_VALUE) FindClose(hFind1);
        if (hFind2 != INVALID_HANDLE_VALUE) FindClose(hFind2);
        return value_bool(false);
    }
    
    FindClose(hFind1);
    FindClose(hFind2);
    
    // Compare file sizes
    HANDLE hOriginal = CreateFileA(original_path, GENERIC_READ, FILE_SHARE_READ, 
                                  NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    HANDLE hDisc = CreateFileA(full_disc_path, GENERIC_READ, FILE_SHARE_READ, 
                              NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    
    if (hOriginal == INVALID_HANDLE_VALUE || hDisc == INVALID_HANDLE_VALUE) {
        if (hOriginal != INVALID_HANDLE_VALUE) CloseHandle(hOriginal);
        if (hDisc != INVALID_HANDLE_VALUE) CloseHandle(hDisc);
        return value_bool(false);
    }
    
    LARGE_INTEGER originalSize, discSize;
    if (!GetFileSizeEx(hOriginal, &originalSize) || !GetFileSizeEx(hDisc, &discSize)) {
        CloseHandle(hOriginal);
        CloseHandle(hDisc);
        return value_bool(false);
    }
    
    if (originalSize.QuadPart != discSize.QuadPart) {
        CloseHandle(hOriginal);
        CloseHandle(hDisc);
        return value_bool(false);
    }
    
    // Compare file contents in chunks
    const DWORD BUFFER_SIZE = 64 * 1024; // 64KB buffer
    BYTE* buffer1 = (BYTE*)malloc(BUFFER_SIZE);
    BYTE* buffer2 = (BYTE*)malloc(BUFFER_SIZE);
    BOOL files_match = TRUE;
    
    if (buffer1 && buffer2) {
        DWORD bytesRead1, bytesRead2;
        
        while (files_match) {
            BOOL read1 = ReadFile(hOriginal, buffer1, BUFFER_SIZE, &bytesRead1, NULL);
            BOOL read2 = ReadFile(hDisc, buffer2, BUFFER_SIZE, &bytesRead2, NULL);
            
            if (!read1 || !read2 || bytesRead1 != bytesRead2) {
                files_match = FALSE;
                break;
            }
            
            if (bytesRead1 == 0) break; // End of file
            
            if (memcmp(buffer1, buffer2, bytesRead1) != 0) {
                files_match = FALSE;
                break;
            }
        }
    } else {
        files_match = FALSE;
    }
    
    if (buffer1) free(buffer1);
    if (buffer2) free(buffer2);
    CloseHandle(hOriginal);
    CloseHandle(hDisc);
    
    return value_bool(files_match);
}

/* DVD.getWriteCapacity(drive) - Get available write capacity */
HcsValue* builtin_dvd_get_write_capacity(int argc, HcsValue** args) {
    if (argc < 1 || args[0]->type != HCS_VAL_STRING) {
        return value_null();
    }
    
    const char* drive = args[0]->data.string;
    char drive_path[10];
    
    if (strlen(drive) == 1) {
        sprintf(drive_path, "%s:\\", drive);
    } else {
        strcpy(drive_path, drive);
    }
    
    ULARGE_INTEGER freeBytesAvailable, totalBytes, totalFreeBytes;
    
    if (GetDiskFreeSpaceExA(drive_path, &freeBytesAvailable, &totalBytes, &totalFreeBytes)) {
        HcsValue* result = value_object();
        
        double totalMB = (double)totalBytes.QuadPart / (1024.0 * 1024.0);
        double freeMB = (double)totalFreeBytes.QuadPart / (1024.0 * 1024.0);
        double usedMB = totalMB - freeMB;
        
        value_object_set(result, "total", value_number(totalMB));
        value_object_set(result, "free", value_number(freeMB));
        value_object_set(result, "used", value_number(usedMB));
        
        return result;
    }
    
    return value_null();
}