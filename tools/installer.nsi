; 中国象棋 Windows 安装脚本（NSIS）
Unicode true
Name "中国象棋"
OutFile "xiangqi-qt_0.1.0_x64-setup.exe"
InstallDir "$PROGRAMFILES64\XiangqiQt"
RequestExecutionLevel admin

Page directory
Page instfiles
UninstPage uninstConfirm
UninstPage instfiles

Section "MainSection" SEC01
  SetOutPath "$INSTDIR"
  File /r "win-dist\*.*"
  WriteUninstaller "$INSTDIR\uninstall.exe"
  CreateShortCut "$DESKTOP\中国象棋.lnk" "$INSTDIR\xiangqi-qt.exe"
  CreateDirectory "$SMPROGRAMS\中国象棋"
  CreateShortCut "$SMPROGRAMS\中国象棋\中国象棋.lnk" "$INSTDIR\xiangqi-qt.exe"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XiangqiQt" "DisplayName" "中国象棋 (C++/QML)"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XiangqiQt" "DisplayVersion" "0.1.0"
  WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XiangqiQt" "UninstallString" '"$INSTDIR\uninstall.exe"'
SectionEnd

Section "Uninstall"
  RMDir /r "$INSTDIR"
  Delete "$DESKTOP\中国象棋.lnk"
  RMDir /r "$SMPROGRAMS\中国象棋"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\XiangqiQt"
SectionEnd
