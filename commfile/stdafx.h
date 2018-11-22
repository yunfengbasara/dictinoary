// stdafx.h : 标准系统包含文件的包含文件，
// 或是经常使用但不常更改的
// 特定于项目的包含文件
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // 从 Windows 头中排除极少使用的资料

#include <Windows.h>
#include <tchar.h>
#include <direct.h>
#include <io.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <mutex>
#include <thread>
#include <list>
#include <vector>
#include <map>
#include <ctime>

#include <winsock2.h>
#include <mswsock.h>
#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "Mswsock.lib")
