#include "editor/trigger_model.h"

#include <cmath>
#include <cstdio>
#include <cstring>

#include "editor/generated/editor_data_gen.h"

namespace immirror::editor {

namespace {

/* 把浮点写成脚本可读的最短形式, 去掉多余的零. */
std::string FormatFloat(float value) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%.3f", static_cast<double>(value));
    std::string text(buffer);
    while (!text.empty() && text.back() == '0') {
        text.pop_back();
    }
    if (!text.empty() && text.back() == '.') {
        text.pop_back();
    }
    if (text.empty() || text == "-0") {
        text = "0";
    }
    return text;
}

/* 把文本转义成可以把整个脚本放进 JSON 字符串的形式; 换行压成空格. */
std::string EscapeScriptText(const std::string& text) {
    std::string out;
    for (const char c : text) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
            case '\r':
            case '\t':
                out.push_back(' ');
                break;
            default:
                out.push_back(c);
                break;
        }
    }
    return out;
}

/* DSL 字符串字面量: 双引号包裹, 转义引号与反斜杠. */
std::string ScriptString(const std::string& text) {
    return "\"" + EscapeScriptText(text) + "\"";
}

}  /* namespace */

std::string TriggerAction::ToScriptCall() const {
    char buffer[160];
    switch (type) {
        case TriggerActionType::kLog:
            return "log(" + ScriptString(text) + ")";
        case TriggerActionType::kSetBlock:
            std::snprintf(buffer, sizeof(buffer), "world_set_block(%d, %d, %d, %d)", x, y, z, block);
            return buffer;
        case TriggerActionType::kTeleport:
            std::snprintf(buffer, sizeof(buffer), "player_teleport(%s, %s, %s)",
                          FormatFloat(fx).c_str(), FormatFloat(fy).c_str(), FormatFloat(fz).c_str());
            return buffer;
    }
    return std::string();
}

std::string TriggerAction::DescribeAscii() const {
    char buffer[160];
    switch (type) {
        case TriggerActionType::kLog:
            std::snprintf(buffer, sizeof(buffer), "LOG \"%s\"", text.c_str());
            return buffer;
        case TriggerActionType::kSetBlock:
            std::snprintf(buffer, sizeof(buffer), "SET %d %d %d -> %d", x, y, z, block);
            return buffer;
        case TriggerActionType::kTeleport:
            std::snprintf(buffer, sizeof(buffer), "TP %.1f %.1f %.1f",
                          static_cast<double>(fx), static_cast<double>(fy),
                          static_cast<double>(fz));
            return buffer;
    }
    return std::string();
}

void Trigger::RebuildScript() {
    std::string source;
    /* 脚本以动作顺序逐行拼出, 每行一条语句, 末尾统一带分号. */
    for (const TriggerAction& action : actions) {
        source += action.ToScriptCall();
        source += ";\n";
    }
    if (source.empty()) {
        source = "log(\"trigger " + std::to_string(id) + " has no action\");\n";
    }
    script = source;
}

const char* TriggerEventKey(TriggerEventType type) {
    switch (type) {
        case TriggerEventType::kEnterRegion:
            return "enter_region";
        case TriggerEventType::kBlockBroken:
            return "block_broken";
        case TriggerEventType::kTimer:
            return "timer";
    }
    return "enter_region";
}

const char* TriggerEventNameEn(TriggerEventType type) {
    switch (type) {
        case TriggerEventType::kEnterRegion:
            return "ENTER REGION";
        case TriggerEventType::kBlockBroken:
            return "BLOCK BROKEN";
        case TriggerEventType::kTimer:
            return "TIMER";
    }
    return "ENTER REGION";
}

const char* TriggerEventNameZh(TriggerEventType type) {
    switch (type) {
        case TriggerEventType::kEnterRegion:
            return "进入区域";
        case TriggerEventType::kBlockBroken:
            return "方块被破坏";
        case TriggerEventType::kTimer:
            return "定时";
    }
    return "进入区域";
}

const char* TriggerActionKey(TriggerActionType type) {
    switch (type) {
        case TriggerActionType::kLog:
            return "log";
        case TriggerActionType::kSetBlock:
            return "set_block";
        case TriggerActionType::kTeleport:
            return "teleport";
    }
    return "log";
}

const char* TriggerActionNameEn(TriggerActionType type) {
    switch (type) {
        case TriggerActionType::kLog:
            return "LOG";
        case TriggerActionType::kSetBlock:
            return "SET BLOCK";
        case TriggerActionType::kTeleport:
            return "TELEPORT";
    }
    return "LOG";
}

const char* TriggerActionNameZh(TriggerActionType type) {
    switch (type) {
        case TriggerActionType::kLog:
            return "输出日志";
        case TriggerActionType::kSetBlock:
            return "放置方块";
        case TriggerActionType::kTeleport:
            return "传送玩家";
    }
    return "输出日志";
}

