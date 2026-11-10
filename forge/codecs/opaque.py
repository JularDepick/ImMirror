# -*- coding: utf-8 -*-
"""兜底编解码器: 未解格式原样保留, 只记录摘要与魔数, 保证管线不丢数据。"""

from __future__ import annotations

import hashlib

from .base import Codec, FIDELITY_PASSTHROUGH, Payload


class OpaqueCodec(Codec):
    name = "opaque"
    fidelity = FIDELITY_PASSTHROUGH
    extensions = ()
    magics = ()

    def decode(self, data: bytes) -> Payload:
        return Payload(
            kind="binary",
            blob=data,
            meta={
                "container": "opaque",
                "magic": data[:8].hex(),
                "sha256": hashlib.sha256(data).hexdigest(),
                "plain_size": len(data),
            },
        )
