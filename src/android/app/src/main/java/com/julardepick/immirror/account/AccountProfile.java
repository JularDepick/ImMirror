package com.julardepick.immirror.account;

/*
 * 账号资料. 只保留界面需要的字段, 其余响应字段忽略.
 */
public final class AccountProfile {

    private final String uid;
    private final String email;
    private final long balance;
    /* 角色皮肤编号, 0 表示默认外观. */
    private final int skinId;

    public AccountProfile(String uid, String email, long balance, int skinId) {
        this.uid = uid;
        this.email = email;
        this.balance = balance;
        this.skinId = skinId;
    }

    public int skinId() {
        return skinId;
    }

    public String uid() {
        return uid;
    }

    public String email() {
        return email;
    }

    /* 镜币余额, 由服务端钱包返回. */
    public long balance() {
        return balance;
    }
}
