#include "ReplxxConfigurator.h"

auto ReplxxConfigurator::configure(const std::unique_ptr<replxx::Replxx>& rx,
                                   const CompletionCallback& completionCallback, const HintCallback& hintCallback)
        -> void
{
    if (!rx)
        return;

    /// 基础配置
    setBasicConfig(*rx);

    /// 设置回调
    rx->set_completion_callback([completionCallback](const std::string& input, int& contextLen)
                                { return completionCallback(input, contextLen); });

    rx->set_hint_callback([hintCallback](const std::string& input, int& contextLen, replxx::Replxx::Color& color)
                          { return hintCallback(input, contextLen, color); });

    /// 绑定快捷键
    bindKeys(*rx);
}

auto ReplxxConfigurator::setBasicConfig(replxx::Replxx& rx) -> void
{
    rx.set_max_history_size(128);
    rx.set_max_hint_rows(3);
    rx.set_completion_count_cutoff(128);
    rx.set_hint_delay(500);

    setWordBreakCharacters(rx);

    rx.set_complete_on_empty(true);
    rx.set_beep_on_ambiguous_completion(false);
    rx.set_no_color(false);
    rx.set_double_tab_completion(false);
    rx.set_indent_multiline(false);
}

auto ReplxxConfigurator::setWordBreakCharacters(replxx::Replxx& rx) -> void
{
#ifdef _WIN32
    rx.set_word_break_characters(" \t\n-%!;:=*~^<>?|&()");
#else
    rx.set_word_break_characters(" \t\n-%!;:=*~^<>?|&()");
#endif
}

void ReplxxConfigurator::bindKeys(replxx::Replxx& rx)
{
    using KEY = replxx::Replxx::KEY;

    rx.bind_key_internal(KEY::control('L'), "clear_screen");
    rx.bind_key_internal(KEY::BACKSPACE, "delete_character_left_of_cursor");
    rx.bind_key_internal(KEY::DELETE, "delete_character_under_cursor");
    rx.bind_key_internal(KEY::control('D'), "send_eof");
    rx.bind_key_internal(KEY::control('C'), "abort_line");
    rx.bind_key_internal(KEY::TAB, "complete_line");
}
