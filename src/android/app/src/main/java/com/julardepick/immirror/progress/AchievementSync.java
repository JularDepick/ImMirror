package com.julardepick.immirror.progress;

import android.content.Context;

import com.julardepick.immirror.account.ApiClient;
import com.julardepick.immirror.account.ApiException;
import com.julardepick.immirror.account.Session;

import org.json.JSONObject;

/*
 * 成就与任务的服务端同步.
 *
 * 只做三件事: 取成就总览, 取每日任务, 领取奖励. 结果一律回到主线程,
 * 界面层不需要处理线程切换. 成功响应原文交给缓存层保存,
 * 失败时界面用缓存回落, 因此断网也能看到上一次的进度.
 */
public final class AchievementSync {

    /* 同步回调, 成功与失败都在主线程. */
    public interface Handler<T> {
        void onSuccess(T value);

        void onFailure(ApiException error);
    }

    private final ApiClient api;
    private final Context context;

    public AchievementSync(Context context) {
        this.context = context.getApplicationContext();
        this.api = new ApiClient(this.context);
    }

    public void loadAchievements(Session session, final Handler<AchievementBoard> handler) {
        api.fetchAchievements(session, new ApiClient.Callback<JSONObject>() {
            @Override
            public void onSuccess(JSONObject value) {
                AchievementCache.saveAchievements(context, String.valueOf(value));
                handler.onSuccess(AchievementBoard.parse(value));
            }

            @Override
            public void onFailure(ApiException error) {
                handler.onFailure(error);
            }
        });
    }

    public void loadDaily(Session session, final Handler<DailyBoard> handler) {
        api.fetchDailyTasks(session, new ApiClient.Callback<JSONObject>() {
            @Override
            public void onSuccess(JSONObject value) {
                AchievementCache.saveDaily(context, String.valueOf(value));
                handler.onSuccess(DailyBoard.parse(value));
            }

            @Override
            public void onFailure(ApiException error) {
                handler.onFailure(error);
            }
        });
    }

    public void claimAchievement(Session session, int achievementId, final Handler<JSONObject> handler) {
        api.claimAchievement(session, achievementId, new ApiClient.Callback<JSONObject>() {
            @Override
            public void onSuccess(JSONObject value) {
                handler.onSuccess(value);
            }

            @Override
            public void onFailure(ApiException error) {
                handler.onFailure(error);
            }
        });
    }

    /* 批量领取: 一次领完所有可领奖励, 回调里带回本次入账总额. */
    public void claimAll(Session session, final Handler<JSONObject> handler) {
        api.claimAllRewards(session, new ApiClient.Callback<JSONObject>() {
            @Override
            public void onSuccess(JSONObject value) {
                handler.onSuccess(value);
            }

            @Override
            public void onFailure(ApiException error) {
                handler.onFailure(error);
            }
        });
    }

    public void claimDaily(Session session, int slot, final Handler<JSONObject> handler) {
        api.claimDailyTask(session, slot, new ApiClient.Callback<JSONObject>() {
            @Override
            public void onSuccess(JSONObject value) {
                handler.onSuccess(value);
            }

            @Override
            public void onFailure(ApiException error) {
                handler.onFailure(error);
            }
        });
    }
}
