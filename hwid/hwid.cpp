#include <cstdio>
#include <Windows.h>
#include <iphlpapi.h>
#include <assert.h>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <array>
#include "hwid.h"



std::string hwid::getDiskHWID() {
    const char* cmd = "wmic path win32_physicalmedia get SerialNumber";
    std::array<char, 128> buffer;
    std::string result;
    std::shared_ptr<FILE> pipe(_popen(cmd, "r"), _pclose);
    if (!pipe) throw std::runtime_error("_popen() failed!");
    while (!feof(pipe.get())) {
        if (fgets(buffer.data(), 128, pipe.get()) != NULL) {
            result += buffer.data();
        }
    }
    return result;
}
//char* getMAC() {
//    PIP_ADAPTER_INFO AdapterInfo;
//    DWORD dwBufLen = sizeof(IP_ADAPTER_INFO);
//    char *mac_addr = (char*)malloc(18);
//
//    AdapterInfo = (IP_ADAPTER_INFO *) malloc(sizeof(IP_ADAPTER_INFO));
//    if (AdapterInfo == NULL) {
//        printf("Error allocating memory needed to call GetAdaptersinfo\n");
//        free(mac_addr);
//        return NULL; // it is safe to call free(NULL)
//    }
//
//    // Make an initial call to GetAdaptersInfo to get the necessary size into the dwBufLen variable
//    if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW) {
//        free(AdapterInfo);
//        AdapterInfo = (IP_ADAPTER_INFO *) malloc(dwBufLen);
//        if (AdapterInfo == NULL) {
//            printf("Error allocating memory needed to call GetAdaptersinfo\n");
//            free(mac_addr);
//            return NULL;
//        }
//    }
//
//    if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR) {
//        // Contains pointer to current adapter info
//        PIP_ADAPTER_INFO pAdapterInfo = AdapterInfo;
//        do {
//            // technically should look at pAdapterInfo->AddressLength
//            //   and not assume it is 6.
//            sprintf(mac_addr, "%02X:%02X:%02X:%02X:%02X:%02X",
//                    pAdapterInfo->Address[0], pAdapterInfo->Address[1],
//                    pAdapterInfo->Address[2], pAdapterInfo->Address[3],
//                    pAdapterInfo->Address[4], pAdapterInfo->Address[5]);
//            printf("Address: %s, mac: %s\n", pAdapterInfo->IpAddressList.IpAddress.String, mac_addr);
//            // print them all, return the last one.
//            // return mac_addr;
//
//            printf("\n");
//            pAdapterInfo = pAdapterInfo->Next;
//        } while(pAdapterInfo);
//    }
//    free(AdapterInfo);
//    return mac_addr; // caller must free.
//}

bool hwid::isUserHWIDValid() {
    std:: string userDiskHWID = getDiskHWID();
    std:: string validHWID[]= {
            "0025_3857_01AD_4E27", // Kwan
            "0025_38B5_11B6_11C1",  // Jasun
            "6212171000084" // CKL
    }; //substring to be checked

    int position = 0;
    int index_str;

    int numHWID = sizeof(validHWID)/sizeof(validHWID[0]);

    for (int i = 0; i < numHWID; ++i) {
        if (userDiskHWID.find(validHWID[i], position) != std::string::npos) {
            std::cout<< "USER HWID: " << "'" << validHWID[i] << "'" << " IS VALID!" << std::endl;
            return true;
        }
    }
    std::cout<< "USER HWID INVALID :: CLIENT BANNED!" << std::endl;
    return false;
}

//int main() {
//    std::cout<< isUserHWIDValid() << std::endl;
//}
