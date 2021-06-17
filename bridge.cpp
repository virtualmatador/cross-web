#include <condition_variable>
#include <mutex>
#include <sstream>
#include <list>
#include <vector>

#include "emscripten.h"

#include "../core/src/bridge.h"
#include "../core/src/interface.h"
#include "../core/src/stage.h"

std::condition_variable work_condition_;
std::mutex work_mutex_;
bool need_start_{ true };
bool need_stop_{ false };
bool need_exit_{ false };
bool started_{ false };

struct Message
{
    std::int32_t sender;
    std::string id;
    std::string command;
    std::string info;
};

std::list<Message> messages_;
std::list<std::string> functions_;
std::mutex functions_lock_;
std::vector<uint32_t> pixels_;
std::mutex pixels_lock_;

extern "C"
{
    void PostJsMessage(const std::int32_t sender, const char* id, const char* command, const char* info)
    {
        {
            std::lock_guard<std::mutex> auto_lock{ work_mutex_ };
            messages_.push_back({sender, id, command, info});
        }
        work_condition_.notify_one();
    }

    void NeedExit()
    {
        {
            std::lock_guard<std::mutex> auto_lock{ work_mutex_ };
            need_exit_ = true;
        }
        work_condition_.notify_one();
    }

    void NeedStart()
    {
        {
            std::lock_guard<std::mutex> auto_lock{ work_mutex_ };
            need_start_ = true;
        }
        work_condition_.notify_one();
    }

    void NeedStop()
    {
        {
            std::lock_guard<std::mutex> auto_lock{ work_mutex_ };
            need_stop_ = true;
        }
        work_condition_.notify_one();
    }

    void NeedEscape()
    {
        MAIN_THREAD_EM_ASM(
        {
            Module.ccall('PostJsMessage', null, ['number', 'string', 'string', 'string'],
                [view_id_, "", "escape", ""]);
        });
    }

    const char* PickFunction()
    {
        std::lock_guard<std::mutex> auto_lock{ functions_lock_ };
        return functions_.front().c_str();
    }

    void PopFunction()
    {
        std::lock_guard<std::mutex> auto_lock{ functions_lock_ };
        functions_.pop_front();
    }

    void* CreatePixels(const std::int32_t image_width, const std::int32_t image_height)
    {
        std::lock_guard<std::mutex> auto_lock{ pixels_lock_ };
        pixels_.resize(image_width * image_height);
        return pixels_.data();
    }

    void LockPixels()
    {
        pixels_lock_.lock();
    }

    void UnlockPixels()
    {
        pixels_lock_.unlock();
    }
}

void bridge::NeedRestart()
{
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        Module.ccall('PostJsMessage', null, ['number', 'string', 'string', 'string'],
            [view_id_, "", "restart", ""]);
    });
}

void bridge::LoadWebView(const std::int32_t sender, const std::int32_t view_info, const char *html, const char* waves)
{
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        LoadWebView($0, $1, $2, $3);
    }, sender, view_info, html, waves);
}

void bridge::LoadImageView(const std::int32_t sender, const std::int32_t view_info, const std::int32_t image_width, const char* waves)
{
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        LoadImageView($0, $1, $2, $3);
    }, sender, view_info, image_width, waves);
}

std::uint32_t* bridge::GetPixels()
{
    pixels_lock_.lock();
    return pixels_.data();
}

void bridge::ReleasePixels(std::uint32_t* const pixels)
{
    pixels_lock_.unlock();
}

void bridge::RefreshImageView()
{
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        RefreshImageView();
    });
}

void bridge::CallFunction(const char* function)
{
    {
        std::lock_guard<std::mutex> auto_lock{ functions_lock_ };
        functions_.push_back(function);
    }
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        document.getElementById('web_view').contentWindow.eval(
            Module.ccall("PickFunction", 'string', null, null));
        Module.ccall("PopFunction", null, null, null);
    });
}

std::string bridge::GetAsset(const char* key)
{
    // TODO
    return "";
}

std::string bridge::GetPreference(const char* key)
{
    char* value = (char*)MAIN_THREAD_EM_ASM_INT(
    {
        var value = localStorage.getItem(UTF8ToString($0)) || '';
        var buffer = Module._malloc(value.length + 1);
        Module.stringToUTF8(value, buffer, value.length + 1);
        return buffer;
    }, key);
    std::string result{ value };
    free(value);
    return result;
}

void bridge::SetPreference(const char* key, const char* value)
{
    MAIN_THREAD_EM_ASM(
    {
        localStorage.setItem(UTF8ToString($0), UTF8ToString($1));
    }, key, value);
}

void bridge::PostThreadMessage(const std::int32_t sender, const char* id, const char* command, const char* info)
{
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        Module.ccall('PostJsMessage', null, ['number', 'string', 'string', 'string'],
            [$0, UTF8ToString($1), UTF8ToString($2), UTF8ToString($3)]);
    }, sender, id, command, info);
}

void bridge::AddParam(const char* key, const char* value)
{
    // TODO
}

void bridge::PostHttp(const std::int32_t sender, const char* id, const char* command, const char* url)
{
    // TODO
}

void bridge::PlayAudio(const std::int32_t index)
{
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        var audio = new Audio(document.getElementById('audio-' + $0).getAttribute('src'));
        audio.play();
    }, index);
}

void bridge::Exit()
{
    MAIN_THREAD_ASYNC_EM_ASM(
    {
        location = "about:blank";
    });
}

int main()
{
    interface::Begin();
    interface::Create();
    for(;;)
    {
        std::unique_lock<std::mutex> work_locker{ work_mutex_ };
        work_condition_.wait(work_locker, [&]()
        {
            return need_exit_ || need_stop_ || need_start_ || !messages_.empty();
        });
        if (need_exit_)
        {
            work_locker.unlock();
            break;
        }
        else if (need_stop_)
        {
            need_stop_ = false;
            if (started_)
            {
                started_ = false;
                work_locker.unlock();
                interface::Stop();
            }
        }
        else if (need_start_)
        {
            need_start_ = false;
            if (!started_)
            {
                started_ = true;
                work_locker.unlock();
                interface::Start();
            }
        }
        else
        {
            Message msg = messages_.front();
            messages_.pop_front();
            work_locker.unlock();
            if (!msg.id.empty())
            {
                interface::HandleAsync(msg.sender, msg.id.c_str(), msg.command.c_str(), msg.info.c_str());
            }
            else if (msg.sender == core::Stage::index_)
            {
                if (msg.command == "restart")
                {
                    interface::Restart();
                }
                else if (msg.command == "escape")
                {
                    interface::Escape();
                }
            }
        }
    }
    if (started_)
    {
        started_ = false;
        interface::Stop();
    }
    interface::Destroy();
    interface::End();
    return 0;
}
