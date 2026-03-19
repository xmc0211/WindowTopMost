# Some information about ```CreateWindowInBandEx``` 14th argument ```dwInternalFlag```

GitHub project homepage: https://github.com/xmc0211/WindowTopMost

Author: ```xmc0211```

The system version I used for analysis: ```Windows 11 25H2 Professional, Version 10.0.26200.7623```.

If you like this project, please give it a Star. Thank you all for your support!

- - -

Just to declare, the actual name of parameter ```dwInternalFlag``` **may not be accurate**. This is the name I defined for it, **representing only my opinions**.

- - -

## 1. What is function ```CreateWindowInBandEx```?

Provide the function pointer type definition for ```CreateWindowInBandEx```:

```cpp
typedef HWND (WINAPI* T_CreateWindowInBandEx) (
    DWORD dwExStyle,			// Same as 'CreateWindowExW'
    LPCWSTR lpClassName,
    LPCWSTR lpWindowName,
    DWORD dwStyle,
    int X,
    int Y,
    int nWidth,
    int nHeight,
    HWND hWndParent,
    HMENU hMenu,
    HINSTANCE hInstance,
    LPVOID lpParam,
    DWORD dwBand,               // Window Z-Order Band
    DWORD dwInternalFlag        // An internal window flag
);
```

Function ```CreateWindowInBandEx``` adds two parameters, **```dwBand``` and ```dwInternalFlag```**, based on the public function ```CreateWindowExW```. Among them, ```dwBand``` is the Z-Order Band of the window, indicating **which Z-Order Band the window to be created**; And ```dwInternalFlag``` is Microsoft's **internal identifier**, which is also the center of this article. At present, I have not found any information related to its meaning.

- - -

## Where was parameter ```dwInternalFlag``` passed to?

Function ```CreateWindowInBandEx``` is located in the exported function of ```User32.DLL```.

![user32_1.png](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/user32_1.png)

Parameter ```dwInternalFlag``` is passed internally in ```User32.DLL``` and ultimately reaches ***function ```ZwUserCreateWindowEx``` in ```Win32u.DLL```***. 

During this transmission process, ```User32.DLL``` **did not make any changes or checks to it**.

Function ```CreateWindowBandEx``` in ```User32.DLL```:

![user32_2.png](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/user32_2.png)

Code of ```ZwUserCreateWindowEx``` function calling ```Win32u.DLL``` in ```User32.DLL```:

![user32_3.png](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/user32_3.png)

- - -

When parameter ```dwInternalFlag``` reaches the interior of ```Win32u.DLL```, ```Win32u.DLL``` triggers **```syscall```**. 

From service number ```0x106F```, it can be seen that it is an **SSSDT call** (Service number >= 0x1000). 

Syscall trigger in ```Win32u.DLL```:

![win32u.png](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/win32u.png)

Function ```NtUserCreateWindowEx``` inside ```Win32kFull.SYS```:

![win32kfull_1.png](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/win32kfull_1.png)

This function is where ```dwInternalFlag``` is ultimately processed.

- - -

To summarize, from function ```CreateWindowBandEx``` in ```User32.DLL``` to function ```NtUserCreateWindowEx``` in ```Win32kFull.SYS```, the call stack is:

```
6. Win32kFull.SYS!xxxCreateWindowEx
5. Win32kFull.SYS!NtUserCreateWindowEx+0x670
------ syscall, service 0x106F ------
4. Win32u.DLL!ZwUserCreateWindowEx+0x12
3. User32.DLL!VerNtUserCreateWindowEx+0x22F
2. User32.DLL!CreateWindowInternal+0x1DE
1. User32.DLL!CreateWindowInBandEx+0x8B
```

- - -

## What did ```Win32Full.SYS``` do to parameter ```dwInternalFlag```?

Let's come to the implementation of the SSSDT function - ```NtUserCreateWindowEx``` inside ```Win32kFull.SYS```.

- - -

### No.1

![win32kfull_2.png](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/win32kfull_2.png)

Function ```NtUserCreateWindowEx``` performs two tests on parameter ```dwInternalFlag```:

- Parameter ```dwInternalFlag``` **only has the last three valid bits**, the other bits must be set to ```0```, otherwise ```ERROR_INVALID_PARAMETER``` will be reported. This indicates that ```dwInternalFlag``` can only take values from ```0x000 (0)``` to ```0x111 (7)```.

- **If ```hWndParent``` is equal to ```-3```, the first bit cannot be set**. I don't know what ```hWndParent``` equals to ```-3``` means.

![win32kfull_3.png](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/win32kfull_3.png)

Then function ```NtUserCreateWindowEx``` did not do anything to that parameter again. It passed it to function ```xxxCreateWindowEx```.

- - -

### No.2

![win32kfull_4.png](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/win32kfull_4.png)

Function ```xxxCreateWindowEx``` performs two tests on parameter ```dwInternalFlag```:

- If the application **is a desktop application**, the **first bit must not be set**, otherwise ```ERROR_ACCESS_DENIED``` will be reported.

- If the application **is not an 'Immersive Broker'**, the **second bit must not be set**, otherwise ```ERROR_ACCESS_DENIED``` will be reported too.

Inside functions ```IsDesktopApp``` and ```IsImmersiveBroker```:

![win32kbase_1.png](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/win32kbase_1.png)

![win32kbase_2.png](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/win32kbase_2.png)

Both functions are **verifying the identifiers** stored within the structure.

An article mentions wanting ```IsImmersiveBroker``` to return ```TRUE```，the program must have a special PE header, named **```.imrsiv```** (bss_seg) and be **signed with a Microsoft certificate ```Microsoft Windows```**. 

_This is basically consistent with the limitation of function ```SetWindowBand```, so ```SetWindowBand``` may **have also used function ```IsImmersiveBroker``` for verification**._

- - -

### No.3

![win32kfull_5.png](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/win32kfull_5.png)

If the third bit is set, certain specific **properties** of the window will **be changed**.

The good news is that I haven't found any restrictions on the third bit. So **you can try** setting this flag.

- - -

### No.4

![win32kfull_6.png](https://github.com/xmc0211/WindowTopMost/blob/main/Assets/win32kfull_6.png)

Setting the second bit will **modify the execution path of the function**, thereby achieving specific functions.

- - -

### Summarize

1. The first bit of ```dwInternalFlag``` is only applicable to **non-desktop applications**, and for windows whose parent window handle is not ```-3```, but it has no practical effect;

2. The second bit is only applicable to **```ImmersiveBroker``` applications**, which will **change the logic** of creating windows;

3. The third bit has no limit and can give the window **a special attribute**.

What surprised me was that the first bit of parameter ```dwInternalFlag``` had no effect. Perhaps IDA did not analyze any other code that uses this parameter.

- - -

At present, I can only analyze up to this point. Welcome all developers interested in it to continue exploring!

My email: ```xmc0211@qq.com``` (in China)
