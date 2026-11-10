package com.julardepick.immirror.account;

/*
 * 验证码投递结果. 有效时长与发送冷却都由服务端给出, 客户端不再自行约定.
 */
public final class CodeDelivery {

    private final int ttlSeconds;
    private final int cooldownSeconds;

    public CodeDelivery(int ttlSeconds, int cooldownSeconds) {
        this.ttlSeconds = ttlSeconds;
        this.cooldownSeconds = cooldownSeconds;
    }

    /* 验证码有效秒数. */
    public int ttlSeconds() {
        return ttlSeconds;
    }

    /* 同一邮箱同用途的再次发送冷却秒数. */
    public int cooldownSeconds() {
        return cooldownSeconds;
    }
}
