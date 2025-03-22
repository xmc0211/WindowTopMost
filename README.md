# WindowTopMost
A library that allows windows to be placed at the top in front of any window (including on-screen keyboards under Win11)

- - -
### References
[https://github.com/killtimer0/uiaccess](https://github.com/killtimer0/uiaccess)

[https://gist.github.com/ADeltaX/80d220579400a95c336af0eec372ecb0](https://gist.github.com/ADeltaX/80d220579400a95c336af0eec372ecb0)

[https://blog.adeltax.com/window-z-order-in-windows-10](https://blog.adeltax.com/window-z-order-in-windows-10)

https://www.bilibili.com/video/BV1HCwwegEVp (Chinese)

https://www.bilibili.com/opus/1023826882327478276 (Chinese)
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
