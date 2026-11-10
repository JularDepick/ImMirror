package com.julardepick.immirror.account;

/*
 * 接口调用异常. code 优先取服务端错误码, 网络与解析失败使用本地错误码,
 * 界面层按 code 映射提示文案, 不依赖服务端 message.
 */
public final class ApiException extends Exception {

    /* 本地错误码: 连接失败或服务端响应无法解析. */
    public static final String CODE_NETWORK = "NETWORK";
    public static final String CODE_PROTOCOL = "PROTOCOL";

    private final String code;

    public ApiException(String code, String detail) {
        super(detail == null ? "" : detail);
        this.code = code == null || code.isEmpty() ? CODE_PROTOCOL : code;
    }

    public String code() {
        return code;
    }

    /* 服务端返回的原始描述, 仅用于日志与未知错误码兜底展示. */
    public String detail() {
        return getMessage() == null ? "" : getMessage();
    }

    public boolean isSessionInvalid() {
        return "AUTH_UNAUTHORIZED".equals(code) || "AUTH_SESSION_EXPIRED".equals(code);
    }
}
