#ifndef DLLLOADER_H
#define DLLLOADER_H

#include <QString>
#include <QStringList>

/**
 * DllLoader: Handles DLL discovery and PATH setup
 * Verifies that critical MinGW runtime DLLs are accessible
 * and configures the system DLL search path appropriately.
 */
class DllLoader {
public:
    /**
     * Initialize DLL loader - verifies DLLs are accessible
     * @return true if all critical DLLs are found, false otherwise
     */
    static bool initialize();
    
    /**
     * Get the cache directory where DLLs can be stored if needed
     * @return Full path to the DLL cache directory
     */
    static QString getCachePath();
    
private:
    /**
     * Verify that a DLL file is valid
     * @param filePath Path to the DLL file
     * @return true if file exists and is valid PE/DLL, false otherwise
     */
    static bool verifyDll(const QString& filePath);
};

#endif // DLLLOADER_H
