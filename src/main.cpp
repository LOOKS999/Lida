#define UNICODE
#define _UNICODE
#define _CRT_NON_CONFORMING_WCSTOK
#include <windows.h>
#include <shellapi.h>
#include <winreg.h>
#include <shlobj.h>
#include <commdlg.h>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <vector>
#include <dwmapi.h>
#include <uxtheme.h>
#pragma comment(lib, "dwmapi.lib")
#pragma comment(lib, "uxtheme.lib")
#pragma comment(lib, "user32.lib")
#pragma comment(lib, "gdi32.lib")
#pragma comment(lib, "shell32.lib")
#pragma comment(lib, "advapi32.lib")
#pragma comment(lib, "comdlg32.lib")
#define WM_TRAYICON (WM_USER + 1)
#define ID_TRAY_EXIT 1001
#define ID_TRAY_RELOAD 1002
#define ID_TRAY_APPLY 1003
#define ID_TRAY_OPENCFG 1004
#define ID_TRAY_SET_LIGHT_WALL 1005
#define ID_TRAY_SET_DARK_WALL 1006
#define ID_TRAY_LIGHT_CURSOR_SCHEME 1007
#define ID_TRAY_DARK_CURSOR_SCHEME 1008
#define ID_TRAY_AUTO_SWITCH 1009
#define ID_TRAY_RESTORE_CURSORS 1010
#define ID_TRAY_LOCKSCREEN 1011
#define ID_TRAY_STARTUP 1012
#define ID_TRAY_ELEVATE 1013
#define ID_INPUT_EDIT 2001
#define ID_INPUT_OK 2002
#define ID_INPUT_CANCEL 2003
using namespace std;
namespace fs = std::filesystem;
struct ModeConfig { wstring wallpaper; wstring cursorScheme; };
struct AppConfig { int check_interval_ms = 1000; bool auto_switch = true; bool lock_screen = false; bool start_with_windows = true; wstring ui_style = L"modern"; wstring ui_theme = L"system"; wstring ui_bg = L"#F8F7F4"; wstring ui_accent = L"#7F9C90"; wstring ui_font = L"Segoe UI"; int ui_font_size = 10; bool ui_anim = true; ModeConfig light; ModeConfig dark; };
static HWND g_hWnd = NULL;
static NOTIFYICONDATAW g_nid = {0};
static UINT g_uID = 1;
static AppConfig g_cfg;
static wstring g_exeDir;
static wstring g_configPath;
static bool g_lastIsLight = true;
static bool g_running = true;
static HWND g_hInputDlg = NULL;
static HWND g_hInputEdit = NULL;
static wstring g_inputResult = L"";
static bool g_inputDone = false;
static wstring g_inputPrompt = L"";
static wstring g_inputTitle = L"";
static string WtoA(const wstring &w){ if(w.empty())return string(); int len=WideCharToMultiByte(CP_UTF8,0,w.c_str(),(int)w.size(),0,0,0,0); string s(len,0); WideCharToMultiByte(CP_UTF8,0,w.c_str(),(int)w.size(),&s[0],len,0,0); return s; }
static wstring AtoW(const string &a){ if(a.empty())return wstring(); int len=MultiByteToWideChar(CP_UTF8,0,a.c_str(),(int)a.size(),0,0); wstring w(len,0); MultiByteToWideChar(CP_UTF8,0,a.c_str(),(int)a.size(),&w[0],len); return w; }
static wstring ReadFileToWString(const wstring &path){ wifstream f(path,ios::binary); if(!f)return L""; wstringstream ss; ss<<f.rdbuf(); return ss.str(); }
static string ExtractString(const string &json,const string &key){ string pat="\""+key+"\""; size_t p=json.find(pat); if(p==string::npos)return""; p=json.find(':',p); if(p==string::npos)return""; p=json.find('"',p); if(p==string::npos)return""; size_t e=json.find('"',p+1); if(e==string::npos)return""; return json.substr(p+1,e-(p+1)); }
static bool ExtractBool(const string &json,const string &key,bool def){ string pat="\""+key+"\""; size_t p=json.find(pat); if(p==string::npos)return def; p=json.find(':',p); if(p==string::npos)return def; p++; while(p<json.size()&&isspace((unsigned char)json[p]))p++; if(p+4<=json.size()&&json.compare(p,4,"true")==0)return true; if(p+5<=json.size()&&json.compare(p,5,"false")==0)return false; return def; }
static int ExtractInt(const string &json,const string &key,int def){ string pat="\""+key+"\""; size_t p=json.find(pat); if(p==string::npos)return def; p=json.find(':',p); if(p==string::npos)return def; while(p<json.size()&&(isspace((unsigned char)json[p])||json[p]==':'))p++; if(p>=json.size()||(!isdigit((unsigned char)json[p])&&json[p]!='-'))return def; size_t s=p; if(json[p]=='-')p++; while(p<json.size()&&isdigit((unsigned char)json[p]))p++; try{return stoi(json.substr(s,p-s));}catch(...){return def;} }
static void SaveConfig(){ wofstream f(g_configPath,ios::binary); if(!f)return; f<<L"{\n"; f<<L"  \"check_interval_ms\": "<<g_cfg.check_interval_ms<<L",\n"; f<<L"  \"auto_switch\": "<<(g_cfg.auto_switch?L"true":L"false")<<L",\n"; f<<L"  \"lock_screen\": "<<(g_cfg.lock_screen?L"true":L"false")<<L",\n"; f<<L"  \"start_with_windows\": "<<(g_cfg.start_with_windows?L"true":L"false")<<L",\n"; f<<L"  \"ui_style\": \""<<g_cfg.ui_style<<L"\",\n"; f<<L"  \"ui_theme\": \""<<g_cfg.ui_theme<<L"\",\n"; f<<L"  \"ui_bg\": \""<<g_cfg.ui_bg<<L"\",\n"; f<<L"  \"ui_accent\": \""<<g_cfg.ui_accent<<L"\",\n"; f<<L"  \"ui_font\": \""<<g_cfg.ui_font<<L"\",\n"; f<<L"  \"ui_font_size\": "<<g_cfg.ui_font_size<<L",\n"; f<<L"  \"ui_anim\": "<<(g_cfg.ui_anim?L"true":L"false")<<L",\n"; f<<L"  \"light\": {\n"; f<<L"    \"wallpaper\": \""<<AtoW(WtoA(g_cfg.light.wallpaper))<<L"\",\n"; f<<L"    \"cursorScheme\": \""<<AtoW(WtoA(g_cfg.light.cursorScheme))<<L"\"\n"; f<<L"  },\n"; f<<L"  \"dark\": {\n"; f<<L"    \"wallpaper\": \""<<AtoW(WtoA(g_cfg.dark.wallpaper))<<L"\",\n"; f<<L"    \"cursorScheme\": \""<<AtoW(WtoA(g_cfg.dark.cursorScheme))<<L"\"\n"; f<<L"  }\n"; f<<L"}\n"; }
static wstring ConfigDir(){ wchar_t *p=NULL; size_t n=0; wstring dir; if(_wdupenv_s(&p,&n,L"APPDATA")==0&&p){ if(p[0])dir=wstring(p)+L"\\Lida"; free(p); } if(!dir.empty()){ std::error_code ec; fs::create_directories(dir,ec); if(!ec||fs::exists(dir))return dir; } return g_exeDir; }
static void LoadConfig(){ AppConfig cfg; cfg.check_interval_ms=1000; cfg.auto_switch=true; wstring raw=ReadFileToWString(g_configPath); string json=WtoA(raw); if(!json.empty()){ cfg.check_interval_ms=ExtractInt(json,"check_interval_ms",1000); cfg.auto_switch=ExtractBool(json,"auto_switch",true); cfg.lock_screen=ExtractBool(json,"lock_screen",false); cfg.start_with_windows=ExtractBool(json,"start_with_windows",true); cfg.ui_style=AtoW(ExtractString(json,"ui_style")); if(cfg.ui_style.empty())cfg.ui_style=L"modern"; cfg.ui_theme=AtoW(ExtractString(json,"ui_theme")); if(cfg.ui_theme.empty())cfg.ui_theme=L"system"; cfg.ui_bg=AtoW(ExtractString(json,"ui_bg")); if(cfg.ui_bg.size()<7)cfg.ui_bg=L"#F8F7F4"; cfg.ui_accent=AtoW(ExtractString(json,"ui_accent")); if(cfg.ui_accent.size()<7)cfg.ui_accent=L"#7F9C90"; cfg.ui_font=AtoW(ExtractString(json,"ui_font")); if(cfg.ui_font.empty())cfg.ui_font=L"Segoe UI"; cfg.ui_font_size=ExtractInt(json,"ui_font_size",10); if(cfg.ui_font_size<7||cfg.ui_font_size>48)cfg.ui_font_size=10; cfg.ui_anim=ExtractBool(json,"ui_anim",true); auto lm=[&](const string &name,ModeConfig &m){ size_t p=json.find("\""+name+"\""); if(p==string::npos)return; size_t b=json.find('{',p); if(b==string::npos)return; size_t e=json.find('}',b); if(e==string::npos)return; string block=json.substr(b,e-b+1); m.wallpaper=AtoW(ExtractString(block,"wallpaper")); m.cursorScheme=AtoW(ExtractString(block,"cursorScheme")); }; lm("light",cfg.light); lm("dark",cfg.dark); } g_cfg=cfg; if(json.empty())SaveConfig(); }
static bool IsLightMode(){ HKEY hKey; DWORD val=1; DWORD size=sizeof(DWORD); LONG res=RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",0,KEY_READ,&hKey); if(res!=ERROR_SUCCESS)return true; res=RegQueryValueExW(hKey,L"AppsUseLightTheme",NULL,NULL,(LPBYTE)&val,&size); RegCloseKey(hKey); if(res!=ERROR_SUCCESS)return true; return val==1; }
static void ApplyWallpaper(const wstring &path){ if(path.empty())return; wstring full=path; if(full.size()<2||!(full[1]==L':'||full[0]==L'\\')){ full=(fs::path(g_exeDir)/fs::path(path)).wstring(); } if(!fs::exists(full))return; SystemParametersInfoW(SPI_SETDESKWALLPAPER,0,(PVOID)full.c_str(),SPIF_UPDATEINIFILE|SPIF_SENDCHANGE); }
static void RestoreSystemCursors(){ SystemParametersInfoW(SPI_SETCURSORS,0,NULL,SPIF_UPDATEINIFILE|SPIF_SENDCHANGE); SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, 0, (LPARAM)L"UserEnvironment"); }
static wstring ExpandEnv(const wchar_t *s){ WCHAR buf[32768]; DWORD n=ExpandEnvironmentStringsW(s,buf,32768); if(n>0&&n<32768)return wstring(buf); return wstring(s); }
static void ApplyCursorScheme(const wstring &schemeName){ if(schemeName.empty())return;
    HKEY hSchemes = NULL;
    if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Control Panel\\Cursors\\Schemes",0,KEY_READ,&hSchemes)==ERROR_SUCCESS){
        WCHAR buf[4096]={0}; DWORD sz=sizeof(buf);
        if(RegQueryValueExW(hSchemes,schemeName.c_str(),NULL,NULL,(LPBYTE)buf,&sz)==ERROR_SUCCESS){
            wchar_t *p = buf;
            wchar_t *next = wcstok(p,L",");
            int idx=0;
            const wchar_t* names[] = {L"Arrow",L"Help",L"AppStarting",L"Wait",L"Crosshair",L"IBeam",L"NWPen",L"No",L"SizeNS",L"SizeWE",L"SizeNWSE",L"SizeNESW",L"SizeAll",L"UpArrow",L"Hand",L"Pin",L"Person"};
            HKEY hCursors = NULL;
            if(RegCreateKeyExW(HKEY_CURRENT_USER,L"Control Panel\\Cursors",0,NULL,REG_OPTION_NON_VOLATILE,KEY_WRITE,NULL,&hCursors,NULL)==ERROR_SUCCESS){
                while(next && idx < 17){
                    wstring expanded=ExpandEnv(next); RegSetValueExW(hCursors,names[idx],0,REG_SZ,(const BYTE*)expanded.c_str(),(DWORD)((expanded.size()+1)*sizeof(WCHAR)));
                    next = wcstok(NULL,L",");
                    idx++;
                }
                RegSetValueExW(hCursors,L"",0,REG_SZ,(const BYTE*)schemeName.c_str(),(DWORD)((schemeName.size()+1)*sizeof(WCHAR)));
                DWORD src=1;
                RegSetValueExW(hCursors,L"Scheme Source",0,REG_DWORD,(const BYTE*)&src,sizeof(DWORD));
                RegCloseKey(hCursors);
            }
        }
        RegCloseKey(hSchemes);
    }
    SystemParametersInfoW(SPI_SETCURSORS,0,NULL,SPIF_UPDATEINIFILE|SPIF_SENDCHANGE);
    SendNotifyMessage(HWND_BROADCAST, WM_SETTINGCHANGE, SPI_SETICONS, (LPARAM)L"windows"); }
