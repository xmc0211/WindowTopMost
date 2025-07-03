# WindowTopMost

A library that allows windows to be placed at the top in front of any window.

The earliest supported version of Windows: **Windows 8.x**

- - -
### References
[https://github.com/killtimer0/uiaccess](https://github.com/killtimer0/uiaccess)

[https://gist.github.com/ADeltaX/80d220579400a95c336af0eec372ecb0](https://gist.github.com/ADeltaX/80d220579400a95c336af0eec372ecb0)

[https://blog.adeltax.com/window-z-order-in-windows-10](https://blog.adeltax.com/window-z-order-in-windows-10)

https://www.bilibili.com/video/BV1HCwwegEVp (website in China)

https://www.bilibili.com/opus/1023826882327478276 (website in China)
- - -
## Defining Window Z-order “band”

![Windows z-order](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/Windows%20z-order.png?raw=true)

For clarity, in this context the word “band” means group of Z-orders.

**Until Windows 8, there was only one band, the ZBID_DESKTOP band**, which is the default one when you write an application that creates a new window and when it gains focus it will go to the highest Z-order meaning it will go on top of other windows. This is true unless there are **topmost windows**. As the name says, **it will stay on top of other windows**. What if there are 2 topmost windows? Well, in this case, **the last window to gain focus will stay on top of other one**. In the end there are 2 groups of Z-order in the ZBID_DESKTOP band, normal and topmost. These will never be “touched” by each other, **topmost window will always stay on top of other normal window**, no matter what.

**Starting from Windows 8, Microsoft "introduced" other bands**, and all of them are higher Z-orders than the desktop band. For example the start menu is on ZBID_IMMERSIVE_MOGO. Task manager selected "always on top" is on ZBID_SYSTEM_TOOLS. **These are always on top of any window in the ZBID_DESKTOP band**, that is, any normal or topmost window created by third parts developers will no longer cover start menu and stuff. **These bands cannot mix each other**, meaning, for example, that ZBID_DESKTOP will never be on top of ZBID_IMMERSIVE_MOGO, that is, **bands have their orders too**.

Like ZBID_DESKTOP, other bands also can be subdivided in 2 groups: **normal and topmost window**.

The order of bands are the following, from the lowest to the highest Z-order:

| ZBID Name | Value | Description |
| :-----: | :-----: | :----- |
| ZBID_DEFAULT | 0 | Default Z segment（ZBID_DESKTOP）|
| ZBID_DESKTOP | 1 | Normal window|
| ZBID_IMMERSIVE_RESTRICTED | 14 | \[Generally not used\]|
| ZBID_IMMERSIVE_BACKGROUND | 12 |  |
| ZBID_IMMERSIVE_INACTIVEDOCK | 9 | \[Generally not used\]|
| ZBID_IMMERSIVE_INACTIVEMOBODY | 8 | \[Generally not used\]|
| ZBID_IMMERSIVE_ACTIVEDOCK | 11 | \[Generally not used\]|
| ZBID_IMMERSIVE_ACTIVEMOBODY | 10 | \[Generally not used\]|
| ZBID_IMMERSIVE_APPCHROME | 5 | Task View (Win+Tab menu)|
| ZBID_IMMERSIVE_MOGO | 6 | Start menu, taskbar|
| ZBID_IMMERSIVE_SEARCH | 13 | Cortana、Windows search|
| ZBID_IMMERSIVE_NOTIFICATION | 4 | Operation Center (Notification Center)|
| ZBID_IMMERSIVE_EDGY | 7 |  |
| ZBID_SYSTEM_TOOLS | 15 | Top task manager, Alt+Tab menu|
| ZBID_LOCK (>= Win10) | 16 | Lock screen interface|
| ZBID_ABOVELOCK_UX (>= Win10) | 17 | real-time playback window|
| ZBID_IMMERSIVE_IHM | 3 |  |
| ZBID_GENUINE_WINDOWS | 14 | 'Activate Windows' window|
| ZBID_UIACCESS | 2 | Screen keyboard, magnifying glass|

## Visualizing Window bands

### ZBID_DESKTOP

![ZBID_DESKTOP](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/ZBID_DESKTOP.png?raw=true)

ZBID_DESKTOP: this band is where all our window stays on. Let’s suppose you have 2 window here, Paint and Photos. Both are desktop windows, and they can overlap each other by just focusing a window or another. Now suppose that Paint window is topmost, now it will stay on top of Photos, regardless of its focus state.

### ZBID_IMMERSIVE_APPCHROME

![ZBID_IMMERSIVE_APPCHROME](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/ZBID_IMMERSIVE_APPCHROME.png?raw=true)

ZBID_IMMERSIVE_APPCHROME: this band is used by the task view (Win+Tab menu).

### ZBID_IMMERSIVE_MOGO

![ZBID_IMMERSIVE_MOGO](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/ZBID_IMMERSIVE_MOGO.png?raw=true)

ZBID_IMMERSIVE_MOGO: this band is used by the start menu AND the taskbar (only if the start menu is open).

### ZBID_IMMERSIVE_SEARCH

![ZBID_IMMERSIVE_SEARCH](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/ZBID_IMMERSIVE_SEARCH.png?raw=true)

ZBID_IMMERSIVE_SEARCH: this band is used by the search menu and the cortana.

### ZBID_IMMERSIVE_NOTIFICATIONS

![ZBID_IMMERSIVE_NOTIFICATIONS](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/ZBID_IMMERSIVE_NOTIFICATIONS.png?raw=true)

ZBID_IMMERSIVE_NOTIFICATIONS: used by the Action Center, notifications and some system flyouts (e.g. Network, Volume).

### ZBID_SYSTEM_TOOLS

![ZBID_SYSTEM_TOOLS](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/ZBID_SYSTEM_TOOLS.png?raw=true)

ZBID_SYSTEM_TOOLS: used by Task Manager with "Always on Top" enabled, Alt-Tab view.

### ZBID_ABOVELOCK_UX

![ZBID_ABOVELOCK_UX](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/ZBID_ABOVELOCK_UX.png?raw=true)

ZBID_ABOVELOCK_UX: used by "Playing Now". This band and anything else higher will stay on top of the lock screen.

### ZBID_GENUINE_WINDOWS

![ZBID_GENUINE_WINDOWS](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/ZBID_GENUINE_WINDOWS.png?raw=true)

ZBID_GENUINE_WINDOWS: used by "Activate Windows" window. This band is the highest priority band outside of ZBID_UIACCESS.

### ZBID_UIACCESS

![ZBID_UIACCESS](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/ZBID_UIACCESS.png?raw=true)

ZBID_UIACCESS: used by magnifier and on-screen keyboard. This band is typically used for assistive technology applications (UIAccess).

- - -

## Technical Parts

### ZBID enum

```cpp
enum ZBID : DWORD {
    ZBID_DEFAULT = 0,
    ZBID_DESKTOP = 1,
    ZBID_UIACCESS = 2,
    ZBID_IMMERSIVE_IHM = 3,
    ZBID_IMMERSIVE_NOTIFICATION = 4,
    ZBID_IMMERSIVE_APPCHROME = 5,
    ZBID_IMMERSIVE_MOGO = 6,
    ZBID_IMMERSIVE_EDGY = 7,
    ZBID_IMMERSIVE_INACTIVEMOBODY = 8,
    ZBID_IMMERSIVE_INACTIVEDOCK = 9,
    ZBID_IMMERSIVE_ACTIVEMOBODY = 10,
    ZBID_IMMERSIVE_ACTIVEDOCK = 11,
    ZBID_IMMERSIVE_BACKGROUND = 12,
    ZBID_IMMERSIVE_SEARCH = 13,
    ZBID_GENUINE_WINDOWS = 14,
    ZBID_IMMERSIVE_RESTRICTED = 15,
    ZBID_SYSTEM_TOOLS = 16,
    ZBID_LOCK = 17, // Win 10 up
    ZBID_ABOVELOCK_UX = 18, // Win 10 up
};
```

- - -

### Some private api function introduction

#### CreateWindowInBand
Private api function found in user32.dll. Since it’s not present in Windows SDK headers/libs, you have to use ```GetModuleHandle``` and ```GetProcAddress``` to get access to that function.

```CreateWindowInBand``` function is the same as ```CreateWindowEx``` except it has 1 more parameter, ```dwBand```, that is where you specify the band on which the window should stay (ZBID).

```cpp
HWND WINAPI CreateWindowInBand(
     DWORD dwExStyle,
     LPCWSTR lpClassName,
     LPCWSTR lpWindowName,
     DWORD dwStyle,
     int x,
     int y,
     int nWidth,
     int nHeight,
     HWND hWndParent,
     HMENU hMenu,
     HINSTANCE hInstance,
     LPVOID lpParam,
     DWORD dwBand
);
```

#### CreateWindowInBandEx
Private api function found in user32.dll.

Same as ```CreateWindowInBand``` plus 1 parameter, ```dwTypeFlags``` (Unknown purpose).

```cpp
HWND WINAPI CreateWindowInBandEx(
     DWORD dwExStyle,
     LPCWSTR lpClassName,
     LPCWSTR lpWindowName,
     DWORD dwStyle,
     int x,
     int y,
     int nWidth,
     int nHeight,
     HWND hWndParent,
     HMENU hMenu,
     HINSTANCE hInstance,
     LPVOID lpParam,
     DWORD dwBand,
     DWORD dwTypeFlags
);
```

#### SetWindowBand
Private api function found in user32.dll.

```SetWindowBand``` function has 3 parameters, ```hWnd```, ```hWndInserAfter``` and ```dwBand```. Same as 2 first parameters from ```SetWindowPos```. The returned value indicates success or failure. In case of failure call ```GetLastError```. Most likely, you will got a nice ```0x5``` (```ACCESS_DENIED```).

```cpp
BOOL WINAPI SetWindowBand(
     HWND hWnd, 
     HWND hwndInsertAfter, 
     DWORD dwBand
);
```

#### GetWindowBand
Private api function found in user32.dll.

```GetWindowBand``` function has 2 parameters, ```hWnd``` and ```pdwBand```. ```pdwBand``` is a pointer that receives the band (ZBID) of the HWND. The returned value indicates success or failure. In case of failure call ```GetLastError```.

```cpp
BOOL WINAPI GetWindowBand(
     HWND hWnd, 
     PDWORD pdwBand
);
```

#### NtUserEnableIAMAccess
Private api function found in user32.dll.

```NtUserEnableIAMAccess``` function has 2 parameters, ```key``` and ```enable```. ```enable``` is a flag that specifies whether IAM access is enabled or not. ```key``` is the IAM access key. In case of failure call ```GetLastError```.

```cpp
BOOL WINAPI NtUserEnableIAMAccess(
    ULONG64 key,
    BOOL enable
);
```

#### NtUserAcquireIAMKey
Private api function found in user32.dll.

```NtUserAcquireIAMKey``` function has only 1 parameter, ```pkey```. ```pkey``` is a pointer to ULONG64 used to receive the created IAM access key.

```cpp
BOOL WINAPI NtUserAcquireIAMKey(
    OUT ULONG64* pkey
);
```

- - -

### Can I call these APIs?

Short reply: Some yes (under certain conditions), some no.

```GetWindowBand``` works in every case, you can use it without any problem.

```CreateWindowInBand(Ex)``` **works ONLY if you pass ```ZBID_DEFAULT``` or ```ZBID_DESKTOP```** as ```dwBand``` argument. Also **```ZBID_UIACCESS``` is permitted only if the process has UIAccess token** (below is a detailed explanation). **Any other ZBID will fail with ```0x5``` (```ACCESS DENIED```)**.

```SetWindowBand``` will **never** work, it will **always fail with ```0x5``` (```ACCESS DENIED```)**.

```NtUserEnableIAMAccess``` also hardly works because you need to provide a **ULONG64 type access key**.

```NtUserAcquireIAMKey``` never works **when Explorer.EXE is running**.

#### Why I can’t call using other ZBIDs?
Because Microsoft added extra checks for these bands.

For ```CreateWindowInBand(Ex)```, to be able to use more ZBIDs, the program **must have** a special PE header, named **```.imrsiv```** (bss_seg), flagged with **```IMAGE_DLLCHARACTERISTICS_FORCE_INTEGRITY```** and be **signed with a Microsoft certificate ```Microsoft Windows```**.

For ```SetWindowBand```, we cannot directly call it, but there are other methods, which will be explained in detail below.

- - -

### Solution

The article mentions that calling SetWindowBand **requires calling NtUserEnableIAMAccess to enable IAM access** first. But this function needs a ULONG64 type access key. 

We can **inject a DLL** (IAMKeyHacker.DLL in this example) **into Explorer EXE**, And monitor the behavior of Explorer.EXE calling **```NtUserEnableIAMAccess```**. We can respond to ```NtUserEnabeIAMAccess``` **in our own DLL** through API hook, and **steal the IAM access key given by Explorer.EXE** calling function based on calling the original function and return the correct result.

How to make Explorer.EXE calls ```NtUserEnabeIAMAccess```? Just call ```SetForegroundWindow``` to set the focus to the desktop window first, and then set the focus to the taskbar. I don't know why?

I have tried using the registry to **pass IAM keys back to the program** that injected the DLL (hereinafter referred to as the "main program"). However, when the main program attempted to call ```NtUserEnableIAMAccess``` using the correct IAM key, it **obtained the result ```0x5``` (```ACCESS DENIED```)**. This indicates that it is **not advisable to call this function in ordinary applications**.

I also found that **if code is written in the DLL that steals IAM keys to call ```NtUserEnableIAMAccess``` and ```SetWindowBand``` to set window Z-order band**, there will be **no errors and the operation will be successfully completed**. Perhaps due to the Explorer.EXE needs to call ```SetWindowBand``` to set the Z-order band of other windows.

Finally, I used file transfer data (hWnd, hWndInsertAfter and dwBand) and a hidden window to trigger the message to set window Z-order band.

This is all the solutions.

- - -

## Final Effect

- Boot Menu

![Boot](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/Boot.png?raw=true)

- CreateWindowInBand

![CreateWindowInBand_Taskmgr](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/CreateWindowInBand_Taskmgr.png?raw=true)

![CreateWindowInBand_All](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/CreateWindowInBand_All.png?raw=true)

- GetWindowBand

![GetWindowBand_StartMenu](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/GetWindowBand_StartMenu.png?raw=true)

![GetWindowBand_Taskmgr](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/GetWindowBand_Taskmgr.png?raw=true)

- - -
