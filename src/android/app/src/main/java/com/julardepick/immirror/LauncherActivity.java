package com.julardepick.immirror;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;

import com.julardepick.immirror.lobby.MainLobbyActivity;
import com.julardepick.immirror.account.LoginActivity;
import com.julardepick.immirror.account.SessionStore;

/*
 * 入口路由页.
 *
 * 启动时按本地会话判断去向: 已登录进大厅 (lobby.MainLobbyActivity),
 * 未登录进登录界面 (account.LoginActivity), 判断完立即结束自身,
 * 不再承载任何界面内容; 大厅承担账号资料, 世界列表与全部功能入口.
 * 旧入口页的世界列表对话框已升级为独立页 lobby.WorldListActivity.
 */
public final class LauncherActivity extends Activity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        /*
         * 只做路由: 已登录进大厅 (不再绕过大厅直进游戏),
         * 未登录进登录页; 登录成功后的目标也是大厅.
         */
        final Intent next = new Intent(this,
                SessionStore.hasUsableSession(this) ? MainLobbyActivity.class : LoginActivity.class);
        startActivity(next);
        finish();
    }
}
