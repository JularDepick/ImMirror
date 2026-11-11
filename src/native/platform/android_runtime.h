#pragma once

struct android_app;

namespace immirror::game {
class GameShell;
}

namespace immirror::platform {

/* 当前运行时持有的装配层实例; 无运行时或已析构时返回空. */
game::GameShell* RuntimeHostShell();

/*
 * 应用装配根. 在胶水层的原生线程上运行:
 * 组装平台桥接, 渲染后端与主循环, 负责帧率统计与设备画像日志.
 * 该函数会阻塞到主循环退出, 返回前完成渲染资源回收.
 */
void RunApplication(android_app* app);

}  /* namespace immirror::platform */
