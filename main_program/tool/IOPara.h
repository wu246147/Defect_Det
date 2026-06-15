#ifndef IOPARA_H
#define IOPARA_H

#include <map>
#include <string>

static std::map<std::string, int> IOPara
{
    //输出
    {"heardBeat", 0x100},   //心跳       0,1
    {"vehicleModelFeedback", 0x102},   //车型反馈   以数字形式反馈
    {"getReady", 0x104},   //准备好     1为准备好
    {"detFinish", 0x106},   //检测完成   1为检测完成
    {"detResult", 0x108},   //检测结果   1为ok，2为ng

    //输入
    {"check", 0x200},   //触发        1为触发信号
    {"vehicleMode", 0x202},   //车型        以数字形式反馈
    {"vehicleModeChange", 0x204},   //切换车型     1为切换信号

};

#endif // IOPARA_H
