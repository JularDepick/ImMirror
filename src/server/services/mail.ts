/**
 * 邮件发送抽象
 * MAIL_TRANSPORT=smtp 时通过 nodemailer 投递, =memory 时只保留在内存 (测试与本地调试用)
 */

import nodemailer from "nodemailer";
import type { AppConfig } from "../utils/config";
import type { CodePurpose } from "../db/repositories/codes";

export interface MailMessage {
  to: string;
  subject: string;
  text: string;
  purpose?: CodePurpose;
  code?: string;
}

export interface MailTransport {
  send(message: MailMessage): Promise<void>;
}

/**
 * 内存通道: 保存所有投递记录, 供测试读取验证码
 */
export class MemoryMailTransport implements MailTransport {
  private readonly messages: MailMessage[] = [];

  async send(message: MailMessage): Promise<void> {
    this.messages.push(message);
  }

  list(): MailMessage[] {
    return [...this.messages];
  }

  findLatest(to: string, purpose: CodePurpose): MailMessage | undefined {
    for (let index = this.messages.length - 1; index >= 0; index -= 1) {
      const message = this.messages[index];
      if (message && message.to === to && message.purpose === purpose) {
        return message;
      }
    }
    return undefined;
  }

  clear(): void {
    this.messages.length = 0;
  }
}

/**
 * SMTP 通道: 使用 nodemailer, 连接参数来自配置
 */
export class SmtpMailTransport implements MailTransport {
  private readonly transporter: nodemailer.Transporter;
  private readonly from: string;

  constructor(config: AppConfig) {
    this.transporter = nodemailer.createTransport({
      host: config.smtp.host,
      port: config.smtp.port,
      secure: config.smtp.secure,
      auth:
        config.smtp.user === "" ? undefined : { user: config.smtp.user, pass: config.smtp.password },
    });
    this.from = config.smtp.from === "" ? config.smtp.user : config.smtp.from;
  }

  async send(message: MailMessage): Promise<void> {
    await this.transporter.sendMail({ from: this.from, to: message.to, subject: message.subject, text: message.text });
  }
}

const PURPOSE_SUBJECTS: Record<CodePurpose, string> = {
  register: "我之镜界 注册验证码",
  login: "我之镜界 登录验证码",
  reset: "我之镜界 重置密码验证码",
  rebind: "我之镜界 换绑邮箱验证码",
};

const PURPOSE_ACTIONS: Record<CodePurpose, string> = {
  register: "注册账号",
  login: "验证码登录",
  reset: "重置密码",
  rebind: "换绑邮箱",
};

export interface MailService {
  readonly transport: MailTransport;
  // 仅 memory 通道存在, 测试用它读取验证码明文
  readonly memory?: MemoryMailTransport;
  sendCode(email: string, purpose: CodePurpose, code: string, ttlSeconds: number): Promise<void>;
}

/**
 * 组装邮件服务; 只有 memory 通道才会按 MAIL_DEBUG_LOG 输出验证码明文
 */
export function createMailService(config: AppConfig): MailService {
  const transport: MailTransport =
    config.mailTransport === "memory" ? new MemoryMailTransport() : new SmtpMailTransport(config);
  const memory = transport instanceof MemoryMailTransport ? transport : undefined;

  return {
    transport,
    memory,
    async sendCode(email: string, purpose: CodePurpose, code: string, ttlSeconds: number): Promise<void> {
      const text = [
        `您正在${PURPOSE_ACTIONS[purpose]}, 验证码为 ${code}, ${Math.round(ttlSeconds / 60)} 分钟内有效。`,
        "若非本人操作请忽略本邮件。",
      ].join("\n");
      await transport.send({ to: email, subject: PURPOSE_SUBJECTS[purpose], text, purpose, code });
      if (memory && config.mailDebugLog) {
        console.log(`[mail][memory] ${purpose} -> ${email}: ${code}`);
      }
    },
  };
}
