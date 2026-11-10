package com.julardepick.immirror.account;

import android.content.Context;

import com.julardepick.immirror.R;

/*
 * 错误码到界面文案的映射.
 *
 * 文案取自服务端 docs/error-codes.md 的面向用户列, 保证客户端提示与服务端语义一致;
 * 未知错误码回落到服务端 message, 再回落到通用文案.
 */
public final class ErrorMessages {

    private ErrorMessages() {
    }

    public static String messageFor(Context context, ApiException error) {
        if (error == null) {
            return context.getString(R.string.account_error_unknown);
        }
        int resource = resourceFor(error.code());
        if (resource != 0) {
            return context.getString(resource);
        }
        String detail = error.detail().trim();
        if (!detail.isEmpty()) {
            return detail;
        }
        return context.getString(R.string.account_error_unknown);
    }

    private static int resourceFor(String code) {
        switch (code) {
            case "BAD_REQUEST":
                return R.string.account_error_bad_request;
            case "AUTH_INVALID_IDENTIFIER":
                return R.string.account_error_invalid_identifier;
            case "AUTH_INVALID_CREDENTIALS":
                return R.string.account_error_invalid_credentials;
            case "AUTH_CODE_INVALID":
                return R.string.account_error_code_invalid;
            case "AUTH_CODE_EXPIRED":
                return R.string.account_error_code_expired;
            case "AUTH_CODE_ATTEMPTS_EXCEEDED":
                return R.string.account_error_code_attempts;
            case "AUTH_EMAIL_TAKEN":
                return R.string.account_error_email_taken;
            case "AUTH_UNAUTHORIZED":
                return R.string.account_error_unauthorized;
            case "AUTH_SESSION_EXPIRED":
                return R.string.account_error_session_expired;
            case "RATE_LIMITED":
                return R.string.account_error_rate_limited;
            case "NOT_FOUND":
                return R.string.account_error_not_found;
            case "INTERNAL":
                return R.string.account_error_internal;
            case ApiException.CODE_NETWORK:
                return R.string.account_error_network;
            case ApiException.CODE_PROTOCOL:
                return R.string.account_error_protocol;
            default:
                return 0;
        }
    }
}
