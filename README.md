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
For clarity, in this context the word “band” means group of Z-orders.

**Until Windows 8, there was only one band, the ZBID_DESKTOP band**, which is the default one when you write an application that creates a new window and when it gains focus it will go to the highest Z-order meaning it will go on top of other windows. This is true unless there are **topmost windows**. As the name says, **it will stay on top of other windows**. What if there are 2 topmost windows? Well, in this case, **the last window to gain focus will stay on top of other one**. In the end there are 2 groups of Z-order in the ZBID_DESKTOP band, normal and topmost. These will never be “touched” by each other, **topmost window will always stay on top of other normal window**, no matter what.

**Starting from Windows 8, Microsoft "introduced" other bands**, and all of them are higher Z-orders than the desktop band. For example the start menu is on ZBID_IMMERSIVE_MOGO. Task manager selected "always on top" is on ZBID_SYSTEM_TOOLS. **These are always on top of any window in the ZBID_DESKTOP band**, that is, any normal or topmost window created by third parts developers will no longer cover start menu and stuff. **These bands cannot mix each other**, meaning, for example, that ZBID_DESKTOP will never be on top of ZBID_IMMERSIVE_MOGO, that is, **bands have their orders too**.

Like ZBID_DESKTOP, other bands also can be subdivided in 2 groups: **normal and topmost window**.

The order of bands are the following, from the lowest to the highest Z-order (some ZBID orders are unknown for me ATM):

ZBID_DEFAULT 0 Default Z segment（ZBID_DESKTOP）
ZBID_DESKTOP 1 Normal window
ZBID_IMMERSIVE_RESTRICTED 14 ** Generally not used **
ZBID_IMMERSIVE_BACKGROUND 12
ZBID_IMMERSIVE_INACTIVEDOCK 9 ** Generally not used **
ZBID_IMMERSIVE_INACTIVEMOBODY 8 ** Generally not used **
ZBID_IMMERSIVE_ACTIVEDOCK 11 ** Generally not used **
ZBID_IMMERSIVE_ACTIVEMOBODY 10 ** Generally not used **
ZBID_IMMERSIVE_APPCHROME 5 Task View (Win+Tab menu)
ZBID_IMMERSIVE_MOGO 6 Start menu, taskbar
ZBID_IMMERSIVE_SEARCH 13 Cortana、Windows search
ZBID_IMMERSIVE_NOTIFICATION 4 Operation Center (Notification Center)
ZBID_IMMERSIVE_EDGY 7
ZBID_SYSTEM_TOOLS 15 Top task manager, Alt+Tab menu
ZBID_LOCK (>= Win10) 16 Lock screen interface
ZBID_ABOVELOCK_UX (>= Win10) 17 real-time playback window
ZBID_IMMERSIVE_IHM 3
ZBID_GENUINE_WINDOWS 14 'Activate Windows' window
ZBID_UIACCESS 2 Screen keyboard, magnifying glass
Visualizing Window bands
