; DPS8M NSIS Setup: .nsis/setup.nsi
; vim: set ft=nsis ts=2 sw=2 tw=0 expandtab :
; SPDX-License-Identifier: MIT-0
; scspell-id: 43d06fef-488e-11ed-9b62-80ee73e9b8e7
; Copyright (c) 2022-2024 The DPS8M Development Team

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; NSIS: Includes

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; NSIS: Variables

!define APPNAME           "DPS8M"
!define COMPANYNAME       "The DPS8M Development Team"
!define DESCRIPTION       "DPS‑8/M mainframe simulator"
!define DEVELOPER         "The DPS8M Development Team"
!define LICENSE_TEXT_FILE "share/LICENSE.txt"
!define ICON              "icon/icon.ico"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; NSIS: Configuration

Name                  "${APPNAME}"
Icon                  "${ICON}"
OutFile               "dps8m-setup.exe"
XPStyle               on
SetCompressor         /solid /final lzma
RequestExecutionLevel admin
CRCCheck              force
SetDatablockOptimize  on

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Function: Installer: Init

Function .onInit
  ${If} ${RunningX64}
    StrCpy $INSTDIR "$PROGRAMFILES64\${APPNAME}"
  ${Else}
    StrCpy $INSTDIR "$PROGRAMFILES\${APPNAME}"
  ${EndIf}
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Function: Installation failed

Function .onInstFailed
  MessageBox MB_OK|MB_ICONEXCLAMATION "Sorry, ${APPNAME} installation failed!"
FunctionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Macro: Verify user is an Administrator

!macro VerifyUserIsAdmin
  UserInfo::GetAccountType
  pop $0
  ${If} $0 != "admin"
	BringToFront
    messageBox mb_iconstop "Sorry, Administrator rights are required!"
      setErrorLevel 740
        quit
  ${EndIf}
!macroend

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; MUI: Default configuration

!define MUI_ABORTWARNING
!define MUI_ICON   "${ICON}"
!define MUI_UNICON "${ICON}"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; MUI: Installer pages

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "${LICENSE_TEXT_FILE}"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; MUI: Uninstaller pages

!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; MUI: Languages

!insertmacro MUI_LANGUAGE "English"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Section: DPS8M Simulator

LangString DESC_DPS8M ${LANG_ENGLISH} "DPS8M is a simulator of the 36‑bit GE Large Systems / Honeywell / Bull 600/6000‑series mainframe computers."
Section "DPS8M Simulator" DPS8M
  SectionIn RO
  SetOutPath $INSTDIR
  ${If} ${RunningX64}
    SetRegView 64
    File "/oname=dps8.exe" "bin/64-bit/dps8.exe"
  ${Else}
    SetRegView 32
    File "/oname=dps8.exe" "bin/32-bit/dps8.exe"
  ${EndIf}
SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Section: DPS8M Omnibus Documentation

LangString DESC_OMNIBUS ${LANG_ENGLISH} "The DPS8M Omnibus Documentation is the definitive reference to the simulator and utilities, in (PDF) ISO 32000 Portable Document Format."
Section "Omnibus Documentation" OMNIBUS
  SetOutPath $INSTDIR
  File "share/dps8m-omnibus.pdf"
SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Section: PunUtil

LangString DESC_PUNUTIL ${LANG_ENGLISH} "PunUtil is a utility to process the output of the DPS8M punch device."
Section "PunUtil" PUNUTIL
  SetOutPath $INSTDIR
  ${If} ${RunningX64}
    SetRegView 64
    File "/oname=punutil.exe" "bin/64-bit/punutil.exe"
  ${Else}
    SetRegView 32
    File "/oname=punutil.exe" "bin/32-bit/punutil.exe"
  ${EndIf}
SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Section: tap2raw

LangString DESC_TAP2RAW ${LANG_ENGLISH} "tap2raw is a utility to convert the tap-format output of the simulated DPS8M tape device to raw format."
Section "tap2raw" TAP2RAW
  SetOutPath $INSTDIR
  ${If} ${RunningX64}
    SetRegView 64
    File "/oname=tap2raw.exe" "bin/64-bit/tap2raw.exe"
  ${Else}
    SetRegView 32
    File "/oname=tap2raw.exe" "bin/32-bit/tap2raw.exe"
  ${EndIf}
SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Section: Prt2PDF

LangString DESC_PRT2PDF ${LANG_ENGLISH} "Prt2PDF is a utility to convert the Prt-format output of the simulated DPS8M line printer device to (PDF) ISO 32000 Portable Document Format."
Section "Prt2PDF" PRT2PDF
  SetOutPath $INSTDIR
  ${If} ${RunningX64}
    SetRegView 64
    File "/oname=prt2pdf.exe" "bin/64-bit/prt2pdf.exe"
  ${Else}
    SetRegView 32
    File "/oname=prt2pdf.exe" "bin/32-bit/prt2pdf.exe"
  ${EndIf}
SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Section: Source Code

LangString DESC_SOURCES ${LANG_ENGLISH} "A DEFLATE-compressed ZIP archive containing the DPS8M source code."
Section /o "DPS8M Source Code" SOURCES
  SetOutPath $INSTDIR
  File "/oname=dps8m-sources.zip" "source/dps8m-sources.zip"
SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Installer: MUI descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
!insertmacro MUI_DESCRIPTION_TEXT ${DPS8M}   $(DESC_DPS8M)
!insertmacro MUI_DESCRIPTION_TEXT ${PUNUTIL} $(DESC_PUNUTIL)
!insertmacro MUI_DESCRIPTION_TEXT ${TAP2RAW} $(DESC_TAP2RAW)
!insertmacro MUI_DESCRIPTION_TEXT ${PRT2PDF} $(DESC_PRT2PDF)
!insertmacro MUI_DESCRIPTION_TEXT ${OMNIBUS} $(DESC_OMNIBUS)
!insertmacro MUI_DESCRIPTION_TEXT ${SOURCES} $(DESC_SOURCES)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Installer: Default script

Section "-"
  File "/oname=LICENSE.md" "share/LICENSE.md"
SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Installer: Post-install script

Section "-hidden.postinstall"
  BringToFront
  writeUninstaller "$INSTDIR\uninstall.exe"
SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Function: Uninstaller: Init

function un.onInit
  BringToFront
  MessageBox MB_OKCANCEL "Do you really want to uninstall ${APPNAME}?" IDOK IsOK
    Abort
IsOK:
  !insertmacro VerifyUserIsAdmin
functionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;; Uninstaller: Uninstall script

Section "uninstall"
  delete "$INSTDIR\dps8.exe"
  delete "$INSTDIR\punutil.exe"
  delete "$INSTDIR\tap2raw.exe"
  delete "$INSTDIR\prt2pdf.exe"
  delete "$INSTDIR\dps8m-omnibus.pdf"
  delete "$INSTDIR\dps8m-sources.zip"
  delete "$INSTDIR\LICENSE.md"
  delete "$INSTDIR\uninstall.exe"
  rmDir  "$INSTDIR"
SectionEnd

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