bool ParseTriggerEventKey(const std::string& key, TriggerEventType& out) {
    for (int32_t index = 0; index < kEventTypeCount; ++index) {
        const TriggerEventType candidate = TriggerEventAt(index);
        if (key == TriggerEventKey(candidate)) {
            out = candidate;
            return true;
        }
    }
    return false;
}

bool ParseTriggerActionKey(const std::string& key, TriggerActionType& out) {
    for (int32_t index = 0; index < kActionTypeCount; ++index) {
        const TriggerActionType candidate = TriggerActionAt(index);
        if (key == TriggerActionKey(candidate)) {
            out = candidate;
            return true;
        }
    }
    return false;
}

TriggerEventType TriggerEventAt(int32_t index) {
    switch (index) {
        case 1:
            return TriggerEventType::kBlockBroken;
        case 2:
            return TriggerEventType::kTimer;
        default:
            return TriggerEventType::kEnterRegion;
    }
}

TriggerActionType TriggerActionAt(int32_t index) {
    switch (index) {
        case 1:
            return TriggerActionType::kSetBlock;
        case 2:
            return TriggerActionType::kTeleport;
        default:
            return TriggerActionType::kLog;
    }
}

int32_t TriggerEventItemId(TriggerEventType type) {
    /* 事件 ID 与生成表里的 ASCII 键一一对应, 数据表调整时这里自动跟随. */
    const char* keySuffix = nullptr;
    switch (type) {
        case TriggerEventType::kEnterRegion:
            keySuffix = "EVENT_ENTER_REGION";
            break;
        case TriggerEventType::kBlockBroken:
            keySuffix = "EVENT_BLOCK_BROKEN";
            break;
        case TriggerEventType::kTimer:
            keySuffix = "EVENT_TIMER";
            break;
    }
    const gen::TriggerItemEntry* entry = gen::TriggerItemOfKey(keySuffix);
    return entry != nullptr ? entry->itemId : 0;
}

int32_t TriggerActionItemId(TriggerActionType type) {
    const char* keySuffix = nullptr;
    switch (type) {
        case TriggerActionType::kLog:
            keySuffix = "ACTION_LOG";
            break;
        case TriggerActionType::kSetBlock:
            keySuffix = "ACTION_SET_BLOCK";
            break;
        case TriggerActionType::kTeleport:
            keySuffix = "ACTION_TELEPORT";
            break;
    }
    const gen::TriggerItemEntry* entry = gen::TriggerItemOfKey(keySuffix);
    return entry != nullptr ? entry->itemId : 0;
}

JsonValue TriggerToJson(const Trigger& trigger) {
    JsonValue root = JsonValue::MakeObject();
    root.Set("format", JsonValue::MakeString("immirror.trigger"));
    root.Set("formatVersion", JsonValue::MakeInt(1));
    root.Set("id", JsonValue::MakeInt(trigger.id));
    root.Set("name", JsonValue::MakeString(trigger.name));
    root.Set("enabled", JsonValue::MakeBool(trigger.enabled));

    JsonValue event = JsonValue::MakeObject();
    event.Set("type", JsonValue::MakeString(TriggerEventKey(trigger.eventType)));
    event.Set("itemId", JsonValue::MakeInt(TriggerEventItemId(trigger.eventType)));
    switch (trigger.eventType) {
        case TriggerEventType::kEnterRegion: {
            JsonValue region = JsonValue::MakeObject();
            region.Set("cx", JsonValue::MakeInt(trigger.region.cx));
            region.Set("cy", JsonValue::MakeInt(trigger.region.cy));
            region.Set("cz", JsonValue::MakeInt(trigger.region.cz));
            region.Set("sx", JsonValue::MakeInt(trigger.region.sx));
            region.Set("sy", JsonValue::MakeInt(trigger.region.sy));
            region.Set("sz", JsonValue::MakeInt(trigger.region.sz));
            event.Set("region", std::move(region));
            break;
        }
        case TriggerEventType::kBlockBroken:
            event.Set("block", JsonValue::MakeInt(static_cast<int64_t>(trigger.brokenBlock)));
            break;
        case TriggerEventType::kTimer:
            event.Set("intervalSeconds", JsonValue::MakeNumber(
                                             static_cast<double>(trigger.intervalSeconds)));
            event.Set("fireImmediately", JsonValue::MakeBool(trigger.fireImmediately));
            break;
    }
    root.Set("event", std::move(event));

    JsonValue actions = JsonValue::MakeArray();
    for (const TriggerAction& action : trigger.actions) {
        JsonValue item = JsonValue::MakeObject();
        item.Set("type", JsonValue::MakeString(TriggerActionKey(action.type)));
        item.Set("itemId", JsonValue::MakeInt(TriggerActionItemId(action.type)));
        switch (action.type) {
            case TriggerActionType::kLog:
                item.Set("text", JsonValue::MakeString(action.text));
                break;
            case TriggerActionType::kSetBlock:
                item.Set("x", JsonValue::MakeInt(action.x));
                item.Set("y", JsonValue::MakeInt(action.y));
                item.Set("z", JsonValue::MakeInt(action.z));
                item.Set("block", JsonValue::MakeInt(action.block));
                break;
            case TriggerActionType::kTeleport:
                item.Set("x", JsonValue::MakeNumber(static_cast<double>(action.fx)));
                item.Set("y", JsonValue::MakeNumber(static_cast<double>(action.fy)));
                item.Set("z", JsonValue::MakeNumber(static_cast<double>(action.fz)));
                break;
        }
        actions.Push(std::move(item));
    }
    root.Set("actions", std::move(actions));
    root.Set("script", JsonValue::MakeString(trigger.script));
    return root;
}

