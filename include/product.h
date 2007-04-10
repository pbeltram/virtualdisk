/*
Copyright 2007 Primoz Beltram

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

//******************************************************************************

#define __TO_STR(x)    #x
#define TO_STR(x)    __TO_STR(x)

//
//  Product name & description
//
#define tst_PRODUCT_NAME_d          "Virtual Bus"
#define tst_PRODUCT_NAME_SHORT_d    ""
#define tst_PRODUCT_DESC_d          ""

//
//  Company name & copyright info
//
#define tst_COMPANY_NAME_d          ""
#define tst_COPYRIGHT_d             ""

//
//  Product version: major & minor version (to be updated for each release)
//
#define tst_VERSION_MAJOR_d         0
#define tst_VERSION_MINOR_d         0

//
//  Product version: build number
//
#define tst_VERSION_BUILDNUM_d      0
#define tst_VERSION_LABEL_d         "UNKNOWN BUILD"

//
//  Product build info: time, compiler & computer (from environment)
//
#define tst_BUILD_TIME_d            __TIMESTAMP__

#ifdef _MSC_VER
    #define tst_COMPILER_INFO_d     "MS VC++ " TO_STR(_MSC_VER) " with STL " TO_STR(_CPPLIB_VER)
#else                               
    #define tst_COMPILER_INFO_d     "UNKNOWN COMPILER"
#endif

#define tst_COMPUTER_INFO_d         "MY COMPUTER"

#define tst_VER_FILEFLAGSMASK_d     VS_FFI_FILEFLAGSMASK
#define tst_VER_FILEOS_d            VOS_NT_WINDOWS32

#if _DEBUG
    #define tst_VER_FILEFLAGS_d     (VS_FF_PRERELEASE|VS_FF_DEBUG|VS_FF_PRIVATEBUILD)
#else 
    #define tst_VER_FILEFLAGS_d     0
#endif

#if defined(_DRIVER)
    #define tst_VER_FILETYPE_d      VFT_DRV
    #define tst_VER_FILESUBTYPE_d   VFT2_DRV_SYSTEM
#elif defined(_DLL)
    #define tst_VER_FILETYPE_d      VFT_DLL
    #define tst_VER_FILESUBTYPE_d   VFT2_UNKNOWN
#else
    #define tst_VER_FILETYPE_d      VFT_APP
    #define tst_VER_FILESUBTYPE_d   VFT2_UNKNOWN
#endif

#define tst_VER_LANG_STR_d          "040904B0" /* LANG_ENGLISH/SUBLANG_ENGLISH_US, Unicode CP */
#define tst_VER_TRANSLATION_d       0x0409, 0x04B0

#define VS2(x,y,z) #x "." #y "." #z
#define VS1(x,y,z) VS2(x, y, z)

#define tst_VER_PRODUCTVERSION_STR_d    VS1(tst_VERSION_MAJOR_d, \
                                            tst_VERSION_MINOR_d, \
                                            tst_VERSION_BUILDNUM_d)
#define tst_VER_PRODUCTVERSION_d        tst_VERSION_MAJOR_d,\
                                        tst_VERSION_MINOR_d,\
                                        tst_VERSION_BUILDNUM_d,\
                                        0
// End Of File
