package com.julardepick.immirror.wallet;

import org.json.JSONObject;

import java.util.ArrayList;
import java.util.LinkedHashSet;
import java.util.List;
import java.util.Set;

/*
 * 钱包页数据快照.
 *
 * 聚合 GET /api/v1/wallet (余额与流水) 与 GET /api/v1/wallet/checkin (签到状态)
 * 两份响应, 界面一次拿到全部字段后整体渲染, 不出现半块数据.
 * 服务端暂未下发逐日签到记录, signedDates 优先取签到状态里可能补充的
 * signedDates 数组, 缺失时用流水中 reason=checkin 的 ref (即签到日期) 反推;
 * 流水只保留最新二十条, 跨月的记录反推不到, 属于可接受的退化.
 */
public final class WalletBoard {

    private final long balance;
    private final long updatedAt;
    private final CheckinStatus checkin;
    private final List<LedgerEntry> ledger;

    /* 供同包 WalletApi 构造; 字段不可变, 外部只读. */
    WalletBoard(long balance, long updatedAt, CheckinStatus checkin, List<LedgerEntry> ledger) {
        this.balance = balance;
        this.updatedAt = updatedAt;
        this.checkin = checkin;
        this.ledger = ledger;
    }

    /* 空快照: 会话不可用或请求失败时的兜底渲染数据. */
    public static WalletBoard empty() {
        return new WalletBoard(0L, 0L, CheckinStatus.parse(null),
                new ArrayList<LedgerEntry>());
    }

    /*
     * 签到成功后的即时快照: 余额与月内次数取签到响应, 签到状态翻转为已签,
     * 今日奖励记录本次数值; 流水与逐日高亮等待随后的整页刷新补齐.
     */
    public static WalletBoard withCheckinResult(WalletBoard source, CheckinResult result) {
        if (source == null) {
            source = empty();
        }
        CheckinStatus previous = source.checkin;
        JSONObject raw = new JSONObject();
        try {
            raw.put("signedToday", true);
            raw.put("monthCount", result.monthCount());
            raw.put("month", previous.month());
            raw.put("todayReward", result.reward());
        } catch (org.json.JSONException ignored) {
            return source;
        }
        CheckinStatus updated = CheckinStatus.parse(raw);
        return new WalletBoard(result.balance(), source.updatedAt, updated, source.ledger);
    }

    public long balance() {
        return balance;
    }

    /* 余额最近一次变动时刻, Unix 毫秒; 服务端未回传时为零. */
    public long updatedAt() {
        return updatedAt;
    }

    public CheckinStatus checkin() {
        return checkin;
    }

    public List<LedgerEntry> ledger() {
        return ledger;
    }

    /*
     * 已签到的日期键列表, 形如 2026-11-10, 与签到状态同月.
     * 用 LinkedHashSet 去重并保持顺序, 供日历高亮.
     */
    public List<String> signedDates() {
        Set<String> dates = new LinkedHashSet<>();
        dates.addAll(checkin.signedDates());
        for (LedgerEntry entry : ledger) {
            if (LedgerEntry.REASON_CHECKIN.equals(entry.reason()) && !entry.ref().isEmpty()) {
                dates.add(entry.ref());
            }
        }
        return new ArrayList<>(dates);
    }
}