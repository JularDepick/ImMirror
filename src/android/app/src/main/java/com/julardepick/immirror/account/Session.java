package com.julardepick.immirror.account;

/*
 * 登录会话快照. 令牌由服务端签发, 客户端只做保存与携带, 不做任何解析.
 */
public final class Session {

    private final String token;
    private final String uid;
    private final String email;
    private final long expiresAt;

    public Session(String token, String uid, String email, long expiresAt) {
        this.token = token;
        this.uid = uid;
        this.email = email;
        this.expiresAt = expiresAt;
    }

    public String token() {
        return token;
    }

    public String uid() {
        return uid;
    }

    public String email() {
        return email;
    }

    /* 服务端给出的过期时刻, 单位毫秒; 0 表示服务端未返回, 视为长期有效. */
    public long expiresAt() {
        return expiresAt;
    }

    public boolean isUsable() {
        if (token == null || token.isEmpty()) {
            return false;
        }
        return expiresAt <= 0L || expiresAt > System.currentTimeMillis();
    }
}
