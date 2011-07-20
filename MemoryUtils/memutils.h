#ifndef _INCLUDE_MEMUTILS_H_
#define _INCLUDE_MEMUTILS_H_

/*
 * Written by Paul Clothier (aka Wazz)
 * :3
 */

#include "vtableinfo.h"
#include "classinfo.h"

#define	PAGE_SIZE	4096

#if defined PLATFORM_LINUX
#include <sys/mman.h>
#define ALIGN(ar) ((long)ar & ~(PAGE_SIZE-1))
#define	PAGE_EXECUTE_READWRITE	PROT_READ|PROT_WRITE|PROT_EXEC
#endif

/*
 * Major parts of this were taken from pRED*'s CDetour module and from AlliedModders
 * MemUtils modules from SourceMod
 */
class MemUtils
{
public :
	// Sets protection on the memory
	static void ProtectMemory(void *pAddr, int nLength, int nProt);
	// Allows the memory to be written to
	static void SetMemPatchable(void *pAddr, size_t nSize);

	// Attempts at counting the number of bytes until a jump instruction is found
	// This is useless now
	static size_t CountBytesUntilJump(BYTE *pFunc, int *pnOffset);

	// All from AMs MemUtils modules for finding signatures
	static void *FindPattern(const void *pLibPtr, const char *pchPattern, size_t nLen);
	static void *ResolveSymbol(void *pHandle, const char *pchSymbol);
	static bool GetLibraryInfo(const void *pLibPtr, DynLibInfo &lib);

	// Initializes a CUtlMap that contains a list of all vtables. Key = class name, Element = ClassInfo
	static bool InitializeClassMap(char *error, size_t maxlength);
	/* Calculates an offset after being given the address of a function
	 * @param pszClassName	Classname, should be equal to one of the keys in the Map eg. CTFPlayer
	 * @param pAddr			Address of the function to find the offset of
	 * @param offset		Variable to store the offset in
	 * @param pszVTableName	Name of the vtable if it is not the same as the class name
	 * @param vtable_offset	Variable to store the vtable offset in if the previous parameter was used
	 * @return				True if success
	 */
	static bool CalculateOffsetFromAddress(const char *pszClassName, const void *pAddr, int *offset, const char *pszVTableName = NULL, int *vtable_offset = NULL);
	/* Attempts to demangle a symbol name
	 * @param pszSymbol		Mangled symbol name. This will be replaced with the demangled name - NOT AN IDEAL SOLUTION FUCK KNOWS WHY I DID THIS
	 * @param unSize		Size of pszSymbol
	 * @noreturn
	 */
	static void DemangleSymbol(char *pszSymbol, size_t unSize);

	/* Identifies the classname of a pointer to a class
	 * @param pAddr			Pointer to find the class name of
	 * @param pszBuffer		Buffer to store the name in
	 * @param unSize		Size of buffer
	 * @noreturn
	 */
	static void GetPointerClassName(void *pAddr, char *pszBuffer, size_t unSize);
};

extern CUtlMap<const char *, ClassInfo *> g_ClassInfoMap;

#endif//_INCLUDE_MEMUTILS_H_

/*
	oh look
                                  ######                                  ######
                                ##########                              ##########
                               ############                            ############
                              ##############                          ##############
                              ##############                          ##############
                              ##############                          ##############
                               ############                            ############
                                ##########                              ##########
                                  ######                                  ######



                                      ###
                                     #####                                                                     #
                                    #######                                                                    ##
                                   #########                                                                  #####
                                  ########                                                                    ######
                                 #######                                                                     ########
                                #######                                                                       ########
                                ######                                                                          #######
                               ######                                                                            ######
                               #####                                                                              ######
                              #####                                                                                #####
                              #####                                                                                 ####
                              ####                                                                                   ####
                              ####                                                                                   ####
                              ####                                                                                   ####
                              ####                                                                                   ####
                              ####                                             ##                                    ####
                              #####                                           ####                                   ####
                              #####                                          ######                                 #####
                              ######                                        ########                               ######
                              #######                                      ###########                            ######
                              ########                                   ###############                        ########
                               #########                               ###################                    #########
                               ###########                          ##########################             ############
                                #############                    ##############   ####################################
                                ################             #################     ##################################
                                 ############################################       ################################
                                  ##########################################         ##############################
                                   ########################################            ###########################
                                    ######################################               #######################
                                      ##################################                    #################
                                        ##############################                         ###########
                                          ##########################
                                             ####################
                                                 ###########
*/