bool TriggerFromJson(const JsonValue& value, Trigger& out, std::string& error) {
    if (!value.IsObject()) {
        error = "触发器不是 JSON 对象";
        return false;
    }
    const std::string format = value.GetString("format", "immirror.trigger");
    if (format != "immirror.trigger") {
        error = "触发器 format 字段不是 immirror.trigger";
        return false;
    }
    out = Trigger{};
    out.id = static_cast<int32_t>(value.GetInt("id", 0));
    out.name = value.GetString("name", "触发器");
    out.enabled = value.GetBool("enabled", true);

    const JsonValue* event = value.Find("event");
    if (event == nullptr || !event->IsObject()) {
        error = "触发器缺少 event 对象";
        return false;
    }
    const std::string eventKey = event->GetString("type", "enter_region");
    if (!ParseTriggerEventKey(eventKey, out.eventType)) {
        error = "未知的事件类型: " + eventKey;
        return false;
    }
    switch (out.eventType) {
        case TriggerEventType::kEnterRegion: {
            const JsonValue* region = event->Find("region");
            if (region == nullptr || !region->IsObject()) {
                error = "进入区域事件缺少 region 对象";
                return false;
            }
            out.region.cx = static_cast<int32_t>(region->GetInt("cx", 0));
            out.region.cy = static_cast<int32_t>(region->GetInt("cy", 0));
            out.region.cz = static_cast<int32_t>(region->GetInt("cz", 0));
            out.region.sx = static_cast<int32_t>(region->GetInt("sx", 1));
            out.region.sy = static_cast<int32_t>(region->GetInt("sy", 1));
            out.region.sz = static_cast<int32_t>(region->GetInt("sz", 1));
            out.region.Normalize();
            break;
        }
        case TriggerEventType::kBlockBroken:
            out.brokenBlock = static_cast<voxel::BlockId>(event->GetInt("block", 0));
            break;
        case TriggerEventType::kTimer:
            out.intervalSeconds = static_cast<float>(event->GetNumber("intervalSeconds", 2.0));
            if (out.intervalSeconds < 0.1f) {
                out.intervalSeconds = 0.1f;
            }
            out.fireImmediately = event->GetBool("fireImmediately", false);
            break;
    }

    const JsonValue* actions = value.Find("actions");
    if (actions != nullptr && actions->IsArray()) {
        for (const JsonValue& item : actions->Items()) {
            if (!item.IsObject()) {
                error = "动作不是 JSON 对象";
                return false;
            }
            TriggerAction action;
            const std::string actionKey = item.GetString("type", "log");
            if (!ParseTriggerActionKey(actionKey, action.type)) {
                error = "未知的动作类型: " + actionKey;
                return false;
            }
            action.itemId = static_cast<int32_t>(item.GetInt("itemId", 0));
            switch (action.type) {
                case TriggerActionType::kLog:
                    action.text = item.GetString("text", "");
                    break;
                case TriggerActionType::kSetBlock:
                    action.x = static_cast<int32_t>(item.GetInt("x", 0));
                    action.y = static_cast<int32_t>(item.GetInt("y", 0));
                    action.z = static_cast<int32_t>(item.GetInt("z", 0));
                    action.block = static_cast<int32_t>(item.GetInt("block", 0));
                    break;
                case TriggerActionType::kTeleport:
                    action.fx = static_cast<float>(item.GetNumber("x", 0.0));
                    action.fy = static_cast<float>(item.GetNumber("y", 0.0));
                    action.fz = static_cast<float>(item.GetNumber("z", 0.0));
                    break;
            }
            if (out.actions.size() < static_cast<size_t>(limits::kMaxActions)) {
                out.actions.push_back(action);
            }
        }
    }

    out.script = value.GetString("script", "");
    if (out.script.empty()) {
        /* 手写工程可能只给了动作列表, 缺少源码时按动作重建, 保证一定能执行. */
        out.RebuildScript();
    }
    return true;
}

}  /* namespace immirror::editor */