static wstring ExePath(){ WCHAR p[MAX_PATH]={0}; GetModuleFileNameW(NULL,p,MAX_PATH); return wstring(p); }
static bool IsElevated(){ HANDLE t=NULL; if(!OpenProcessToken(GetCurrentProcess(),TOKEN_QUERY,&t))return false; TOKEN_ELEVATION e={0}; DWORD sz=sizeof(e); bool ok=GetTokenInformation(t,TokenElevation,&e,sizeof(e),&sz)&&e.TokenIsElevated!=0; CloseHandle(t); return ok; }
static bool ApplyLockScreen(const wstring &path){ if(path.empty())return true; wstring full=path; if(full.size()<2||!(full[1]==L':'||full[0]==L'\\')) full=(fs::path(g_exeDir)/fs::path(path)).wstring(); if(!fs::exists(full))return false; HKEY h=NULL; if(RegCreateKeyExW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PersonalizationCSP",0,NULL,REG_OPTION_NON_VOLATILE,KEY_SET_VALUE,NULL,&h,NULL)!=ERROR_SUCCESS)return false; LONG r=RegSetValueExW(h,L"LockScreenImagePath",0,REG_SZ,(const BYTE*)full.c_str(),(DWORD)((full.size()+1)*sizeof(WCHAR))); RegSetValueExW(h,L"LockScreenImageUrl",0,REG_SZ,(const BYTE*)full.c_str(),(DWORD)((full.size()+1)*sizeof(WCHAR))); DWORD on=1; RegSetValueExW(h,L"LockScreenImageStatus",0,REG_DWORD,(const BYTE*)&on,sizeof(on)); RegCloseKey(h); return r==ERROR_SUCCESS; }
static bool LockScreenActive(){ HKEY h=NULL; if(RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PersonalizationCSP",0,KEY_READ,&h)!=ERROR_SUCCESS)return false; DWORD st=0,sz=sizeof(st); LONG r=RegQueryValueExW(h,L"LockScreenImageStatus",NULL,NULL,(LPBYTE)&st,&sz); WCHAR p[1024]={0}; DWORD psz=sizeof(p); LONG r2=RegQueryValueExW(h,L"LockScreenImagePath",NULL,NULL,(LPBYTE)p,&psz); RegCloseKey(h); return r==ERROR_SUCCESS&&st==1&&r2==ERROR_SUCCESS&&p[0]!=0; }
static bool ClearLockScreen(){ HKEY h=NULL; if(RegOpenKeyExW(HKEY_LOCAL_MACHINE,L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\PersonalizationCSP",0,KEY_SET_VALUE,&h)!=ERROR_SUCCESS)return false; DWORD off=0; RegSetValueExW(h,L"LockScreenImageStatus",0,REG_DWORD,(const BYTE*)&off,sizeof(off)); RegDeleteValueW(h,L"LockScreenImagePath"); RegDeleteValueW(h,L"LockScreenImageUrl"); RegCloseKey(h); SendNotifyMessage(HWND_BROADCAST,WM_SETTINGCHANGE,0,(LPARAM)L"PersonalizationCSP"); return true; }
static void RelaunchElevated(){ HINSTANCE r=ShellExecuteW(NULL,L"runas",ExePath().c_str(),NULL,g_exeDir.c_str(),SW_SHOWNORMAL); if((INT_PTR)r>32){ g_running=false; PostQuitMessage(0); } }
static int RunHidden(const wstring &cmd){ STARTUPINFOW si={0}; si.cb=sizeof(si); si.dwFlags=STARTF_USESHOWWINDOW; si.wShowWindow=SW_HIDE; PROCESS_INFORMATION pi={0}; wstring line=L"cmd.exe /c "+cmd; if(!CreateProcessW(NULL,&line[0],NULL,NULL,FALSE,CREATE_NO_WINDOW,NULL,NULL,&si,&pi))return -1; WaitForSingleObject(pi.hProcess,15000); DWORD code=(DWORD)-1; GetExitCodeProcess(pi.hProcess,&code); CloseHandle(pi.hProcess); CloseHandle(pi.hThread); return (int)code; }
static bool StartupTaskExists(){ return RunHidden(L"schtasks /Query /TN \"Lida\" >nul 2>&1")==0; }
static void SetElevatedStartupTask(bool on){ if(on){ wstring tr=L"\""+ExePath()+L"\""; RunHidden(L"schtasks /Create /TN \"Lida\" /TR \""+tr+L"\" /SC ONLOGON /RL HIGHEST /F >nul 2>&1"); } else RunHidden(L"schtasks /Delete /TN \"Lida\" /F >nul 2>&1"); }
static bool RunValueExists(){ HKEY h=NULL; if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_READ,&h)!=ERROR_SUCCESS)return false; WCHAR buf[2048]={0}; DWORD sz=sizeof(buf); LONG r=RegQueryValueExW(h,L"Lida",NULL,NULL,(LPBYTE)buf,&sz); RegCloseKey(h); return r==ERROR_SUCCESS; }
static bool IsRunAtStartup(){ return RunValueExists()||StartupTaskExists(); }
static void WriteRunValue(){ HKEY h=NULL; if(RegCreateKeyExW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,NULL,REG_OPTION_NON_VOLATILE,KEY_SET_VALUE,NULL,&h,NULL)!=ERROR_SUCCESS)return; wstring exe=L"\""+ExePath()+L"\""; RegSetValueExW(h,L"Lida",0,REG_SZ,(const BYTE*)exe.c_str(),(DWORD)((exe.size()+1)*sizeof(WCHAR))); RegCloseKey(h); }
static void DeleteRunValue(){ HKEY h=NULL; if(RegOpenKeyExW(HKEY_CURRENT_USER,L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",0,KEY_SET_VALUE,&h)!=ERROR_SUCCESS)return; RegDeleteValueW(h,L"Lida"); RegCloseKey(h); }
static void SetRunAtStartup(bool on){ if(!on){ DeleteRunValue(); SetElevatedStartupTask(false); return; } if(StartupTaskExists()){ DeleteRunValue(); return; } if(IsElevated()){ SetElevatedStartupTask(true); if(StartupTaskExists()){ DeleteRunValue(); return; } } WriteRunValue(); }
static void ApplyForMode(bool isLight){ wstring wp=isLight?g_cfg.light.wallpaper:g_cfg.dark.wallpaper; ApplyWallpaper(wp); ApplyCursorScheme(isLight?g_cfg.light.cursorScheme:g_cfg.dark.cursorScheme); if(g_cfg.lock_screen)ApplyLockScreen(wp); }
static wstring BrowseForFile(const wchar_t *filter,const wchar_t *title){ OPENFILENAMEW ofn={0}; wchar_t szFile[MAX_PATH]={0}; ofn.lStructSize=sizeof(OPENFILENAMEW); ofn.hwndOwner=g_hWnd; ofn.lpstrFilter=filter; ofn.lpstrFile=szFile; ofn.nMaxFile=MAX_PATH; ofn.lpstrTitle=title; ofn.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR; if(GetOpenFileNameW(&ofn))return wstring(szFile); return L""; }
/* ===================== Ui: palette, fonts, panel, splash, settings ===================== */
static COLORREF uiHex(const wstring &s,COLORREF def){ if(s.size()>=7&&s[0]==L'#'){ int r=(int)wcstol(s.substr(1,2).c_str(),NULL,16),g=(int)wcstol(s.substr(3,2).c_str(),NULL,16),b=(int)wcstol(s.substr(5,2).c_str(),NULL,16); return RGB(r,g,b);} return def; }
static wstring uiHexStr(COLORREF c){ WCHAR b[16]; swprintf_s(b,L"#%02X%02X%02X",GetRValue(c),GetGValue(c),GetBValue(c)); return wstring(b); }
static COLORREF uiMix(COLORREF a,COLORREF b,double t){ return RGB((int)(GetRValue(a)+(GetRValue(b)-GetRValue(a))*t),(int)(GetGValue(a)+(GetGValue(b)-GetGValue(a))*t),(int)(GetBValue(a)+(GetBValue(b)-GetBValue(a))*t)); }
static int uiLuma(COLORREF c){ return (GetRValue(c)*299+GetGValue(c)*587+GetBValue(c)*114)/1000; }
struct UiPalette{ COLORREF bg,surface,text,muted,accent,accentText,hover,border; };
static UiPalette UiColors(){ UiPalette p; bool dark;
  if(g_cfg.ui_theme==L"light")dark=false; else if(g_cfg.ui_theme==L"dark")dark=true; else if(g_cfg.ui_theme==L"custom")dark=uiLuma(uiHex(g_cfg.ui_bg,RGB(248,247,244)))<128; else dark=!IsLightMode();
  if(g_cfg.ui_theme==L"custom"){ p.bg=uiHex(g_cfg.ui_bg,RGB(248,247,244)); p.accent=uiHex(g_cfg.ui_accent,RGB(127,156,144)); p.text=dark?RGB(240,240,240):RGB(38,38,38); }
  else if(dark){ p.bg=RGB(26,27,29); p.accent=uiHex(g_cfg.ui_accent,RGB(150,178,164)); p.text=RGB(238,238,238); }
  else { p.bg=RGB(248,247,244); p.accent=uiHex(g_cfg.ui_accent,RGB(127,156,144)); p.text=RGB(40,40,40); }
  p.surface=dark?uiMix(p.bg,RGB(255,255,255),0.07):RGB(255,255,255);
  p.hover=uiMix(p.surface,p.accent,0.20); p.border=uiMix(p.bg,p.text,0.16); p.muted=uiMix(p.text,p.bg,0.45);
  p.accentText=uiLuma(p.accent)>150?RGB(25,25,25):RGB(255,255,255); return p; }
static HFONT uiFont(int pt,int weight){ HDC dc=GetDC(NULL); int h=-MulDiv(pt,GetDeviceCaps(dc,LOGPIXELSY),72); ReleaseDC(NULL,dc); LOGFONTW lf={0}; lf.lfHeight=h; lf.lfWeight=weight; lf.lfQuality=CLEARTYPE_QUALITY; lf.lfCharSet=DEFAULT_CHARSET; wcsncpy_s(lf.lfFaceName,LF_FACESIZE,g_cfg.ui_font.c_str(),_TRUNCATE); return CreateFontIndirectW(&lf); }
static void uiText(HDC dc,const wstring &s,RECT r,COLORREF c,HFONT f,UINT fmt){ HFONT of=(HFONT)SelectObject(dc,f); int om=SetBkMode(dc,TRANSPARENT); SetTextColor(dc,c); DrawTextW(dc,s.c_str(),-1,&r,fmt); SetBkMode(dc,om); SelectObject(dc,of); }
static void uiSwitch(HDC dc,int x,int y,bool on,const UiPalette &p){ int w=36,h=18; COLORREF track=on?p.accent:uiMix(p.bg,p.text,0.28); HBRUSH br=CreateSolidBrush(track); HPEN pen=CreatePen(PS_SOLID,1,track); HGDIOBJ ob=SelectObject(dc,br); HGDIOBJ op=SelectObject(dc,pen); RoundRect(dc,x,y,x+w,y+h,18,18); SelectObject(dc,ob); SelectObject(dc,op); DeleteObject(br); DeleteObject(pen); int kx=on?x+w-15:x+3; HBRUSH kb=CreateSolidBrush(RGB(255,255,255)); HGDIOBJ ob2=SelectObject(dc,kb); HGDIOBJ op2=SelectObject(dc,GetStockObject(NULL_PEN)); Ellipse(dc,kx,y+3,kx+12,y+15); SelectObject(dc,ob2); SelectObject(dc,op2); DeleteObject(kb); }

#define ID_TRAY_SETTINGS 1014
#define IDC_UI_STYLE 3001
#define IDC_UI_THEME 3002
#define IDC_UI_BG 3003
#define IDC_UI_ACCENT 3004
#define IDC_UI_FONT 3005
#define IDC_UI_ANIM 3006
#define IDC_UI_CLOSE 3007
#define IDC_UI_FONTLBL 3008
#define UI_PANEL_W 334
#define UI_PAD 10
#define UI_HEAD 46
struct UiItem{ wstring label; int cmd; bool header; bool sep; bool toggle; bool on; };
static vector<UiItem> g_items;
static HWND g_hPanel=NULL;
static int g_hot=-1,g_alpha=255;
static void UiBuildItems(){ g_items.clear();
  auto H=[&](const wchar_t *t){ g_items.push_back(UiItem{t,0,true,false,false,false}); };
  auto R=[&](const wchar_t *t,int c){ g_items.push_back(UiItem{t,c,false,false,false,false}); };
  auto S=[&](){ g_items.push_back(UiItem{L"",0,false,true,false,false}); };
  auto T=[&](const wchar_t *t,int c,bool on){ g_items.push_back(UiItem{t,c,false,false,true,on}); };
  H(L"Light mode"); R(L"Set wallpaper",ID_TRAY_SET_LIGHT_WALL); R(L"Cursor scheme",ID_TRAY_LIGHT_CURSOR_SCHEME); S();
  H(L"Dark mode"); R(L"Set wallpaper",ID_TRAY_SET_DARK_WALL); R(L"Cursor scheme",ID_TRAY_DARK_CURSOR_SCHEME); S();
  T(L"Auto switch",ID_TRAY_AUTO_SWITCH,g_cfg.auto_switch); T(L"Lock screen",ID_TRAY_LOCKSCREEN,LockScreenActive()); T(L"Start with Windows",ID_TRAY_STARTUP,IsRunAtStartup()); S();
  R(L"Apply now",ID_TRAY_APPLY); R(L"Restore default cursors",ID_TRAY_RESTORE_CURSORS); R(L"UI Settings...",ID_TRAY_SETTINGS); S();
  if(!IsElevated())R(L"Restart as Administrator",ID_TRAY_ELEVATE);
  R(L"Reload config",ID_TRAY_RELOAD);
  R(L"Open config folder",ID_TRAY_OPENCFG); R(L"Exit",ID_TRAY_EXIT); }
static int UiItemH(const UiItem &it){ return it.sep?9:(it.header?30:40); }
static int UiPanelH(){ int y=UI_PAD*2+UI_HEAD; for(auto &it:g_items)y+=UiItemH(it); return y; }
static bool UiItemRect(int i,RECT *r){ if(i<0||i>=(int)g_items.size())return false; int y=UI_PAD+UI_HEAD; for(int k=0;k<i;k++)y+=UiItemH(g_items[k]); r->left=UI_PAD; r->right=UI_PANEL_W-UI_PAD; r->top=y; r->bottom=y+UiItemH(g_items[i]); return true; }
static int UiHit(int x,int y){ for(size_t i=0;i<g_items.size();i++){ RECT r; if(!UiItemRect((int)i,&r))continue; if(x>=r.left&&x<r.right&&y>=r.top&&y<r.bottom)return (int)i; } return -1; }
static HFONT g_pfText=NULL,g_pfBold=NULL,g_pfHead=NULL;
static void UiFreePanelFonts(){ if(g_pfText){DeleteObject(g_pfText);g_pfText=NULL;} if(g_pfBold){DeleteObject(g_pfBold);g_pfBold=NULL;} if(g_pfHead){DeleteObject(g_pfHead);g_pfHead=NULL;} }
static void UiMakePanelFonts(){ if(g_pfText)return; g_pfText=uiFont(g_cfg.ui_font_size,FW_NORMAL); g_pfBold=uiFont(g_cfg.ui_font_size,FW_SEMIBOLD); g_pfHead=uiFont(g_cfg.ui_font_size+2,FW_SEMIBOLD); }
static void UiInvalidateRow(HWND h,int i){ RECT r; if(UiItemRect(i,&r))InvalidateRect(h,&r,FALSE); }
static void UiPaintPanel(HWND h){ PAINTSTRUCT ps; HDC dc=BeginPaint(h,&ps); RECT rc; GetClientRect(h,&rc); UiMakePanelFonts();
  HDC buf=CreateCompatibleDC(dc); HBITMAP bmp=CreateCompatibleBitmap(dc,rc.right,rc.bottom); HGDIOBJ oldBmp=SelectObject(buf,bmp);
  UiPalette p=UiColors(); HBRUSH bg=CreateSolidBrush(p.surface); FillRect(buf,&rc,bg); DeleteObject(bg);
  HFONT f=g_pfText,fb=g_pfBold,fh=g_pfHead;
  RECT hr={UI_PAD+8,UI_PAD,UI_PANEL_W-UI_PAD,UI_PAD+UI_HEAD-6}; uiText(buf,L"Lida",hr,p.text,fh,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
  uiText(buf,IsLightMode()?L"Light theme":L"Dark theme",hr,p.muted,f,DT_RIGHT|DT_VCENTER|DT_SINGLELINE);
  for(size_t i=0;i<g_items.size();i++){ UiItem &it=g_items[i]; RECT r; UiItemRect((int)i,&r);
    if(it.sep){ RECT l={r.left+8,(r.top+r.bottom)/2,r.right-8,(r.top+r.bottom)/2+1}; HBRUSH b=CreateSolidBrush(p.border); FillRect(buf,&l,b); DeleteObject(b); continue; }
    if(it.header){ RECT t={r.left+8,r.top,r.right,r.bottom}; uiText(buf,it.label,t,p.muted,fb,DT_LEFT|DT_VCENTER|DT_SINGLELINE); continue; }
    if((int)i==g_hot){ HBRUSH hb=CreateSolidBrush(p.hover); HPEN hp=CreatePen(PS_SOLID,1,p.hover); HGDIOBJ ob=SelectObject(buf,hb); HGDIOBJ op=SelectObject(buf,hp); RoundRect(buf,r.left,r.top,r.right,r.bottom,10,10); SelectObject(buf,ob); SelectObject(buf,op); DeleteObject(hb); DeleteObject(hp); }
    RECT t={r.left+10,r.top,r.right-(it.toggle?56:10),r.bottom}; uiText(buf,it.label,t,p.text,f,DT_LEFT|DT_VCENTER|DT_SINGLELINE);
    if(it.toggle)uiSwitch(buf,r.right-48,(r.top+r.bottom)/2-9,it.on,p); }
  BitBlt(dc,0,0,rc.right,rc.bottom,buf,0,0,SRCCOPY);
  SelectObject(buf,oldBmp); DeleteObject(bmp); DeleteDC(buf); EndPaint(h,&ps); }
static void UiHidePanel(){ if(g_hPanel){ HWND h=g_hPanel; g_hPanel=NULL; DestroyWindow(h);} UiFreePanelFonts(); }
static LRESULT CALLBACK UiPanelProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam){ switch(msg){
  case WM_ERASEBKGND: return 1;
  case WM_PAINT: UiPaintPanel(hWnd); return 0;
  case WM_MOUSEMOVE:{ int x=(int)(short)LOWORD(lParam),y=(int)(short)HIWORD(lParam); int i=UiHit(x,y); if(i!=g_hot){ int old=g_hot; g_hot=i; UiInvalidateRow(hWnd,old); UiInvalidateRow(hWnd,i); } TRACKMOUSEEVENT tme={sizeof(tme),TME_LEAVE,hWnd,0}; TrackMouseEvent(&tme); return 0; }
  case WM_MOUSELEAVE: if(g_hot>=0){ int old=g_hot; g_hot=-1; UiInvalidateRow(hWnd,old); } else g_hot=-1; return 0;
  case WM_LBUTTONUP:{ int x=(int)(short)LOWORD(lParam),y=(int)(short)HIWORD(lParam); int i=UiHit(x,y); if(i>=0){ UiItem it=g_items[i]; if(!it.header&&!it.sep&&it.cmd){ int cmd=it.cmd; if(it.toggle){ SendMessageW(g_hWnd,WM_COMMAND,MAKEWPARAM(cmd,0),0); if(g_hPanel==hWnd){ UiBuildItems(); InvalidateRect(hWnd,NULL,FALSE); } } else { UiHidePanel(); PostMessageW(g_hWnd,WM_COMMAND,MAKEWPARAM(cmd,0),0);} } } else UiHidePanel(); return 0; }
  case WM_TIMER: g_alpha+=55; if(g_alpha>=255){g_alpha=255;KillTimer(hWnd,1);} SetLayeredWindowAttributes(hWnd,0,(BYTE)g_alpha,LWA_ALPHA); return 0;
  case WM_ACTIVATE: if(LOWORD(wParam)==WA_INACTIVE)UiHidePanel(); return 0;
  case WM_KEYDOWN: if(wParam==VK_ESCAPE)UiHidePanel(); return 0;
  case WM_DESTROY: if(g_hPanel==hWnd)g_hPanel=NULL; return 0; }
  return DefWindowProcW(hWnd,msg,wParam,lParam); }
static void UiShowPanel(){ UiBuildItems(); g_hot=-1; UiHidePanel(); POINT pt; GetCursorPos(&pt); int w=UI_PANEL_W,h=UiPanelH(); MONITORINFO mi={sizeof(mi)}; GetMonitorInfoW(MonitorFromPoint(pt,MONITOR_DEFAULTTONEAREST),&mi); int x=pt.x-w/2; if(x<mi.rcWork.left+8)x=mi.rcWork.left+8; if(x+w>mi.rcWork.right-8)x=mi.rcWork.right-8-w; int y=pt.y-h-8; if(y<mi.rcWork.top+8)y=pt.y+8;
  g_hPanel=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED,L"LidaPanel",L"",WS_POPUP,x,y,w,h,g_hWnd,NULL,GetModuleHandleW(NULL),NULL); if(!g_hPanel)return;
  SetWindowRgn(g_hPanel,CreateRoundRectRgn(0,0,w+1,h+1,16,16),TRUE); g_alpha=g_cfg.ui_anim?0:255; SetLayeredWindowAttributes(g_hPanel,0,(BYTE)g_alpha,LWA_ALPHA);
  ShowWindow(g_hPanel,SW_SHOW); UpdateWindow(g_hPanel); SetForegroundWindow(g_hPanel); if(g_cfg.ui_anim)SetTimer(g_hPanel,1,15,NULL); }

static int g_spAlpha=0,g_spPhase=0,g_spHold=0;
static void UiPaintSplash(HWND h){ PAINTSTRUCT ps; HDC dc=BeginPaint(h,&ps); RECT rc; GetClientRect(h,&rc); UiPalette p=UiColors(); HBRUSH b=CreateSolidBrush(p.surface); FillRect(dc,&rc,b); DeleteObject(b); HFONT f=uiFont(g_cfg.ui_font_size+8,FW_SEMIBOLD); uiText(dc,L"Lida",rc,p.text,f,DT_CENTER|DT_VCENTER|DT_SINGLELINE); DeleteObject(f); EndPaint(h,&ps); }
static LRESULT CALLBACK UiSplashProc(HWND h,UINT m,WPARAM w,LPARAM l){ switch(m){ case WM_ERASEBKGND: return 1; case WM_PAINT: UiPaintSplash(h); return 0;
  case WM_TIMER: if(g_spPhase==0){ g_spAlpha+=45; if(g_spAlpha>=255){g_spAlpha=255;g_spPhase=1;g_spHold=20;} } else if(g_spPhase==1){ if(--g_spHold<=0)g_spPhase=2; } else { g_spAlpha-=25; if(g_spAlpha<=0){KillTimer(h,1);DestroyWindow(h);return 0;} } SetLayeredWindowAttributes(h,0,(BYTE)g_spAlpha,LWA_ALPHA); return 0; }
  return DefWindowProcW(h,m,w,l); }
static void UiShowSplash(){ if(!g_cfg.ui_anim)return; int w=260,h=120; int x=(GetSystemMetrics(SM_CXSCREEN)-w)/2,y=(GetSystemMetrics(SM_CYSCREEN)-h)/2; HWND hw=CreateWindowExW(WS_EX_TOPMOST|WS_EX_TOOLWINDOW|WS_EX_LAYERED|WS_EX_NOACTIVATE,L"LidaSplash",L"",WS_POPUP,x,y,w,h,NULL,NULL,GetModuleHandleW(NULL),NULL); if(!hw)return; SetWindowRgn(hw,CreateRoundRectRgn(0,0,w+1,h+1,18,18),TRUE); g_spAlpha=0;g_spPhase=0;g_spHold=0; SetLayeredWindowAttributes(hw,0,0,LWA_ALPHA); ShowWindow(hw,SW_SHOWNOACTIVATE); UpdateWindow(hw); SetTimer(hw,1,15,NULL); }

static HWND g_hSettings=NULL; static HBRUSH g_brSettings=NULL; static HBRUSH g_brSurface=NULL; static HFONT g_setFont=NULL;
static void UiRefreshSettings(HWND h);
static void UiApplyCtlFont(HWND parent){ if(g_setFont)DeleteObject(g_setFont); g_setFont=uiFont(g_cfg.ui_font_size,FW_NORMAL); for(HWND c=GetWindow(parent,GW_CHILD);c;c=GetWindow(c,GW_HWNDNEXT))SendMessageW(c,WM_SETFONT,(WPARAM)g_setFont,TRUE); }
static void UiRefreshSettings(HWND h){ WCHAR buf[160];
  swprintf_s(buf,L"Background  %s",g_cfg.ui_bg.c_str()); SetDlgItemTextW(h,IDC_UI_BG,buf);
  swprintf_s(buf,L"Accent  %s",g_cfg.ui_accent.c_str()); SetDlgItemTextW(h,IDC_UI_ACCENT,buf);
  swprintf_s(buf,L"%s, %d pt",g_cfg.ui_font.c_str(),g_cfg.ui_font_size); SetDlgItemTextW(h,IDC_UI_FONTLBL,buf);
  bool custom=g_cfg.ui_theme==L"custom"; EnableWindow(GetDlgItem(h,IDC_UI_BG),custom); EnableWindow(GetDlgItem(h,IDC_UI_ACCENT),custom);
  SendDlgItemMessageW(h,IDC_UI_ANIM,BM_SETCHECK,g_cfg.ui_anim?BST_CHECKED:BST_UNCHECKED,0);
  BOOL dark=uiLuma(UiColors().bg)<128; DwmSetWindowAttribute(h,20,&dark,sizeof(dark));
  if(g_brSettings)DeleteObject(g_brSettings); g_brSettings=CreateSolidBrush(UiColors().bg);
  if(g_brSurface)DeleteObject(g_brSurface); g_brSurface=CreateSolidBrush(UiColors().surface);
  SetWindowTheme(GetDlgItem(h,IDC_UI_STYLE),dark?L"DarkMode_Explorer":NULL,NULL);
  SetWindowTheme(GetDlgItem(h,IDC_UI_THEME),dark?L"DarkMode_Explorer":NULL,NULL);
  InvalidateRect(h,NULL,TRUE); }
static LRESULT CALLBACK UiSettingsProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam){ switch(msg){
  case WM_CREATE:{ HINSTANCE hi=GetModuleHandleW(NULL);
    auto C=[&](const wchar_t *cls,const wchar_t *txt,DWORD st,int x,int y,int w,int h,int id){ return CreateWindowExW(0,cls,txt,WS_CHILD|WS_VISIBLE|st,x,y,w,h,hWnd,(HMENU)(INT_PTR)id,hi,NULL); };
    C(L"STATIC",L"Tray interface",0,24,24,150,20,0);
    HWND cb=C(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL,184,22,312,220,IDC_UI_STYLE);
    SendMessageW(cb,CB_ADDSTRING,0,(LPARAM)L"Classic menu"); SendMessageW(cb,CB_ADDSTRING,0,(LPARAM)L"Modern panel"); SendMessageW(cb,CB_SETCURSEL,g_cfg.ui_style==L"classic"?0:1,0);
    C(L"STATIC",L"Appearance",0,24,66,150,20,0);
    HWND tb=C(L"COMBOBOX",L"",CBS_DROPDOWNLIST|WS_VSCROLL,184,64,312,220,IDC_UI_THEME);
    SendMessageW(tb,CB_ADDSTRING,0,(LPARAM)L"Light"); SendMessageW(tb,CB_ADDSTRING,0,(LPARAM)L"Dark"); SendMessageW(tb,CB_ADDSTRING,0,(LPARAM)L"System"); SendMessageW(tb,CB_ADDSTRING,0,(LPARAM)L"Custom colors");
    int sel=g_cfg.ui_theme==L"light"?0:(g_cfg.ui_theme==L"dark"?1:(g_cfg.ui_theme==L"custom"?3:2)); SendMessageW(tb,CB_SETCURSEL,sel,0);
    C(L"STATIC",L"Colors",0,24,110,150,20,0);
    C(L"BUTTON",L"Background",BS_PUSHBUTTON,184,106,150,30,IDC_UI_BG);
    C(L"BUTTON",L"Accent",BS_PUSHBUTTON,346,106,150,30,IDC_UI_ACCENT);
    C(L"STATIC",L"Font",0,24,154,150,20,0);
    C(L"BUTTON",L"Choose font...",BS_PUSHBUTTON,184,150,150,30,IDC_UI_FONT);
    C(L"STATIC",L"",0,346,150,150,30,IDC_UI_FONTLBL);
    C(L"BUTTON",L"Animate panel and splash",BS_AUTOCHECKBOX,24,198,320,24,IDC_UI_ANIM);
    C(L"STATIC",L"Changes apply immediately and are saved to config.json.",0,24,226,400,20,0);
    C(L"BUTTON",L"Close",BS_DEFPUSHBUTTON,410,256,86,30,IDC_UI_CLOSE);
    break; }
  case WM_ERASEBKGND:{ RECT rc; GetClientRect(hWnd,&rc); HBRUSH b=CreateSolidBrush(UiColors().bg); FillRect((HDC)wParam,&rc,b); DeleteObject(b); return 1; }
  case WM_CTLCOLORSTATIC:{ HDC dc=(HDC)wParam; UiPalette p=UiColors(); SetTextColor(dc,p.text); SetBkColor(dc,p.bg); if(!g_brSettings)g_brSettings=CreateSolidBrush(p.bg); return (LRESULT)g_brSettings; }
  case WM_CTLCOLORLISTBOX:{ HDC dc=(HDC)wParam; UiPalette p=UiColors(); SetTextColor(dc,p.text); SetBkColor(dc,p.surface); if(!g_brSurface)g_brSurface=CreateSolidBrush(p.surface); return (LRESULT)g_brSurface; }
  case WM_COMMAND:{ int id=LOWORD(wParam); int code=HIWORD(wParam);
    if(id==IDC_UI_STYLE&&code==CBN_SELCHANGE){ int s=(int)SendMessageW(GetDlgItem(hWnd,IDC_UI_STYLE),CB_GETCURSEL,0,0); g_cfg.ui_style=(s==0)?L"classic":L"modern"; SaveConfig(); return 0; }
    if(id==IDC_UI_THEME&&code==CBN_SELCHANGE){ int s=(int)SendMessageW(GetDlgItem(hWnd,IDC_UI_THEME),CB_GETCURSEL,0,0); g_cfg.ui_theme=(s==0)?L"light":(s==1?L"dark":(s==2?L"system":L"custom")); SaveConfig(); UiRefreshSettings(hWnd); return 0; }
    if(id==IDC_UI_BG&&code==BN_CLICKED){ COLORREF cust[16]={0}; CHOOSECOLORW cc={0}; cc.lStructSize=sizeof(cc); cc.hwndOwner=hWnd; cc.rgbResult=uiHex(g_cfg.ui_bg,RGB(248,247,244)); cc.Flags=CC_FULLOPEN|CC_RGBINIT; cc.lpCustColors=cust; if(ChooseColorW(&cc)){ g_cfg.ui_bg=uiHexStr(cc.rgbResult); SaveConfig(); UiRefreshSettings(hWnd); } return 0; }
    if(id==IDC_UI_ACCENT&&code==BN_CLICKED){ COLORREF cust[16]={0}; CHOOSECOLORW cc={0}; cc.lStructSize=sizeof(cc); cc.hwndOwner=hWnd; cc.rgbResult=uiHex(g_cfg.ui_accent,RGB(127,156,144)); cc.Flags=CC_FULLOPEN|CC_RGBINIT; cc.lpCustColors=cust; if(ChooseColorW(&cc)){ g_cfg.ui_accent=uiHexStr(cc.rgbResult); SaveConfig(); UiRefreshSettings(hWnd); } return 0; }
    if(id==IDC_UI_FONT&&code==BN_CLICKED){ LOGFONTW lf={0}; HDC dc=GetDC(NULL); lf.lfHeight=-MulDiv(g_cfg.ui_font_size,GetDeviceCaps(dc,LOGPIXELSY),72); ReleaseDC(NULL,dc); lf.lfWeight=FW_NORMAL; wcsncpy_s(lf.lfFaceName,LF_FACESIZE,g_cfg.ui_font.c_str(),_TRUNCATE); CHOOSEFONTW cf={0}; cf.lStructSize=sizeof(cf); cf.hwndOwner=hWnd; cf.lpLogFont=&lf; cf.Flags=CF_SCREENFONTS|CF_INITTOLOGFONTSTRUCT|CF_NOSTYLESEL|CF_NOSCRIPTSEL|CF_LIMITSIZE; cf.nSizeMin=7; cf.nSizeMax=48; if(ChooseFontW(&cf)){ g_cfg.ui_font=lf.lfFaceName; g_cfg.ui_font_size=cf.iPointSize/10; if(g_cfg.ui_font_size<7)g_cfg.ui_font_size=7; SaveConfig(); UiApplyCtlFont(hWnd); UiRefreshSettings(hWnd); } return 0; }
    if(id==IDC_UI_ANIM&&code==BN_CLICKED){ g_cfg.ui_anim=SendDlgItemMessageW(hWnd,IDC_UI_ANIM,BM_GETCHECK,0,0)==BST_CHECKED; SaveConfig(); return 0; }
    if(id==IDC_UI_CLOSE){ DestroyWindow(hWnd); return 0; }
    break; }
  case WM_CLOSE: DestroyWindow(hWnd); return 0;
  case WM_DESTROY: g_hSettings=NULL; if(g_brSettings){DeleteObject(g_brSettings);g_brSettings=NULL;} if(g_brSurface){DeleteObject(g_brSurface);g_brSurface=NULL;} return 0; }
  return DefWindowProcW(hWnd,msg,wParam,lParam); }
static void UiShowSettings(){ if(g_hSettings){ ShowWindow(g_hSettings,SW_SHOW); SetForegroundWindow(g_hSettings); return; } RECT r={0,0,520,310}; AdjustWindowRect(&r,WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,FALSE); int w=r.right-r.left,h=r.bottom-r.top; int x=(GetSystemMetrics(SM_CXSCREEN)-w)/2,y=(GetSystemMetrics(SM_CYSCREEN)-h)/2; g_hSettings=CreateWindowExW(0,L"LidaSettings",L"Lida Settings",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,x,y,w,h,g_hWnd,NULL,GetModuleHandleW(NULL),NULL); if(!g_hSettings)return; UiApplyCtlFont(g_hSettings); UiRefreshSettings(g_hSettings); ShowWindow(g_hSettings,SW_SHOW); SetForegroundWindow(g_hSettings); }
/* ======================================================================================== */
static LRESULT CALLBACK InputWndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam){
    switch(msg){
    case WM_CREATE:{
        HFONT hf=(HFONT)GetStockObject(DEFAULT_GUI_FONT);
        HINSTANCE hInst = (HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE);
        HWND hSt = CreateWindowExW(0, L"STATIC", L"Prompt", WS_CHILD|WS_VISIBLE,10,10,340,40,hWnd,(HMENU)1001,hInst,NULL);
        if(hSt) SetWindowTextW(hSt, g_inputPrompt.c_str()); if(hSt) SendMessage(hSt,WM_SETFONT,(WPARAM)hf,TRUE);
        g_hInputEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", L"", WS_CHILD|WS_VISIBLE|WS_TABSTOP|ES_AUTOHSCROLL,10,55,340,24,hWnd,(HMENU)ID_INPUT_EDIT,hInst,NULL);
        SendMessage(g_hInputEdit,WM_SETFONT,(WPARAM)hf,TRUE);
        HWND hOK = CreateWindowExW(0, L"BUTTON", L"OK", WS_CHILD|WS_VISIBLE|WS_TABSTOP|BS_DEFPUSHBUTTON,190,90,75,24,hWnd,(HMENU)ID_INPUT_OK,hInst,NULL);
        SendMessage(hOK,WM_SETFONT,(WPARAM)hf,TRUE);
        HWND hCancel = CreateWindowExW(0, L"BUTTON", L"Cancel", WS_CHILD|WS_VISIBLE|WS_TABSTOP,275,90,75,24,hWnd,(HMENU)ID_INPUT_CANCEL,hInst,NULL);
        SendMessage(hCancel,WM_SETFONT,(WPARAM)hf,TRUE);
        SetFocus(g_hInputEdit);
        break;
    }
    case WM_COMMAND:
        if(LOWORD(wParam)==ID_INPUT_OK){
            wchar_t buf[512]={0};
            GetWindowTextW(g_hInputEdit,buf,511);
            g_inputResult = buf;
            g_inputDone = true;
            DestroyWindow(hWnd);
            return 0;
        }
        if(LOWORD(wParam)==ID_INPUT_CANCEL){
            g_inputResult = L"";
            g_inputDone = true;
            DestroyWindow(hWnd);
            return 0;
        }
        break;
    case WM_CLOSE:
        g_inputResult = L"";
        g_inputDone = true;
        DestroyWindow(hWnd);
        return 0;
    default:
        return DefWindowProcW(hWnd,msg,wParam,lParam);
    }
    return 0;
}
static wstring InputBox(const wchar_t *title,const wchar_t *prompt){
    g_inputResult = L"";
    g_inputDone = false;
    g_inputTitle = title;
    g_inputPrompt = prompt;
    HINSTANCE hInst = GetModuleHandleW(NULL);
    WNDCLASSEXW wc={0};
    wc.cbSize=sizeof(WNDCLASSEXW);
    wc.lpfnWndProc=InputWndProc;
    wc.hInstance=hInst;
    wc.lpszClassName=L"LidaInputDlg";
    RegisterClassExW(&wc);
    g_hInputDlg = CreateWindowExW(WS_EX_DLGMODALFRAME|WS_EX_TOPMOST, L"LidaInputDlg", title, WS_POPUP|WS_CAPTION|WS_SYSMENU,
        (GetSystemMetrics(SM_CXSCREEN)-370)/2, (GetSystemMetrics(SM_CYSCREEN)-150)/2, 370, 150, g_hWnd, NULL, hInst, NULL);
    if(!g_hInputDlg) return L"";
    EnableWindow(g_hWnd,FALSE);
    ShowWindow(g_hInputDlg,SW_SHOW);
    UpdateWindow(g_hInputDlg);
    SetForegroundWindow(g_hInputDlg);
    MSG msg;
    while(GetMessageW(&msg,NULL,0,0)){
        if(msg.message==WM_QUIT) break;
        if(!IsDialogMessage(g_hInputDlg,&msg)){
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        if(g_inputDone) break;
    }
    EnableWindow(g_hWnd,TRUE);
    SetForegroundWindow(g_hWnd);
    return g_inputResult;
}
static void AddTrayIcon(){ ZeroMemory(&g_nid,sizeof(g_nid)); g_nid.cbSize = sizeof(NOTIFYICONDATAW); g_nid.hWnd=g_hWnd; g_nid.uID=g_uID; g_nid.uFlags=NIF_ICON|NIF_MESSAGE|NIF_TIP; g_nid.uCallbackMessage=WM_TRAYICON; HINSTANCE hInst = GetModuleHandleW(NULL); g_nid.hIcon = LoadIcon(hInst, MAKEINTRESOURCE(101)); if(!g_nid.hIcon) g_nid.hIcon = LoadIcon(NULL, IDI_APPLICATION); wcscpy_s(g_nid.szTip, L"Lida"); Shell_NotifyIconW(NIM_ADD,&g_nid); }
static void RemoveTrayIcon(){ Shell_NotifyIconW(NIM_DELETE,&g_nid); }
static void ShowTrayMenu(){ POINT pt; GetCursorPos(&pt); if(g_cfg.ui_style==L"modern"){ UiShowPanel(); return; } HMENU hMenu=CreatePopupMenu(); HMENU hLight=CreatePopupMenu(); HMENU hDark=CreatePopupMenu(); AppendMenuW(hLight,MF_STRING,ID_TRAY_SET_LIGHT_WALL,L"Set Wallpaper..."); AppendMenuW(hLight,MF_STRING,ID_TRAY_LIGHT_CURSOR_SCHEME,L"Set Cursor Scheme by Name..."); AppendMenuW(hDark,MF_STRING,ID_TRAY_SET_DARK_WALL,L"Set Wallpaper..."); AppendMenuW(hDark,MF_STRING,ID_TRAY_DARK_CURSOR_SCHEME,L"Set Cursor Scheme by Name..."); AppendMenuW(hMenu,MF_POPUP,(UINT_PTR)hLight,L"Light Mode"); AppendMenuW(hMenu,MF_POPUP,(UINT_PTR)hDark,L"Dark Mode"); AppendMenuW(hMenu,MF_SEPARATOR,0,NULL); AppendMenuW(hMenu,MF_STRING|(g_cfg.auto_switch?MF_CHECKED:MF_UNCHECKED),ID_TRAY_AUTO_SWITCH,L"Auto Switch on Theme Change"); AppendMenuW(hMenu,MF_STRING,ID_TRAY_APPLY,L"Apply Current Theme Now"); AppendMenuW(hMenu,MF_STRING,ID_TRAY_RESTORE_CURSORS,L"Restore Default Cursors"); AppendMenuW(hMenu,MF_SEPARATOR,0,NULL); AppendMenuW(hMenu,MF_STRING|(LockScreenActive()?MF_CHECKED:MF_UNCHECKED),ID_TRAY_LOCKSCREEN,L"Lock Screen Follows Theme"); AppendMenuW(hMenu,MF_STRING|(IsRunAtStartup()?MF_CHECKED:MF_UNCHECKED),ID_TRAY_STARTUP,L"Start with Windows"); if(!IsElevated())AppendMenuW(hMenu,MF_STRING,ID_TRAY_ELEVATE,L"Restart as Administrator..."); AppendMenuW(hMenu,MF_SEPARATOR,0,NULL); AppendMenuW(hMenu,MF_STRING,ID_TRAY_SETTINGS,L"UI Settings..."); AppendMenuW(hMenu,MF_SEPARATOR,0,NULL); AppendMenuW(hMenu,MF_STRING,ID_TRAY_RELOAD,L"Reload Config"); AppendMenuW(hMenu,MF_STRING,ID_TRAY_OPENCFG,L"Open Config Folder"); AppendMenuW(hMenu,MF_SEPARATOR,0,NULL); AppendMenuW(hMenu,MF_STRING,ID_TRAY_EXIT,L"Exit"); SetForegroundWindow(g_hWnd); TrackPopupMenu(hMenu,TPM_RIGHTBUTTON,pt.x,pt.y,0,g_hWnd,NULL); DestroyMenu(hMenu); }
static void OpenConfigFolder(){ wstring d=fs::path(g_configPath).parent_path().wstring(); if(d.empty())d=g_exeDir; ShellExecuteW(NULL,L"open",d.c_str(),NULL,NULL,SW_SHOWDEFAULT); }
LRESULT CALLBACK WndProc(HWND hWnd,UINT msg,WPARAM wParam,LPARAM lParam){ switch(msg){ case WM_TRAYICON: if(lParam==WM_RBUTTONUP||lParam==WM_LBUTTONUP)ShowTrayMenu(); break; case WM_COMMAND: switch(LOWORD(wParam)){ case ID_TRAY_EXIT:g_running=false;PostQuitMessage(0);break; case ID_TRAY_RELOAD:LoadConfig();MessageBoxW(g_hWnd,L"Config reloaded",L"Lida",MB_OK|MB_ICONINFORMATION);break; case ID_TRAY_APPLY:ApplyForMode(IsLightMode());break; case ID_TRAY_OPENCFG:OpenConfigFolder();break; case ID_TRAY_SETTINGS:UiShowSettings();break; case ID_TRAY_AUTO_SWITCH:g_cfg.auto_switch=!g_cfg.auto_switch;SaveConfig();break; case ID_TRAY_RESTORE_CURSORS:RestoreSystemCursors();MessageBoxW(g_hWnd,L"Restored default system cursors",L"Lida",MB_OK|MB_ICONINFORMATION);break; case ID_TRAY_LOCKSCREEN:{ bool active=LockScreenActive(); bool want=!active; g_cfg.lock_screen=want; SaveConfig(); if(!IsElevated()){ const wchar_t *msg=want?L"Turning the lock screen sync on needs administrator rights.\n\nRestart Lida as administrator now? (Your choice is saved.)":L"Turning the lock screen sync off needs administrator rights, so Lida can restore your own lock screen image.\n\nRestart Lida as administrator now? (Your choice is saved.)"; if(MessageBoxW(g_hWnd,msg,L"Lida",MB_YESNO|MB_ICONQUESTION)==IDYES)RelaunchElevated(); } else if(want){ wstring wp=IsLightMode()?g_cfg.light.wallpaper:g_cfg.dark.wallpaper; if(!ApplyLockScreen(wp))MessageBoxW(g_hWnd,L"Could not set the lock screen image. Check that the wallpaper file still exists.",L"Lida",MB_OK|MB_ICONWARNING); } else if(!ClearLockScreen())MessageBoxW(g_hWnd,L"Could not restore your own lock screen image.",L"Lida",MB_OK|MB_ICONWARNING); break; } case ID_TRAY_STARTUP:g_cfg.start_with_windows=!IsRunAtStartup();SetRunAtStartup(g_cfg.start_with_windows);SaveConfig();break; case ID_TRAY_ELEVATE:{ HINSTANCE r=ShellExecuteW(NULL,L"runas",ExePath().c_str(),NULL,g_exeDir.c_str(),SW_SHOWNORMAL); if((INT_PTR)r>32){g_running=false;PostQuitMessage(0);} break; } case ID_TRAY_SET_LIGHT_WALL:{wstring f=BrowseForFile(L"Image Files\0*.jpg;*.jpeg;*.png;*.bmp;*.gif\0All Files\0*.*\0",L"Select Light Mode Wallpaper"); if(!f.empty()){fs::path base(g_exeDir);error_code ec;fs::path rel=fs::relative(f,base,ec);if(!ec&&rel.wstring().find(L"..")==wstring::npos)g_cfg.light.wallpaper=rel.wstring();else g_cfg.light.wallpaper=f;SaveConfig();if(IsLightMode())ApplyWallpaper(g_cfg.light.wallpaper);}break;} case ID_TRAY_SET_DARK_WALL:{wstring f=BrowseForFile(L"Image Files\0*.jpg;*.jpeg;*.png;*.bmp;*.gif\0All Files\0*.*\0",L"Select Dark Mode Wallpaper"); if(!f.empty()){fs::path base(g_exeDir);error_code ec;fs::path rel=fs::relative(f,base,ec);if(!ec&&rel.wstring().find(L"..")==wstring::npos)g_cfg.dark.wallpaper=rel.wstring();else g_cfg.dark.wallpaper=f;SaveConfig();if(!IsLightMode())ApplyWallpaper(g_cfg.dark.wallpaper);}break;} case ID_TRAY_LIGHT_CURSOR_SCHEME:{wstring name=InputBox(L"Light Mode Cursor Scheme",L"Enter exact cursor scheme name (as in Mouse Properties > Pointers):"); if(!name.empty()){g_cfg.light.cursorScheme=name;SaveConfig();if(IsLightMode())ApplyCursorScheme(g_cfg.light.cursorScheme);}break;} case ID_TRAY_DARK_CURSOR_SCHEME:{wstring name=InputBox(L"Dark Mode Cursor Scheme",L"Enter exact cursor scheme name (as in Mouse Properties > Pointers):"); if(!name.empty()){g_cfg.dark.cursorScheme=name;SaveConfig();if(!IsLightMode())ApplyCursorScheme(g_cfg.dark.cursorScheme);}break;} }break; case WM_DESTROY:RemoveTrayIcon();PostQuitMessage(0);break; default:return DefWindowProcW(hWnd,msg,wParam,lParam);} return 0; }
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevInstance,LPSTR lpCmdLine,int nCmdShow){ WCHAR path[MAX_PATH]; GetModuleFileNameW(NULL,path,MAX_PATH); g_exeDir=fs::path(path).parent_path().wstring(); wstring portable=(fs::path(g_exeDir)/L"config.json").wstring(); g_configPath=(fs::path(ConfigDir())/L"config.json").wstring(); if(g_configPath!=portable){ std::error_code ec; if(!fs::exists(g_configPath)&&fs::exists(portable))fs::copy_file(portable,g_configPath,fs::copy_options::overwrite_existing,ec); } LoadConfig(); SetRunAtStartup(g_cfg.start_with_windows); WNDCLASSEXW wc={0}; wc.cbSize=sizeof(WNDCLASSEXW); wc.lpfnWndProc=WndProc; wc.hInstance=hInstance; wc.lpszClassName=L"LidaClass"; wc.hIcon=(HICON)LoadImageW(hInstance,MAKEINTRESOURCEW(101),IMAGE_ICON,0,0,LR_DEFAULTSIZE|LR_SHARED); wc.hIconSm=wc.hIcon; RegisterClassExW(&wc); WNDCLASSEXW pc={0}; pc.cbSize=sizeof(WNDCLASSEXW); pc.lpfnWndProc=UiPanelProc; pc.hInstance=hInstance; pc.lpszClassName=L"LidaPanel"; pc.hIcon=wc.hIcon; RegisterClassExW(&pc); WNDCLASSEXW spc={0}; spc.cbSize=sizeof(WNDCLASSEXW); spc.lpfnWndProc=UiSplashProc; spc.hInstance=hInstance; spc.lpszClassName=L"LidaSplash"; spc.hIcon=wc.hIcon; RegisterClassExW(&spc); WNDCLASSEXW stc={0}; stc.cbSize=sizeof(WNDCLASSEXW); stc.lpfnWndProc=UiSettingsProc; stc.hInstance=hInstance; stc.lpszClassName=L"LidaSettings"; stc.hIcon=wc.hIcon; stc.hIconSm=wc.hIcon; RegisterClassExW(&stc); g_hWnd=CreateWindowExW(0,L"LidaClass",L"Lida",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,0,0,NULL,NULL,hInstance,NULL); if(wc.hIcon){ SendMessageW(g_hWnd,WM_SETICON,ICON_BIG,(LPARAM)wc.hIcon); SendMessageW(g_hWnd,WM_SETICON,ICON_SMALL,(LPARAM)wc.hIconSm); } AddTrayIcon(); UiShowSplash(); g_lastIsLight=IsLightMode(); ApplyForMode(g_lastIsLight); if(!g_cfg.lock_screen&&IsElevated()&&LockScreenActive())ClearLockScreen(); MSG msg; auto pump=[&](){ while(PeekMessageW(&msg,NULL,0,0,PM_REMOVE)){ if(msg.message==WM_QUIT){g_running=false;return;} TranslateMessage(&msg); DispatchMessageW(&msg); } }; while(g_running){ pump(); if(!g_running)break; if(g_cfg.auto_switch){ bool cur=IsLightMode(); if(cur!=g_lastIsLight){g_lastIsLight=cur;ApplyForMode(cur);} } for(int waited=0;waited<g_cfg.check_interval_ms&&g_running;waited+=10){ pump(); if(!g_running)break; std::this_thread::sleep_for(std::chrono::milliseconds(10)); } } RemoveTrayIcon(); return 0; }
