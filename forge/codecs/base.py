# -*- coding: utf-8 -*-
"""编解码器基类与载荷模型。

保真级别 (fidelity) 是全管线的核心契约:

- lossless-edit: 工程文件是权威数据, 编码回去与原始二进制逐字节相同,
  工程文件可被人工编辑。
- lossless-passthrough: 工程文件里保存的是原始载荷字节, 只附带可读视图,
  不做结构改写; 编码回去同样逐字节相同, 但不支持改动结构。
"""

from __future__ import annotations

from dataclasses import dataclass, field
from typing import Dict, Optional


class CodecError(Exception):
    """编解码失败, 消息里必须带可定位的上下文。"""


FIDELITY_EDIT = "lossless-edit"
FIDELITY_PASSTHROUGH = "lossless-passthrough"


@dataclass
class Payload:
    """一次解码的产物。

    kind 为 text 时填 text 字段, 为 binary 时填 blob 字段。
    meta 保存重新编码所需的全部参数, 不允许存在只存在于内存里的隐式状态。
    """

    kind: str
    text: Optional[str] = None
    blob: Optional[bytes] = None
    meta: Dict = field(default_factory=dict)

    def raw_bytes(self) -> bytes:
        if self.kind == "text":
            from ..util.textio import encode_text

            return encode_text(
                self.text or "",
                self.meta.get("encoding", "utf-8"),
                bool(self.meta.get("bom", False)),
            )
        if self.blob is None:
            raise CodecError("binary 载荷缺少 blob")
        return self.blob


class Codec:
    """编解码器接口。"""

    name = "base"
    fidelity = FIDELITY_PASSTHROUGH
    """可读性分级: text / binary-structured / binary-standard / binary-opaque。"""
    readability = "binary-opaque"
    extensions = ()
    magics = ()

    def sniff(self, data: bytes) -> bool:
        """按魔数判断是否归本编解码器处理。"""
        return False

    def decode(self, data: bytes) -> Payload:
        raise NotImplementedError

    def encode(self, payload: Payload) -> bytes:
        """默认实现原样回写载荷字节, 适用于 lossless-passthrough 的编解码器。"""
        return payload.raw_bytes()

    def view(self, payload: Payload):
        """可读视图, 供工程文件旁挂的 json 使用; 无视图返回 None。"""
        return None
