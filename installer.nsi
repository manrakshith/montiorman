; ==============================================================================
; MonitorMan - Modern Windows 11 NSIS Installer Script
; ==============================================================================

!include "MUI2.nsh"
!include "FileFunc.nsh"

; General Definitions
!define PRODUCT_NAME "MonitorMan"
!define PRODUCT_VERSION "1.0.0"
!define PRODUCT_PUBLISHER "MonitorMan"
!define PRODUCT_EXE "monitorman.exe"
!define PRODUCT_UNINST_EXE "Uninstall.exe"
!define PRODUCT_UNINST_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\MonitorMan"

; Installer file metadata
Name "${PRODUCT_NAME}"
OutFile "bin\MonitorMan_Setup.exe"
InstallDir "$LOCALAPPDATA\Programs\MonitorMan"
InstallDirRegKey HKCU "Software\MonitorMan" "InstallPath"
RequestExecutionLevel user

; Custom Icons
!define MUI_ICON "res\app.ico"
!define MUI_UNICON "res\app.ico"

; Modern UI Settings
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TITLE "Welcome to MonitorMan Setup"
!define MUI_WELCOMEPAGE_TEXT "MonitorMan allows you to control external monitor brightness and contrast with smooth daylight and circadian rhythm tracking.$\r$\n$\r$\nClick Next to continue."

; Pages
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES

; Finish page with launch option
!define MUI_FINISHPAGE_RUN "$INSTDIR\${PRODUCT_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "Launch MonitorMan now"
!insertmacro MUI_PAGE_FINISH

; Uninstaller Pages
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; Languages
!insertmacro MUI_LANGUAGE "English"

; ==============================================================================
; Installer Section
; ==============================================================================
Section "MainSection" SEC01
    ; Close existing running instance
    DetailPrint "Closing running instances of MonitorMan..."
    nsExec::Exec 'cmd /c "taskkill /F /IM ${PRODUCT_EXE} >nul 2>&1"'

    SetOutPath "$INSTDIR"
    SetOverwrite on

    ; Copy executable and icon
    File "bin\${PRODUCT_EXE}"
    File "res\app.ico"
    File "README.md"

    ; Store installation folder
    WriteRegStr HKCU "Software\MonitorMan" "InstallPath" "$INSTDIR"

    ; Create uninstaller
    WriteUninstaller "$INSTDIR\${PRODUCT_UNINST_EXE}"

    ; Start Menu Shortcuts
    CreateDirectory "$SMPROGRAMS\MonitorMan"
    CreateShortcut "$SMPROGRAMS\MonitorMan\MonitorMan.lnk" "$INSTDIR\${PRODUCT_EXE}" "" "$INSTDIR\${PRODUCT_EXE}" 0
    CreateShortcut "$SMPROGRAMS\MonitorMan\Uninstall MonitorMan.lnk" "$INSTDIR\${PRODUCT_UNINST_EXE}" "" "$INSTDIR\${PRODUCT_UNINST_EXE}" 0

    ; Desktop Shortcut
    CreateShortcut "$DESKTOP\MonitorMan.lnk" "$INSTDIR\${PRODUCT_EXE}" "" "$INSTDIR\${PRODUCT_EXE}" 0

    ; Register in Windows Installed Apps (Settings -> Apps)
    WriteRegStr HKCU "${PRODUCT_UNINST_KEY}" "DisplayName" "${PRODUCT_NAME}"
    WriteRegStr HKCU "${PRODUCT_UNINST_KEY}" "DisplayVersion" "${PRODUCT_VERSION}"
    WriteRegStr HKCU "${PRODUCT_UNINST_KEY}" "Publisher" "${PRODUCT_PUBLISHER}"
    WriteRegStr HKCU "${PRODUCT_UNINST_KEY}" "DisplayIcon" "$INSTDIR\${PRODUCT_EXE},0"
    WriteRegStr HKCU "${PRODUCT_UNINST_KEY}" "UninstallString" '"$INSTDIR\${PRODUCT_UNINST_EXE}"'
    WriteRegStr HKCU "${PRODUCT_UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\${PRODUCT_UNINST_EXE}" /S'
    WriteRegDWORD HKCU "${PRODUCT_UNINST_KEY}" "NoModify" 1
    WriteRegDWORD HKCU "${PRODUCT_UNINST_KEY}" "NoRepair" 1

    ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
    IntFmt $0 "0x%08X" $0
    WriteRegDWORD HKCU "${PRODUCT_UNINST_KEY}" "EstimatedSize" "$0"
SectionEnd

; ==============================================================================
; Uninstaller Section
; ==============================================================================
Section "Uninstall"
    ; Terminate running instance before uninstalling
    nsExec::Exec 'cmd /c "taskkill /F /IM ${PRODUCT_EXE} >nul 2>&1"'

    ; Remove Run with Windows registry entry
    DeleteRegValue HKCU "Software\Microsoft\Windows\CurrentVersion\Run" "MonitorMan"

    ; Remove Shortcuts
    Delete "$DESKTOP\MonitorMan.lnk"
    Delete "$SMPROGRAMS\MonitorMan\MonitorMan.lnk"
    Delete "$SMPROGRAMS\MonitorMan\Uninstall MonitorMan.lnk"
    RMDir "$SMPROGRAMS\MonitorMan"

    ; Remove Files
    Delete "$INSTDIR\${PRODUCT_EXE}"
    Delete "$INSTDIR\app.ico"
    Delete "$INSTDIR\README.md"
    Delete "$INSTDIR\${PRODUCT_UNINST_EXE}"
    RMDir "$INSTDIR"

    ; Clean Registry
    DeleteRegKey HKCU "${PRODUCT_UNINST_KEY}"
    DeleteRegKey HKCU "Software\MonitorMan"
SectionEnd
